#include "stdafx.h"
#include "SSDTDumper.h"

int wmain(int argc, wchar_t* argv[])
{
    ÑSSDTDumper dumper;
    if (argc > 1)
    {
        for (int i = 1; i < argc; ++i)
        {
            if (!_wcsicmp(argv[i], L"-dump"))
            {
                dumper.dumping = true;
            }
            else if (!_wcsicmp(argv[i], L"-sympath"))
            {
                if (argc > i + 1)
                {
                    dumper.sympath.assign(argv[i + 1]);
                }
                else
                {
                    printf("sympath not defined\n");
                    return 0;
                }
            }
            else if (!_wcsicmp(argv[i], L"-symsrv"))
            {
                if (argc > i + 1)
                {
                    dumper.symsrv.assign(argv[i + 1]);
                }
                else
                {
                    printf("symsrv not defined\n");
                    return 0;
                }
            }
            else if (!_wcsicmp(argv[i], L"-ntoskrnl"))
            {
                if (argc > i + 1)
                {
                    dumper.path_ntoskrnl.assign(argv[i + 1]);
                }
                else
                {
                    printf("ntoskrnl not defined\n");
                    return 0;
                }
            }
            else if (!_wcsicmp(argv[i], L"-win32k"))
            {
                if (argc > i + 1)
                {
                    dumper.path_win32k.assign(argv[i + 1]);
                }
                else
                {
                    printf("win32k not defined\n");
                    return 0;
                }
            }
            else if (!_wcsicmp(argv[i], L"-winver"))
            {
                if (argc > i + 1)
                {
                    dumper.winver.assign(argv[i + 1]);
                }
                else
                {
                    printf("winver not defined\n");
                    return 0;
                }
            }
            else if (!_wcsicmp(argv[i], L"-outfile"))
            {
                if (argc > i + 1)
                {
                    dumper.outfile.assign(argv[i + 1]);
                }
                else
                {
                    printf("outfile not defined\n");
                    return 0;
                }
            }
        }
    }
    if (dumper.dumping)
    {
        if (dumper.dump())
        {

        }
        else
        {
            printf("failed\n");
        }
    }
    else
    {
        wprintf(L"usage: SSDTDumper.exe [ options ]\n");
        wprintf(L"  -?                : print this help\n");
        wprintf(L"  -dump             : dump tables\n");

        wprintf(L"  -sympath          : symbols path         [default: \"%ws\"]\n", dumper.sympath.c_str());
        wprintf(L"  -symsrv           : symbols servers      [default: \"%ws\"]\n", dumper.symsrv.c_str());

        wprintf(L"  -ntoskrnl         : path to ntoskrnl.exe [default: \"%ws\"]\n", dumper.path_ntoskrnl.c_str());
        wprintf(L"  -win32k           : path to win32k.sys   [default: \"%ws\"]\n", dumper.path_win32k.c_str());

        wprintf(L"  -winver           : winver info          [default: \"%ws\"]\n", dumper.winver.c_str());
        wprintf(L"  -outfile          : dump info to file    [default: \"%ws\"]\n", dumper.outfile.c_str());
    }
    return 0;
}
