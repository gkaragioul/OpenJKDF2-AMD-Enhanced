#ifndef OPENJKDF2_PATH_OVERLAY_H
#define OPENJKDF2_PATH_OVERLAY_H

#include <stdbool.h>
#include <stddef.h>

bool path_overlay_configure(const char* asset_root, const char* writable_root);
const char* path_overlay_asset_root(void);
const char* path_overlay_writable_root(void);
bool path_overlay_resolve_read(const char* path, char* output, size_t output_size);
bool path_overlay_resolve_write(const char* path, char* output, size_t output_size);
bool path_overlay_resolve_asset(const char* path, char* output, size_t output_size);

#endif
