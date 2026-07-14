#ifndef OPENJKDF2_CONFIG_RECOVERY_H
#define OPENJKDF2_CONFIG_RECOVERY_H

#include <stdbool.h>

bool config_recovery_has_snapshot(const char* snapshot_path);
bool config_recovery_should_offer(bool previous_run_unclean, bool snapshot_exists, bool safe_mode_requested);
bool config_recovery_snapshot(const char* active_path, const char* snapshot_path);
bool config_recovery_restore(const char* snapshot_path, const char* active_path);

#endif
