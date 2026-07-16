#include "reminder_core.h"

void ReminderCore_Initialize(ReminderState* state)
{
    if (state == NULL) return;
    state->seconds_until_break = REMINDER_INTERVAL_SECONDS;
    state->break_seconds_remaining = 0;
    state->break_active = 0;
}

ReminderEvent ReminderCore_Tick(ReminderState* state)
{
    if (state == NULL) return REMINDER_EVENT_NONE;

    if (state->break_active)
    {
        if (state->break_seconds_remaining > 0) --state->break_seconds_remaining;
        if (state->break_seconds_remaining <= 0)
        {
            ReminderCore_FinishBreak(state);
            return REMINDER_EVENT_BREAK_FINISHED;
        }
        return REMINDER_EVENT_NONE;
    }

    if (state->seconds_until_break > 0) --state->seconds_until_break;
    if (state->seconds_until_break <= 0)
    {
        ReminderCore_StartBreak(state);
        return REMINDER_EVENT_BREAK_STARTED;
    }
    return REMINDER_EVENT_NONE;
}

void ReminderCore_StartBreak(ReminderState* state)
{
    if (state == NULL) return;
    state->break_active = 1;
    state->break_seconds_remaining = REMINDER_BREAK_SECONDS;
}

void ReminderCore_FinishBreak(ReminderState* state)
{
    if (state == NULL) return;
    state->break_active = 0;
    state->break_seconds_remaining = 0;
    state->seconds_until_break = REMINDER_INTERVAL_SECONDS;
}

int ReminderCore_IsBreakActive(const ReminderState* state)
{
    return state != NULL && state->break_active;
}

int ReminderCore_SecondsRemaining(const ReminderState* state)
{
    if (state == NULL) return 0;
    return state->break_active ? state->break_seconds_remaining : state->seconds_until_break;
}

double ReminderCore_ProgressPercent(const ReminderState* state)
{
    int total;
    int remaining;
    if (state == NULL) return 0.0;
    total = state->break_active ? REMINDER_BREAK_SECONDS : REMINDER_INTERVAL_SECONDS;
    remaining = ReminderCore_SecondsRemaining(state);
    return 100.0 * (double)(total - remaining) / (double)total;
}

void ReminderCore_FormatTime(int seconds, wchar_t* buffer, size_t buffer_count)
{
    if (buffer == NULL || buffer_count == 0) return;
    if (seconds < 0) seconds = 0;
#if defined(_MSC_VER)
    _snwprintf_s(buffer, buffer_count, _TRUNCATE, L"%02d:%02d", seconds / 60, seconds % 60);
#else
    _snwprintf(buffer, buffer_count, L"%02d:%02d", seconds / 60, seconds % 60);
#endif
    buffer[buffer_count - 1] = L'\0';
}
