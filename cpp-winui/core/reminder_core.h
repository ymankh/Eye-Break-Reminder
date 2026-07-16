#ifndef BREAK_REMINDER_CORE_H
#define BREAK_REMINDER_CORE_H

#include <stddef.h>
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif

enum { REMINDER_INTERVAL_SECONDS = 20 * 60, REMINDER_BREAK_SECONDS = 20 };

typedef enum ReminderEvent
{
    REMINDER_EVENT_NONE = 0,
    REMINDER_EVENT_BREAK_STARTED,
    REMINDER_EVENT_BREAK_FINISHED
} ReminderEvent;

typedef struct ReminderState
{
    int seconds_until_break;
    int break_seconds_remaining;
    int break_active;
} ReminderState;

void ReminderCore_Initialize(ReminderState* state);
ReminderEvent ReminderCore_Tick(ReminderState* state);
void ReminderCore_StartBreak(ReminderState* state);
void ReminderCore_FinishBreak(ReminderState* state);
int ReminderCore_IsBreakActive(const ReminderState* state);
int ReminderCore_SecondsRemaining(const ReminderState* state);
double ReminderCore_ProgressPercent(const ReminderState* state);
void ReminderCore_FormatTime(int seconds, wchar_t* buffer, size_t buffer_count);

#ifdef __cplusplus
}
#endif

#endif
