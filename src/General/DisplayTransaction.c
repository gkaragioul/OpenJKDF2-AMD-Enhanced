#include "General/DisplayTransaction.h"

#include <string.h>

bool display_settings_equal(DisplaySettings left, DisplaySettings right)
{
    return left.mode == right.mode &&
           left.monitor == right.monitor &&
           left.width == right.width &&
           left.height == right.height &&
           left.refresh_hz == right.refresh_hz &&
           left.hidpi == right.hidpi;
}

bool display_transaction_requires_confirmation(DisplaySettings original, DisplaySettings proposed)
{
    return !display_settings_equal(original, proposed);
}

void display_transaction_init(DisplayTransaction* transaction, DisplaySettings original)
{
    memset(transaction, 0, sizeof(*transaction));
    transaction->original = original;
    transaction->proposed = original;
    transaction->state = DISPLAY_TRANSACTION_IDLE;
}

void display_transaction_begin(DisplayTransaction* transaction, DisplaySettings proposed, uint32_t now_ms, uint32_t timeout_ms)
{
    transaction->proposed = proposed;
    transaction->started_ms = now_ms;
    transaction->timeout_ms = timeout_ms;
    transaction->state = DISPLAY_TRANSACTION_PENDING;
}

void display_transaction_confirm(DisplayTransaction* transaction)
{
    if (transaction->state == DISPLAY_TRANSACTION_PENDING)
        transaction->state = DISPLAY_TRANSACTION_COMMIT;
}

void display_transaction_cancel(DisplayTransaction* transaction)
{
    if (transaction->state == DISPLAY_TRANSACTION_PENDING)
        transaction->state = DISPLAY_TRANSACTION_REVERT;
}

uint32_t display_transaction_remaining_ms(const DisplayTransaction* transaction, uint32_t now_ms)
{
    uint32_t elapsed;
    if (transaction->state != DISPLAY_TRANSACTION_PENDING)
        return 0;
    elapsed = now_ms - transaction->started_ms;
    return elapsed < transaction->timeout_ms ? transaction->timeout_ms - elapsed : 0;
}

bool display_transaction_expire(DisplayTransaction* transaction, uint32_t now_ms)
{
    if (transaction->state != DISPLAY_TRANSACTION_PENDING ||
        display_transaction_remaining_ms(transaction, now_ms) != 0)
    {
        return false;
    }
    transaction->state = DISPLAY_TRANSACTION_REVERT;
    return true;
}

DisplayTransactionState display_transaction_state(const DisplayTransaction* transaction)
{
    return transaction->state;
}

DisplaySettings display_transaction_result(const DisplayTransaction* transaction)
{
    return transaction->state == DISPLAY_TRANSACTION_COMMIT ? transaction->proposed : transaction->original;
}
