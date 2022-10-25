#pragma once
class CSymbols
{
public:
    CSymbols();
    ~CSymbols();

    virtual bool LoadFromPeFile(const std::wstring& path, const std::wstring& sympath, const std::wstring& symsrv);
    virtual bool LoadFromPdbFile(const std::wstring& path);

    virtual bool GetRvaByName(const std::wstring& name, PULONG rva);
    virtual bool GetNameByRva(ULONG rva, std::wstring& name);
private:
    virtual void LoadSymbols();
private:
    IDiaSession* m_session;
    IDiaSymbol* m_global;

    std::map<ULONG, std::wstring> m_names;
    std::map<std::wstring, ULONG> m_rva;
};