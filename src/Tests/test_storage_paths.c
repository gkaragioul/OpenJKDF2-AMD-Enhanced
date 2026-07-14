#include "General/StoragePaths.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

int main(void)
{
    char output[1024];
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
    return failures ? 1 : 0;
}
