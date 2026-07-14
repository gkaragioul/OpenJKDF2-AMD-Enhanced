#include "General/PathOverlay.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define make_dir(path) _mkdir(path)
#define change_dir(path) _chdir(path)
#define remove_dir(path) _rmdir(path)
#else
#include <sys/stat.h>
#include <unistd.h>
#define make_dir(path) mkdir(path, 0700)
#define change_dir(path) chdir(path)
#define remove_dir(path) rmdir(path)
#endif

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

static void write_marker(const char* path, const char* value)
{
    FILE* stream = fopen(path, "wb");
    CHECK(stream != NULL);
    if (stream) {
        fwrite(value, 1, strlen(value), stream);
        fclose(stream);
    }
}

int main(void)
{
    char original[1024];
    char temporary[1024];
    char asset_root[1024];
    char writable_root[1024];
    char resolved[2048];
    char absolute_read[2048];
    char outside_write[2048];

#ifdef _WIN32
    CHECK(GetCurrentDirectoryA(sizeof(original), original) != 0);
    CHECK(GetTempPathA(sizeof(temporary), temporary) != 0);
    snprintf(temporary + strlen(temporary), sizeof(temporary) - strlen(temporary), "OpenJKDF2 overlay %lu", (unsigned long)GetCurrentProcessId());
#else
    CHECK(getcwd(original, sizeof(original)) != NULL);
    snprintf(temporary, sizeof(temporary), "/tmp/OpenJKDF2 overlay %ld", (long)getpid());
#endif
    make_dir(temporary);
    CHECK(change_dir(temporary) == 0);
    make_dir("Read Only Assets");
    make_dir("Writable User Data");
    make_dir("Read Only Assets/episode");
    make_dir("Writable User Data/episode");
    write_marker("Read Only Assets/episode/base.gob", "asset");
    write_marker("Read Only Assets/episode/override.gob", "asset");
    write_marker("Writable User Data/episode/override.gob", "override");

#ifdef _WIN32
    CHECK(GetFullPathNameA("Read Only Assets", sizeof(asset_root), asset_root, NULL) != 0);
    CHECK(GetFullPathNameA("Writable User Data", sizeof(writable_root), writable_root, NULL) != 0);
#else
    CHECK(realpath("Read Only Assets", asset_root) != NULL);
    CHECK(realpath("Writable User Data", writable_root) != NULL);
#endif
    CHECK(path_overlay_configure(asset_root, writable_root));
    CHECK(strcmp(path_overlay_asset_root(), asset_root) == 0);
    CHECK(strcmp(path_overlay_writable_root(), writable_root) == 0);

    CHECK(path_overlay_resolve_read("episode/base.gob", resolved, sizeof(resolved)));
    CHECK(strstr(resolved, "Read Only Assets") != NULL);
    CHECK(path_overlay_resolve_read("episode\\override.gob", resolved, sizeof(resolved)));
    CHECK(strstr(resolved, "Writable User Data") != NULL);
    CHECK(!path_overlay_resolve_read("../outside.gob", resolved, sizeof(resolved)));

    snprintf(absolute_read, sizeof(absolute_read), "%s%c%s", asset_root,
#ifdef _WIN32
             '\\',
#else
             '/',
#endif
             "episode/base.gob");
    CHECK(path_overlay_resolve_read(absolute_read, resolved, sizeof(resolved)));
    CHECK(strcmp(resolved, absolute_read) == 0);

    CHECK(path_overlay_resolve_write("player/Test/save.jks", resolved, sizeof(resolved)));
    CHECK(strstr(resolved, "Writable User Data") != NULL);
    CHECK(!path_overlay_resolve_write("../escape.jks", resolved, sizeof(resolved)));
    snprintf(outside_write, sizeof(outside_write), "%s%c%s", asset_root,
#ifdef _WIN32
             '\\',
#else
             '/',
#endif
             "forbidden.txt");
    CHECK(!path_overlay_resolve_write(outside_write, resolved, sizeof(resolved)));
    CHECK(path_overlay_resolve_asset("resource/Res2.gob", resolved, sizeof(resolved)));
    CHECK(strstr(resolved, "Read Only Assets") != NULL);

    remove("Writable User Data/episode/override.gob");
    remove("Read Only Assets/episode/override.gob");
    remove("Read Only Assets/episode/base.gob");
    remove_dir("Writable User Data/episode");
    remove_dir("Read Only Assets/episode");
    remove_dir("Writable User Data");
    remove_dir("Read Only Assets");
    CHECK(change_dir(original) == 0);
    remove_dir(temporary);
    return failures ? 1 : 0;
}
