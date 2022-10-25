#pragma once

class ÑSSDTDumper
{
	bool dump_table(const std::wstring& path, bool win32);
public:
	ÑSSDTDumper();
	~ÑSSDTDumper();

	bool dump();

	std::ofstream logfile;

	std::map<int, std::wstring> shadow_table;
	std::map<int, std::wstring> system_table;

	std::wstring sysroot;
	std::wstring curdir;

	std::wstring symsrv;
	std::wstring sympath;

	std::wstring winver;
	std::wstring outfile;

	std::wstring path_ntoskrnl;
	std::wstring path_win32k;

	bool dumping : 1;
};