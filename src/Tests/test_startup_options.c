#include "General/StartupOptions.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

static void test_defaults(void)
{
    const char* argv[] = { "openjkdf2" };
    StartupOptions options = startup_options_parse(1, argv);
    CHECK(!options.safe_mode);
    CHECK(!options.renderer_smoke_test);
    CHECK(!options.portable);
    CHECK(options.user_dir[0] == '\0');
    CHECK(options.error[0] == '\0');
}

static void test_storage_options_preserve_paths_with_spaces(void)
{
    const char* argv[] = {
        "openjkdf2", "--data-dir", "D:\\Steam Library\\Jedi Knight",
        "--user-dir", "C:\\Users\\Test User\\Saved Games\\OpenJKDF2",
        "--portable"
    };
    StartupOptions options = startup_options_parse(6, argv);
    CHECK(strcmp(options.data_dir, "D:\\Steam Library\\Jedi Knight") == 0);
    CHECK(strcmp(options.user_dir, "C:\\Users\\Test User\\Saved Games\\OpenJKDF2") == 0);
    CHECK(options.portable);
    CHECK(options.error[0] == '\0');
}

static void test_safe_mode(void)
{
    const char* argv[] = { "openjkdf2", "--safe-mode" };
    StartupOptions options = startup_options_parse(2, argv);
    CHECK(options.safe_mode);
    CHECK(options.force_windowed);
    CHECK(options.frame_limit == 60);
    CHECK(options.conservative_renderer);
}

static void test_paths_and_last_wins(void)
{
    const char* argv[] = {
        "openjkdf2", "--data-dir", "first", "-path", "legacy",
        "--data-dir", "final", "--diagnostics-dir", "diagnostics"
    };
    StartupOptions options = startup_options_parse(9, argv);
    CHECK(strcmp(options.data_dir, "final") == 0);
    CHECK(strcmp(options.diagnostics_dir, "diagnostics") == 0);
    CHECK(options.error[0] == '\0');
}

static void test_errors_and_passthrough(void)
{
    const char* missing[] = { "openjkdf2", "--data-dir" };
    const char* unknown[] = { "openjkdf2", "-devmode", "--unknown" };
    StartupOptions bad = startup_options_parse(2, missing);
    StartupOptions good = startup_options_parse(3, unknown);
    CHECK(strstr(bad.error, "--data-dir") != NULL);
    CHECK(good.error[0] == '\0');
}

int main(void)
{
    test_defaults();
    test_safe_mode();
    test_storage_options_preserve_paths_with_spaces();
    test_paths_and_last_wins();
    test_errors_and_passthrough();
    return failures ? 1 : 0;
}
