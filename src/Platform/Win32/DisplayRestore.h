#ifndef OPENJKDF2_DISPLAY_RESTORE_H
#define OPENJKDF2_DISPLAY_RESTORE_H

#include <stdbool.h>
#include <wchar.h>

bool display_restore_capture(const wchar_t* state_path);
bool display_restore_is_armed(const wchar_t* state_path, bool* armed);
bool display_restore_disarm(const wchar_t* state_path);
bool display_restore_apply_if_armed(const wchar_t* state_path, unsigned* restored_displays);
long display_restore_last_result(void);
long display_restore_topology_result(void);

#endif
