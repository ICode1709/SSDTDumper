#pragma once
#include "Symbols.h"

class CPEFile
{
public:
	CPEFile();
	~CPEFile();

	virtual bool LoadFile(const std::wstring& path, const std::wstring& sympath, const std::wstring& symsrv);

	virtual ULONG_PTR GetImageBaseFromHeader() { return m_imagebasehdr; }
	virtual ULONG_PTR GetImageBase() { return m_imagebase; }
	virtual CSymbols* GetSymbol() { return &m_symbols; }

	template <typename type> type Read(ULONG VirtualAddress)
	{
		return *(type*)(m_imagebase + VirtualAddress);
	}

private:
	ULONG_PTR m_imagebasehdr;
	ULONG_PTR m_imagebase;
	PVOID m_file;

	CSymbols m_symbols;
};