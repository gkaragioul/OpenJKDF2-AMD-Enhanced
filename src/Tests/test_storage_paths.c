#include "General/StoragePaths.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

int main(void)
{
    char output[1024];
    char small[16];
    const char* argv[] = {
        "openjkdf2.exe", "--data-dir", "D:\\Steam Library\\Jedi Knight",
        "-autostart", "-sp", "--user-dir", "C:\\Users\\Test User\\OpenJKDF2",
        "--diagnostics-dir", "diagnostics with spaces", "--portable",
        "-episode", "JK1"
    };

    CHECK(storage_paths_select_user_root("C:\\Custom User Root", false,
                                         "E:\\Games\\OpenJKDF2\\openjkdf2.exe",
                                         "C:\\Users\\Test\\AppData\\Local", output, sizeof(output)));
    CHECK(strcmp(output, "C:\\Custom User Root") == 0);
    CHECK(storage_paths_select_user_root(NULL, true,
                                         "E:\\Games\\OpenJKDF2\\openjkdf2.exe",
                                         "C:\\Users\\Test\\AppData\\Local", output, sizeof(output)));
    CHECK(strcmp(output, "E:\\Games\\OpenJKDF2\\UserData") == 0);
    CHECK(storage_paths_select_user_root(NULL, false,
                                         "E:\\Games\\OpenJKDF2\\openjkdf2.exe",
                                         "C:\\Users\\Test\\AppData\\Local", output, sizeof(output)));
    CHECK(strcmp(output, "C:\\Users\\Test\\AppData\\Local\\OpenJKDF2 AMD Enhanced") == 0);
    CHECK(!storage_paths_select_user_root(NULL, false, "openjkdf2.exe", NULL, output, sizeof(output)));

    CHECK(storage_paths_build_legacy_command(12, argv, output, sizeof(output)));
    CHECK(strcmp(output, "-autostart -sp -episode JK1") == 0);
    CHECK(strstr(output, "Steam Library") == NULL);
    CHECK(strstr(output, "Test User") == NULL);
    CHECK(strstr(output, "diagnostics with spaces") == NULL);
    {
        const char* legacy[] = { "openjkdf2.exe", "-path", "MyMod" };
        CHECK(storage_paths_build_legacy_command(3, legacy, output, sizeof(output)));
        CHECK(strcmp(output, "-path MyMod") == 0);
    }
    {
        const char* validation[] = {
            "openjkdf2.exe", "--validation-observer=timing-domains",
            "--frame-limit", "60", "-autostart", "-sp"
        };
        CHECK(storage_paths_build_legacy_command(6, validation, output, sizeof(output)));
        CHECK(strcmp(output, "-autostart -sp") == 0);
    }
    {
        const char* validation[] = {
            "openjkdf2.exe", "--validation-observer", "timing-domains",
            "--frame-limit=120", "-autostart"
        };
        CHECK(storage_paths_build_legacy_command(5, validation, output, sizeof(output)));
        CHECK(strcmp(output, "-autostart") == 0);
    }
    CHECK(storage_paths_build_crash_report_path("diagnostics", output, sizeof(output)));
#ifdef _WIN32
    CHECK(strcmp(output, "diagnostics\\OpenJKDF2-crash.RPT") == 0);
#else
    CHECK(strcmp(output, "diagnostics/OpenJKDF2-crash.RPT") == 0);
#endif
    CHECK(storage_paths_build_crash_report_path("diagnostics/", output, sizeof(output)));
    CHECK(strstr(output, "//OpenJKDF2-crash.RPT") == NULL);
    CHECK(strstr(output, "\\\\OpenJKDF2-crash.RPT") == NULL);
    CHECK(!storage_paths_build_crash_report_path(NULL, output, sizeof(output)));
    CHECK(!storage_paths_build_crash_report_path("diagnostics", small, sizeof(small)));
    return failures ? 1 : 0;
}
