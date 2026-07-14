#include <stdio.h>
#include <windows.h>

typedef void (APIENTRY *CrashHandlerInitialize)(void);
typedef BOOL (APIENTRY *CrashHandlerSetReportPath)(const char* path);

int main(int argc, char** argv)
{
    HMODULE library;
    CrashHandlerInitialize initialize;
    CrashHandlerSetReportPath set_report_path;

    if (argc != 2 || !argv[1][0])
    {
        fprintf(stderr, "Usage: openjkdf2-crash-probe.exe <report-path>\n");
        return 2;
    }

    library = LoadLibraryA("exchndl.dll");
    if (!library)
    {
        fprintf(stderr, "Unable to load exchndl.dll (error %lu)\n", GetLastError());
        return 3;
    }
    initialize = (CrashHandlerInitialize)GetProcAddress(library, "ExcHndlInit");
    set_report_path = (CrashHandlerSetReportPath)GetProcAddress(library, "ExcHndlSetLogFileNameA");
    if (!initialize || !set_report_path)
    {
        fprintf(stderr, "Required DrMinGW exports are unavailable\n");
        return 4;
    }

    initialize();
    if (!set_report_path(argv[1]))
    {
        fprintf(stderr, "Unable to set crash-report path\n");
        return 5;
    }
    fflush(NULL);
    RaiseException(EXCEPTION_ACCESS_VIOLATION, 0, 0, NULL);
    return 6;
}
