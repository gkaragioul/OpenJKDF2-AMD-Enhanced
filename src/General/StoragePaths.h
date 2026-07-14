#ifndef OPENJKDF2_STORAGE_PATHS_H
#define OPENJKDF2_STORAGE_PATHS_H

#include <stdbool.h>
#include <stddef.h>

bool storage_paths_select_user_root(const char* explicit_root, bool portable,
                                    const char* executable_path, const char* local_app_data,
                                    char* output, size_t output_size);
bool storage_paths_build_legacy_command(int argc, const char* const* argv,
                                        char* output, size_t output_size);
bool storage_paths_build_crash_report_path(const char* diagnostics_root,
                                           char* output, size_t output_size);

#endif
