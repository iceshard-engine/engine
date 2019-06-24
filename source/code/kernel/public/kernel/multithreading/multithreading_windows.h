#pragma once

struct sp_criticalsection_base
{
    CRITICAL_SECTION m_Section;
};

struct sp_event_base
{
    HANDLE m_Event;
};

struct sp_multievent_base
{
    HANDLE* m_Events;
    int m_Count;
};

struct sp_semaphore_base
{
    /* todo */
};

typedef uint32_t (__stdcall *sp_thread_proc)(void*);
class sp_thread_base
{
protected:
    HANDLE m_Thread;
    DWORD m_ID;
};
