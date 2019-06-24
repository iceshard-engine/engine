#include <kernel/multithreading/multithreading.h>
#include <kernel/utils/scope_exit.h>

#include <memsys/memsys.h>

#include <ctime>
#include <cassert>

void sp_criticalsection::create()
{
    InitializeCriticalSection(&m_Section);
}

void sp_criticalsection::destroy()
{
    DeleteCriticalSection(&m_Section);
}

void sp_criticalsection::enter()
{
    EnterCriticalSection(&m_Section);
}

void sp_criticalsection::tryenter()
{
    TryEnterCriticalSection(&m_Section);
}

void sp_criticalsection::leave()
{
    LeaveCriticalSection(&m_Section);
}

void sp_criticalsection_spin::create(int spin_count)
{
    InitializeCriticalSectionAndSpinCount(&m_Section, spin_count);
}

void sp_criticalsection_spin::destroy()
{
    DeleteCriticalSection(&m_Section);
}

void sp_criticalsection_spin::set_spincount(int spin_count)
{
    SetCriticalSectionSpinCount(&m_Section, spin_count);
}

void sp_criticalsection_spin::enter()
{
    EnterCriticalSection(&m_Section);
}

void sp_criticalsection_spin::tryenter()
{
    TryEnterCriticalSection(&m_Section);
}

void sp_criticalsection_spin::leave()
{
    LeaveCriticalSection(&m_Section);
}

sp_event::sp_event(bool autoreset, bool initial)
{
    m_Event = CreateEvent(nullptr, autoreset, initial, nullptr);
}

sp_event::~sp_event()
{
    CloseHandle(m_Event);
}

void sp_event::signal()
{
    SetEvent(m_Event);
}

void sp_event::reset()
{
    ResetEvent(m_Event);
}

sp_event_result::TYPE sp_event::wait(time_t time)
{
    return static_cast<sp_event_result::TYPE>(WaitForSingleObject(m_Event, static_cast<DWORD>(time)));
}

sp_multievent::sp_multievent(sp_event** events, int count)
{
    m_Count = count;
    m_Events = reinterpret_cast<HANDLE*>(mem::globals::default_allocator().allocate(sizeof(HANDLE) * count));

    for (int i = 0; i < count; ++i)
    {
        m_Events[i] = events[i]->m_Event;
    }
}

sp_multievent::~sp_multievent()
{
    mem::globals::default_allocator().deallocate(m_Events);
    m_Count = 0;
}

int sp_multievent::count() const
{
    return m_Count;
}

void sp_multievent::signalOne(int ev)
{
    SetEvent(m_Events[ev]);
}

void sp_multievent::signalAll()
{
    for (int i = 0; i < m_Count; ++i)
    {
        SetEvent(m_Events[i]);
    }
}

void sp_multievent::resetOne(int ev)
{
    ResetEvent(m_Events[ev]);
}

void sp_multievent::resetAll()
{
    for (int i = 0; i < m_Count; ++i)
    {
        ResetEvent(m_Events[i]);
    }
}

sp_event_result::TYPE sp_multievent::waitOne(int ev, time_t time)
{
    return static_cast<sp_event_result::TYPE>(WaitForSingleObject(m_Events[ev], static_cast<DWORD>(time)));
}

sp_event_result::TYPE sp_multievent::waitAny(time_t time)
{
    return static_cast<sp_event_result::TYPE>(WaitForMultipleObjects(m_Count, m_Events, false, static_cast<DWORD>(time)));
}

sp_event_result::TYPE sp_multievent::waitAll(time_t time)
{
    return static_cast<sp_event_result::TYPE>(WaitForMultipleObjects(m_Count, m_Events, true, static_cast<DWORD>(time)));
}

sp_thread::sp_thread()
{
}

sp_thread::~sp_thread()
{
    CloseHandle(m_Thread);
}

void sp_thread::create(sp_thread_proc proc, void* data)
{
    m_Thread = CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE) proc, data, 0, static_cast<DWORD*>(&m_ID));
    assert(0 != m_ID);
}

void sp_thread::join()
{
    WaitForSingleObject(m_Thread, INFINITE);
    m_ID = 0;
}

void sp_thread::terminate()
{
    TerminateThread(m_Thread, 0xffff);
    m_ID = 0;
}

void sp_thread::set_name(const std::string& name)
{
    static const DWORD MS_VC_EXCEPTION = 0x406D1388;
#pragma pack(push, 8)
    struct
    {
        DWORD type;
        LPCSTR name;
        DWORD thread_id;
        DWORD flags;
    } threadname_info = { 0x1000, name.c_str(), m_ID, 0 };
#pragma pack(pop)

#pragma warning(push)
#pragma warning(disable: 6320 6322)
    __try {
        RaiseException(MS_VC_EXCEPTION, 0, sizeof(threadname_info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&threadname_info));
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
    }
#pragma warning(pop)
}

bool sp_thread::joinable() const
{
    return m_ID != -1;
}

int sp_thread::getID() const
{
    return m_ID;
}
//
//static DWORD WINAPI NativeThreadProc(LPVOID data)
//{
//    static_cast<IThread*>(data)->run();
//    return 0;
//}
//
//CThread::CThread() : m_Handle(nullptr), m_ID(-1)
//{
//}
//
//CThread::~CThread()
//{
//    if (m_Handle != nullptr)
//    {
//        CloseHandle(m_Handle);
//    }
//}
//
//void CThread::start()
//{
//    m_Handle = CreateThread(nullptr, 0, &NativeThreadProc, this, 0, &m_ID);
//}
//
//void CThread::join()
//{
//    WaitForSingleObject(m_Handle, INFINITE);
//}
//
//void CThread::terminate()
//{
//    TerminateThread(m_Handle, 0xffff);
//}
//
//void CThread::setName(const ttl::string& name)
//{
//    static const DWORD MS_VC_EXCEPTION = 0x406D1388;
//#pragma pack(push, 8)
//    struct
//    {
//        DWORD type;
//        LPCSTR name;
//        DWORD thread_id;
//        DWORD flags;
//    } threadname_info = { 0x1000, name.c_str(), m_ID, 0 };
//#pragma pack(pop)
//
//#pragma warning(push)
//#pragma warning(disable: 6320 6322)
//    __try {
//        RaiseException(MS_VC_EXCEPTION, 0, sizeof(threadname_info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&threadname_info));
//    }
//    __except (EXCEPTION_EXECUTE_HANDLER) {
//    }
//#pragma warning(pop)
//}
//
//unsigned CThread::getID()
//{
//    return m_ID;
//}

#include <chrono>

void get_hires_timestamp(hires_timestamp& timestamp)
{
    //static LARGE_INTEGER frequency = { 0, 0 };
    //if (frequency.QuadPart == 0) QueryPerformanceFrequency((LARGE_INTEGER*)&frequency);

    //LARGE_INTEGER time;
    //QueryPerformanceCounter(&time);

    timestamp.milis = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    timestamp.seconds = ::time(nullptr);
}
