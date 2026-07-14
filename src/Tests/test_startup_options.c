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
    CHECK(options.validation_observer == STARTUP_VALIDATION_NONE);
    CHECK(options.user_dir[0] == '\0');
    CHECK(options.error[0] == '\0');
}

static void test_timing_observer_equals_form(void)
{
    const char* argv[] = { "openjkdf2", "--validation-observer=timing-domains" };
    StartupOptions options = startup_options_parse(2, argv);
    CHECK(options.validation_observer == STARTUP_VALIDATION_TIMING_DOMAINS);
    CHECK(options.error[0] == '\0');
}

static void test_timing_observer_separate_form(void)
{
    const char* argv[] = { "openjkdf2", "--validation-observer", "timing-domains" };
    StartupOptions options = startup_options_parse(3, argv);
    CHECK(options.validation_observer == STARTUP_VALIDATION_TIMING_DOMAINS);
    CHECK(options.error[0] == '\0');
}

static void test_timing_observer_rejects_missing_and_unknown_values(void)
{
    const char* missing[] = { "openjkdf2", "--validation-observer" };
    const char* unknown[] = { "openjkdf2", "--validation-observer=unknown" };
    StartupOptions missing_options = startup_options_parse(2, missing);
    StartupOptions unknown_options = startup_options_parse(2, unknown);
    CHECK(strstr(missing_options.error, "validation-observer") != NULL);
    CHECK(strstr(unknown_options.error, "validation-observer") != NULL);
}

static void test_frame_limit_for_validation(void)
{
    const char* sixty[] = { "openjkdf2", "--frame-limit", "60" };
    const char* one_twenty[] = { "openjkdf2", "--frame-limit=120" };
    const char* invalid[] = { "openjkdf2", "--frame-limit", "144" };
    StartupOptions sixty_options = startup_options_parse(3, sixty);
    StartupOptions one_twenty_options = startup_options_parse(2, one_twenty);
    StartupOptions invalid_options = startup_options_parse(3, invalid);
    CHECK(sixty_options.frame_limit == 60 && sixty_options.error[0] == '\0');
    CHECK(one_twenty_options.frame_limit == 120 && one_twenty_options.error[0] == '\0');
    CHECK(strstr(invalid_options.error, "frame-limit") != NULL);
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

static void test_legacy_mod_path_is_not_a_data_directory(void)
{
    const char* argv[] = { "openjkdf2", "-path", "My Mod" };
    StartupOptions options = startup_options_parse(3, argv);
    CHECK(options.data_dir[0] == '\0');
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
    test_legacy_mod_path_is_not_a_data_directory();
    test_errors_and_passthrough();
    test_timing_observer_equals_form();
    test_timing_observer_separate_form();
    test_timing_observer_rejects_missing_and_unknown_values();
    test_frame_limit_for_validation();
    return failures ? 1 : 0;
}
