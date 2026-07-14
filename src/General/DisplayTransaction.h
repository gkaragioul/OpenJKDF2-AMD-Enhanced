#ifndef OPENJKDF2_DISPLAY_TRANSACTION_H
#define OPENJKDF2_DISPLAY_TRANSACTION_H

#include <stdbool.h>
#include <stdint.h>

#include "General/DisplayMode.h"

typedef struct DisplaySettings
{
    DisplayMode mode;
    int monitor;
    int width;
    int height;
    int refresh_hz;
    int hidpi;
} DisplaySettings;

typedef enum DisplayTransactionState
{
    DISPLAY_TRANSACTION_IDLE = 0,
    DISPLAY_TRANSACTION_PENDING,
    DISPLAY_TRANSACTION_COMMIT,
    DISPLAY_TRANSACTION_REVERT
} DisplayTransactionState;

typedef struct DisplayTransaction
{
    DisplaySettings original;
    DisplaySettings proposed;
    uint32_t started_ms;
    uint32_t timeout_ms;
    DisplayTransactionState state;
} DisplayTransaction;

bool display_settings_equal(DisplaySettings left, DisplaySettings right);
bool display_transaction_requires_confirmation(DisplaySettings original, DisplaySettings proposed);
void display_transaction_init(DisplayTransaction* transaction, DisplaySettings original);
void display_transaction_begin(DisplayTransaction* transaction, DisplaySettings proposed, uint32_t now_ms, uint32_t timeout_ms);
void display_transaction_confirm(DisplayTransaction* transaction);
void display_transaction_cancel(DisplayTransaction* transaction);
bool display_transaction_expire(DisplayTransaction* transaction, uint32_t now_ms);
uint32_t display_transaction_remaining_ms(const DisplayTransaction* transaction, uint32_t now_ms);
DisplayTransactionState display_transaction_state(const DisplayTransaction* transaction);
DisplaySettings display_transaction_result(const DisplayTransaction* transaction);

#endif
