#include "stdafx.h"
#include "PEFile.h"

PIMAGE_NT_HEADERS RtlImageNtHeader(IN PVOID Base)
{
	PIMAGE_DOS_HEADER pIDH = (PIMAGE_DOS_HEADER)Base;
	if (pIDH->e_magic != IMAGE_DOS_SIGNATURE)
		return NULL;

	PIMAGE_NT_HEADERS pINH = (PIMAGE_NT_HEADERS)(PUCHAR(Base) + pIDH->e_lfanew);
	if (pINH->Signature != IMAGE_NT_SIGNATURE)
		return NULL;

	return pINH;
}

PIMAGE_BASE_RELOCATION LdrProcessRelocationBlockLongLong(IN ULONG_PTR Address, IN ULONG Count, IN PUSHORT TypeOffset, IN LONGLONG Delta)
{
	SHORT Offset;
	USHORT Type;
	ULONG i;
	PUSHORT ShortPtr;
	PULONG LongPtr;
	PULONGLONG LongLongPtr;

	for (i = 0; i < Count; i++)
	{
		Offset = *TypeOffset & 0xFFF;
		Type = *TypeOffset >> 12;
		ShortPtr = PUSHORT(PUCHAR(Address) + Offset);

		switch (Type)
		{
		case IMAGE_REL_BASED_ABSOLUTE:
			break;

		case IMAGE_REL_BASED_HIGH:
			*ShortPtr = HIWORD(MAKELONG(0, *ShortPtr) + (Delta & 0xFFFFFFFF));
			break;

		case IMAGE_REL_BASED_LOW:
			*ShortPtr = *ShortPtr + LOWORD(Delta & 0xFFFF);
			break;

		case IMAGE_REL_BASED_HIGHLOW:
			LongPtr = PULONG(PUCHAR(Address) + Offset);
			*LongPtr = *LongPtr + (Delta & 0xFFFFFFFF);
			break;

		case IMAGE_REL_BASED_DIR64:
			LongLongPtr = PULONGLONG(PUCHAR(Address) + Offset);
			*LongLongPtr = *LongLongPtr + Delta;
			break;

		case IMAGE_REL_BASED_HIGHADJ:
		case IMAGE_REL_BASED_MIPS_JMPADDR:
		default:
			printf("Unknown/unsupported fixup type %hu.\n", Type);
			printf("Address %p, Current %u, Count %u, *TypeOffset %x\n", (PVOID)Address, i, Count, *TypeOffset);
			return (PIMAGE_BASE_RELOCATION)NULL;
		}

		TypeOffset++;
	}

	return (PIMAGE_BASE_RELOCATION)TypeOffset;
}




CPEFile::CPEFile()
{
	m_imagebasehdr = NULL;
	m_imagebase = NULL;
	m_file = NULL;
}
CPEFile::~CPEFile()
{
	if (m_file)
		VirtualFree(m_file, 0, MEM_RELEASE);
}

bool CPEFile::LoadFile(const std::wstring& path, const std::wstring& sympath, const std::wstring& symsrv)
{
	if (!m_symbols.LoadFromPeFile(path, sympath, symsrv))
		return false;

	HANDLE hFile = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	ULONG FileSize;
	if ((FileSize = GetFileSize(hFile, NULL)) == NULL)
	{
		CloseHandle(hFile);
		return false;
	}

	PVOID FileBuffer;
	if ((FileBuffer = VirtualAlloc(NULL, FileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)) == NULL)
	{
		CloseHandle(hFile);
		return false;
	}
	
	if (!ReadFile(hFile, FileBuffer, FileSize, &FileSize, NULL))
	{
		VirtualFree(FileBuffer, 0, MEM_RELEASE);
		CloseHandle(hFile);
		return false;
	}
	CloseHandle(hFile);

	PIMAGE_NT_HEADERS pINH = RtlImageNtHeader(FileBuffer);
	if (pINH->Signature != IMAGE_NT_SIGNATURE)
	{
		VirtualFree(FileBuffer, 0, MEM_RELEASE);
		return false;
	}

	PVOID ImageBase;
	if ((ImageBase = VirtualAlloc(NULL, pINH->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)) == NULL)
	{
		VirtualFree(FileBuffer, 0, MEM_RELEASE);
		return false;
	}


	m_imagebasehdr = pINH->OptionalHeader.ImageBase;
	m_imagebase = ULONG_PTR(ImageBase);

	PIMAGE_SECTION_HEADER pISH = PIMAGE_SECTION_HEADER(pINH + 1);
	for (USHORT i = 0; i < pINH->FileHeader.NumberOfSections; ++i)
		memcpy(PUCHAR(ImageBase) + pISH[i].VirtualAddress, PUCHAR(FileBuffer) + pISH[i].PointerToRawData, pISH[i].SizeOfRawData);

	memcpy(ImageBase, FileBuffer, pINH->OptionalHeader.SizeOfHeaders);

	VirtualFree(FileBuffer, 0, MEM_RELEASE);
	return true;
}

