#include "stdafx.h"
#include "Symbols.h"

/*
std::wstring CSymbols::m_sympath = std::filesystem::current_path().wstring() + L"\\Symbols";
std::wstring CSymbols::m_symsrv = L"http://msdl.microsoft.com/download/symbols";

std::wstring CSymbols::m_sympath = L"srv*" + std::filesystem::current_path().wstring() + L"\\Symbols" + L"*http://msdl.microsoft.com/download/symbols";
*/

std::wstring GetSymPath(const std::wstring& sympath, const std::wstring& symsrv)
{
    return L"srv*" + sympath + L"*" + symsrv;
}

CSymbols::CSymbols()
{
    m_session = 0;
    m_global = 0;
}
CSymbols::~CSymbols()
{
    if (m_global)
        m_global->Release();

    if (m_session)
        m_session->Release();
}
/*
void CSymbols::SetSymbolPath(const std::wstring& path)
{
    m_sympath.assign(L"srv*" + path + L"*http://msdl.microsoft.com/download/symbols");
}
*/
bool CSymbols::LoadFromPeFile(const std::wstring& path, const std::wstring& sympath, const std::wstring& symsrv)
{
    IDiaDataSource* source = 0;
    if (SUCCEEDED(NoRegCoCreate(L"msdia140.dll", CLSID_DiaSource, IID_IDiaDataSource, (void**)&source)))
    {
        if (SUCCEEDED(source->loadDataForExe(path.c_str(), GetSymPath(sympath, symsrv).c_str(), NULL)))
        {
            if (SUCCEEDED(source->openSession(&m_session)))
            {
                if (SUCCEEDED(m_session->get_globalScope(&m_global)))
                {
                    source->Release();
                    LoadSymbols();
                    return true;
                }
                m_session->Release();
            }
        }
        source->Release();
    }
    return false;
}
bool CSymbols::LoadFromPdbFile(const std::wstring& path)
{
    IDiaDataSource* source = 0;
    if (SUCCEEDED(NoRegCoCreate(L"msdia140.dll", CLSID_DiaSource, IID_IDiaDataSource, (void**)&source)))
    {
        if (SUCCEEDED(source->loadDataFromPdb(path.c_str())))
        {
            if (SUCCEEDED(source->openSession(&m_session)))
            {
                if (SUCCEEDED(m_session->get_globalScope(&m_global)))
                {
                    source->Release();
                    LoadSymbols();
                    return true;
                }
                m_session->Release();
            }
        }
        source->Release();
    }
    return false;
}

void CSymbols::LoadSymbols()
{
    IDiaEnumSymbols* symbols = 0;
    if (SUCCEEDED(m_global->findChildren(SymTagNull, nullptr, nsNone, &symbols)))
    {
        int index = 0;
        ULONG count = 0;
        IDiaSymbol* symbol = 0;
        while (SUCCEEDED(symbols->Next(1, &symbol, &count)) && count != 0)
        {
            ULONG rva = 0;
            BSTR symname = 0;
            if (SUCCEEDED(symbol->get_relativeVirtualAddress(&rva)) && SUCCEEDED(symbol->get_undecoratedName(&symname)) && symname)
            {
                m_names[rva].assign(symname);
                m_rva[symname] = rva;
            }
            symbol->Release();
        }
        symbols->Release();
    }
}

bool CSymbols::GetRvaByName(const std::wstring& name, PULONG rva)
{
    auto it = m_rva.find(name);
    if (it == m_rva.end())
        return false;

    *rva = it->second;
    return true;
}
bool CSymbols::GetNameByRva(ULONG rva, std::wstring& name)
{
    auto it = m_names.find(rva);
    if (it == m_names.end())
        return false;

    name.assign(it->second);
    return true;
}