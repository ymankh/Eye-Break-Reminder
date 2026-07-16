#include "../core/reminder_core.h"

#include <assert.h>
#include <wchar.h>

int main(void)
{
    ReminderState state;
    wchar_t time_text[16];
    int index;

    ReminderCore_Initialize(&state);
    assert(!ReminderCore_IsBreakActive(&state));
    assert(ReminderCore_SecondsRemaining(&state) == REMINDER_INTERVAL_SECONDS);

    for (index = 0; index < REMINDER_INTERVAL_SECONDS; ++index)
    {
        ReminderEvent event = ReminderCore_Tick(&state);
        if (index + 1 == REMINDER_INTERVAL_SECONDS) assert(event == REMINDER_EVENT_BREAK_STARTED);
    }

    assert(ReminderCore_IsBreakActive(&state));
    assert(ReminderCore_SecondsRemaining(&state) == REMINDER_BREAK_SECONDS);

    for (index = 0; index < REMINDER_BREAK_SECONDS; ++index) ReminderCore_Tick(&state);

    assert(!ReminderCore_IsBreakActive(&state));
    assert(ReminderCore_SecondsRemaining(&state) == REMINDER_INTERVAL_SECONDS);
    ReminderCore_FormatTime(125, time_text, sizeof(time_text) / sizeof(time_text[0]));
    assert(wcscmp(time_text, L"02:05") == 0);
    return 0;
}
