#include "General/PathOverlay.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define PATH_OVERLAY_CAPACITY 1024

static char overlay_asset_root[PATH_OVERLAY_CAPACITY];
static char overlay_writable_root[PATH_OVERLAY_CAPACITY];

static bool overlay_is_separator(char value)
{
    return value == '/' || value == '\\';
}

static bool overlay_is_absolute(const char* path)
{
    if (!path || !path[0]) return false;
    if (overlay_is_separator(path[0])) return true;
    return isalpha((unsigned char)path[0]) && path[1] == ':' && overlay_is_separator(path[2]);
}

static bool overlay_has_traversal(const char* path)
{
    const char* cursor = path;
    while (cursor && *cursor) {
        const char* end;
        while (overlay_is_separator(*cursor)) ++cursor;
        end = cursor;
        while (*end && !overlay_is_separator(*end)) ++end;
        if ((size_t)(end - cursor) == 2 && cursor[0] == '.' && cursor[1] == '.') return true;
        cursor = end;
    }
    return false;
}

static bool overlay_copy_root(char* destination, const char* source)
{
    size_t length;
    if (!destination || !source || !source[0] || !overlay_is_absolute(source)) return false;
    length = strlen(source);
    if (length >= PATH_OVERLAY_CAPACITY) return false;
    while (length > 3 && overlay_is_separator(source[length - 1])) --length;
    memcpy(destination, source, length);
    destination[length] = '\0';
    return true;
}

static bool overlay_join(const char* root, const char* relative, char* output, size_t output_size)
{
    size_t root_length;
    int written;
    if (!root || !root[0] || !relative || !relative[0] || !output || !output_size) return false;
    while (relative[0] == '.' && overlay_is_separator(relative[1])) relative += 2;
    if (!relative[0] || overlay_is_absolute(relative) || overlay_has_traversal(relative)) return false;
    root_length = strlen(root);
    written = snprintf(output, output_size, "%s%c%s", root,
#ifdef _WIN32
                       '\\',
#else
                       '/',
#endif
                       relative);
    if (written < 0 || (size_t)written >= output_size) {
        if (output_size) output[0] = '\0';
        return false;
    }
    for (size_t index = root_length + 1; output[index]; ++index) {
#ifdef _WIN32
        if (output[index] == '/') output[index] = '\\';
#else
        if (output[index] == '\\') output[index] = '/';
#endif
    }
    return true;
}

static bool overlay_copy(const char* path, char* output, size_t output_size)
{
    size_t length = path ? strlen(path) : 0;
    if (!length || !output || length >= output_size) return false;
    memcpy(output, path, length + 1);
    return true;
}

static bool overlay_exists(const char* path)
{
    struct stat status;
    return path && stat(path, &status) == 0;
}

static bool overlay_is_below(const char* root, const char* path)
{
    size_t index;
    size_t root_length;
    if (!root || !path) return false;
    root_length = strlen(root);
    for (index = 0; index < root_length; ++index) {
#ifdef _WIN32
        if (tolower((unsigned char)root[index]) != tolower((unsigned char)path[index])) return false;
#else
        if (root[index] != path[index]) return false;
#endif
    }
    return path[root_length] == '\0' || overlay_is_separator(path[root_length]);
}

bool path_overlay_configure(const char* asset_root, const char* writable_root)
{
    char asset[PATH_OVERLAY_CAPACITY];
    char writable[PATH_OVERLAY_CAPACITY];
    if (!overlay_copy_root(asset, asset_root) || !overlay_copy_root(writable, writable_root)) return false;
    strcpy(overlay_asset_root, asset);
    strcpy(overlay_writable_root, writable);
    return true;
}

const char* path_overlay_asset_root(void) { return overlay_asset_root; }
const char* path_overlay_writable_root(void) { return overlay_writable_root; }

bool path_overlay_resolve_read(const char* path, char* output, size_t output_size)
{
    char candidate[PATH_OVERLAY_CAPACITY * 2];
    if (!path || !path[0]) return false;
    if (overlay_is_absolute(path)) return overlay_exists(path) && overlay_copy(path, output, output_size);
    if (!overlay_join(overlay_writable_root, path, candidate, sizeof(candidate))) return false;
    if (overlay_exists(candidate)) return overlay_copy(candidate, output, output_size);
    if (!overlay_join(overlay_asset_root, path, candidate, sizeof(candidate))) return false;
    return overlay_exists(candidate) && overlay_copy(candidate, output, output_size);
}

bool path_overlay_resolve_write(const char* path, char* output, size_t output_size)
{
    if (!path || !path[0]) return false;
    if (overlay_is_absolute(path)) {
        return overlay_is_below(overlay_writable_root, path) && overlay_copy(path, output, output_size);
    }
    return overlay_join(overlay_writable_root, path, output, output_size);
}

bool path_overlay_resolve_asset(const char* path, char* output, size_t output_size)
{
    if (!path || !path[0]) return false;
    if (overlay_is_absolute(path)) return overlay_copy(path, output, output_size);
    return overlay_join(overlay_asset_root, path, output, output_size);
}
