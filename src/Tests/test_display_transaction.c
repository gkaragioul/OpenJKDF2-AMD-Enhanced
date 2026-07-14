#include "General/DisplayTransaction.h"

#include <stdio.h>

static int failures;
#define CHECK(value) do { if (!(value)) { fprintf(stderr, "line %d: %s\n", __LINE__, #value); ++failures; } } while (0)

static DisplaySettings settings(DisplayMode mode, int monitor, int width, int height, int refresh, int hidpi)
{
    DisplaySettings value = {mode, monitor, width, height, refresh, hidpi};
    return value;
}

int main(void)
{
    const DisplaySettings original = settings(DISPLAY_MODE_WINDOWED, 0, 1280, 720, 60, 0);
    const DisplaySettings borderless = settings(DISPLAY_MODE_BORDERLESS, 0, 2560, 1440, 144, 1);
    DisplayTransaction transaction;

    display_transaction_init(&transaction, original);
    CHECK(display_transaction_state(&transaction) == DISPLAY_TRANSACTION_IDLE);
    CHECK(!display_transaction_requires_confirmation(original, original));
    CHECK(display_transaction_requires_confirmation(original, borderless));

    display_transaction_begin(&transaction, borderless, 1000, 15000);
    CHECK(display_transaction_state(&transaction) == DISPLAY_TRANSACTION_PENDING);
    CHECK(display_transaction_remaining_ms(&transaction, 1000) == 15000);
    CHECK(display_transaction_remaining_ms(&transaction, 15999) == 1);
    CHECK(!display_transaction_expire(&transaction, 15999));
    CHECK(display_transaction_expire(&transaction, 16000));
    CHECK(display_transaction_state(&transaction) == DISPLAY_TRANSACTION_REVERT);
    CHECK(display_settings_equal(display_transaction_result(&transaction), original));

    display_transaction_init(&transaction, original);
    display_transaction_begin(&transaction, borderless, 0xFFFFFFF0u, 32);
    CHECK(display_transaction_remaining_ms(&transaction, 0x00000000u) == 16);
    CHECK(display_transaction_expire(&transaction, 0x00000010u));

    display_transaction_init(&transaction, original);
    display_transaction_begin(&transaction, borderless, 500, 15000);
    display_transaction_confirm(&transaction);
    CHECK(display_transaction_state(&transaction) == DISPLAY_TRANSACTION_COMMIT);
    CHECK(display_settings_equal(display_transaction_result(&transaction), borderless));

    display_transaction_init(&transaction, original);
    display_transaction_begin(&transaction, borderless, 500, 15000);
    display_transaction_cancel(&transaction);
    CHECK(display_transaction_state(&transaction) == DISPLAY_TRANSACTION_REVERT);
    CHECK(display_settings_equal(display_transaction_result(&transaction), original));

    CHECK(display_transaction_requires_confirmation(original,
        settings(original.mode, 1, original.width, original.height, original.refresh_hz, original.hidpi)));
    CHECK(display_transaction_requires_confirmation(original,
        settings(original.mode, original.monitor, 1920, original.height, original.refresh_hz, original.hidpi)));
    CHECK(display_transaction_requires_confirmation(original,
        settings(original.mode, original.monitor, original.width, original.height, 120, original.hidpi)));
    CHECK(display_transaction_requires_confirmation(original,
        settings(original.mode, original.monitor, original.width, original.height, original.refresh_hz, 1)));

    return failures ? 1 : 0;
}
