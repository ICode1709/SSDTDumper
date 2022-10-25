#include "stdafx.h"
#include "PEFile.h"
#include "SSDTDumper.h"

bool ReadRegistryValue(HKEY KeyHandle, CONST WCHAR Name[], PVOID Buffer, ULONG BufferSize)
{
	ULONG ReturnLength = BufferSize;
	return RegQueryValueExW(KeyHandle, Name, NULL, NULL, (PUCHAR)Buffer, &ReturnLength) == ERROR_SUCCESS;
}
bool GetWindowsVersion(WCHAR WindowsVersion[64])
{
	ULONG CurrentMajorVersionNumber = 0, CurrentMinorVersionNumber = 0, UBR = 0;
	WCHAR CurrentVersion[64] = { 0 };
	WCHAR DisplayVersion[64] = { 0 };
	WCHAR CurrentBuild[64] = { 0 };

	HKEY KeyHandle;
	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", &KeyHandle) == ERROR_SUCCESS)
	{
		if (ReadRegistryValue(KeyHandle, L"CurrentBuild", CurrentBuild, sizeof(CurrentBuild)))
		{
			//This value is available starting from build 10.0.10240 1507
			if (ReadRegistryValue(KeyHandle, L"CurrentMajorVersionNumber", &CurrentMajorVersionNumber, sizeof(CurrentMajorVersionNumber)) &&
				ReadRegistryValue(KeyHandle, L"CurrentMinorVersionNumber", &CurrentMinorVersionNumber, sizeof(CurrentMinorVersionNumber)) &&
				ReadRegistryValue(KeyHandle, L"UBR", &UBR, sizeof(UBR)))
			{
				if (ReadRegistryValue(KeyHandle, L"DisplayVersion", DisplayVersion, sizeof(DisplayVersion)))
				{
					//This value is available starting from build 10.0.19042 20H2
					swprintf_s(WindowsVersion, 64, L"%d.%d.%ws.%d %ws", CurrentMajorVersionNumber, CurrentMinorVersionNumber, CurrentBuild, UBR, DisplayVersion);
					return true;
				}
				else if (ReadRegistryValue(KeyHandle, L"ReleaseId", DisplayVersion, sizeof(DisplayVersion)))
				{
					//This value is available starting from build 10.0.10586 1511
					swprintf_s(WindowsVersion, 64, L"%d.%d.%ws.%d %ws", CurrentMajorVersionNumber, CurrentMinorVersionNumber, CurrentBuild, UBR, DisplayVersion);
					return true;
				}
				else
				{
					swprintf_s(WindowsVersion, 64, L"%d.%d.%ws.%d", CurrentMajorVersionNumber, CurrentMinorVersionNumber, CurrentBuild, UBR);
					return true;
				}
			}
			else if (ReadRegistryValue(KeyHandle, L"CurrentVersion", CurrentVersion, sizeof(CurrentVersion)))
			{
				swprintf_s(WindowsVersion, 64, L"%ws.%ws", CurrentVersion, CurrentBuild);
				return true;
			}
		}
		RegCloseKey(KeyHandle);
	}
	return false;
}

ÑSSDTDumper::ÑSSDTDumper()
{
	wchar_t winversion[64];
	winver.assign(GetWindowsVersion(winversion) ? winversion : L"Unkown");

	curdir.assign(std::filesystem::current_path());
	sysroot.assign(_wgetenv(L"SystemRoot"));

	sympath.assign(curdir + L"\\Symbols");
	symsrv.assign(L"http://msdl.microsoft.com/download/symbols");

	path_ntoskrnl.assign(sysroot + L"\\System32\\ntoskrnl.exe");
	path_win32k.assign(sysroot + L"\\System32\\win32k.sys");

	outfile.assign(curdir + L"\\" + winver + L".log");

	dumping = false;
}
ÑSSDTDumper::~ÑSSDTDumper()
{

}

bool ÑSSDTDumper::dump()
{
	logfile.open(outfile, std::ios::out | std::ios::trunc);
	return dump_table(path_ntoskrnl, false) && dump_table(path_win32k, true);
}
bool ÑSSDTDumper::dump_table(const std::wstring& path, bool win32)
{
	PCSTR Tag = win32 ? "shadow" : "system";


	CPEFile PeFile;
	if (!PeFile.LoadFile(path, sympath, symsrv))
	{
		printf("Err: LoadFile[\"%ws\"]\n", path.c_str());
		return false;
	}

	ULONG RvaServiceTable = 0;
	ULONG RvaServiceLimit = 0;
	ULONG RvaArgumentTable = 0;

	if (win32)
	{
		if (!PeFile.GetSymbol()->GetRvaByName(L"W32pServiceTable", &RvaServiceTable) ||
			!PeFile.GetSymbol()->GetRvaByName(L"W32pServiceLimit", &RvaServiceLimit) ||
			!PeFile.GetSymbol()->GetRvaByName(L"W32pArgumentTable", &RvaArgumentTable))
		{
			printf("GetSymbols Failed\n");
			return false;
		}
	}
	else
	{
		if (!PeFile.GetSymbol()->GetRvaByName(L"KiServiceTable", &RvaServiceTable) ||
			!PeFile.GetSymbol()->GetRvaByName(L"KiServiceLimit", &RvaServiceLimit) ||
			!PeFile.GetSymbol()->GetRvaByName(L"KiArgumentTable", &RvaArgumentTable))
		{
			printf("GetSymbols Failed\n");
			return false;
		}
	}

	ULONG ServiceLimit = PeFile.Read<ULONG>(RvaServiceLimit);

	std::cout << Tag << ".count = " << ServiceLimit << ";" << std::endl;
	logfile << Tag << ".count = " << ServiceLimit << ";" << std::endl;

	if ((PeFile.Read<ULONG_PTR>(RvaServiceTable) & PeFile.GetImageBaseFromHeader()) == PeFile.GetImageBaseFromHeader())
	{
		PULONG_PTR ServiceTable = PULONG_PTR(PeFile.GetImageBase() + RvaServiceTable);
		for (ULONG i = 0; i < ServiceLimit; ++i)
		{
			std::wstring name;
			if (!PeFile.GetSymbol()->GetNameByRva(ULONG(ServiceTable[i] - PeFile.GetImageBaseFromHeader()), name))
			{
				wchar_t buffer[64];
				swprintf_s(buffer, 64, L"Rva[%I64X]", ServiceTable[i] - PeFile.GetImageBaseFromHeader());
				name.assign(buffer);
			}

			win32 ? shadow_table[i + 0x1000].assign(name) : system_table[i].assign(name);

			char buffer[255];
			sprintf_s(buffer, 255, "%s.table[%d] = \"%ws\";", Tag, win32 ? i + 0x1000 : i, name.c_str());
			std::wcout << buffer << std::endl;
			logfile << buffer << std::endl;
		}
	}
	else
	{
		//starting with build 10.0.14363 1607, this table is not pointers to the functions themselves.
		PULONG ServiceTable = PULONG(PeFile.GetImageBase() + RvaServiceTable);
		for (ULONG i = 0; i < ServiceLimit; ++i)
		{
			std::wstring name;
			if (!PeFile.GetSymbol()->GetNameByRva(ServiceTable[i], name))
			{
				wchar_t buffer[64];
				swprintf_s(buffer, 64, L"Rva[0x%X]", ServiceTable[i]);
				name.assign(buffer);
			}
			win32 ? shadow_table[i + 0x1000].assign(name) : system_table[i].assign(name);

			char buffer[255];
			sprintf_s(buffer, 255, "%s.table[%d] = \"%ws\";", Tag, win32 ? i + 0x1000 : i, name.c_str());
			std::wcout << buffer << std::endl;
			logfile << buffer << std::endl;
		}
	}
	return !(win32 ? shadow_table.empty() : system_table.empty());
}