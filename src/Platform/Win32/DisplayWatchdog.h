#ifndef OPENJKDF2_DISPLAY_WATCHDOG_H
#define OPENJKDF2_DISPLAY_WATCHDOG_H

#include <stdbool.h>

bool display_watchdog_start(const char* diagnostics_dir);
bool display_watchdog_disarm(void);
const char* display_watchdog_status(void);
long display_watchdog_topology_result(void);
long display_watchdog_display_result(void);

#endif
