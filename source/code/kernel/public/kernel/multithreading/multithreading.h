#pragma once
#include "defines.h"

struct sp_criticalsection : protected sp_criticalsection_base
{
    void create();
    void destroy();

    void enter();
    void tryenter();
    void leave();
};

struct sp_criticalsection_spin : protected sp_criticalsection_base
{
    void create(int spin_count);
    void destroy();

    void set_spincount(int spin_count);

    void enter();
    void tryenter();
    void leave();
};

namespace sp_event_result
{
    enum TYPE {
        obj0 = WAIT_OBJECT_0 + 0,
        obj1 = WAIT_OBJECT_0 + 1,
        obj2 = WAIT_OBJECT_0 + 2,
        obj3 = WAIT_OBJECT_0 + 3,
        obj4 = WAIT_OBJECT_0 + 4,
        obj5 = WAIT_OBJECT_0 + 5,
        obj6 = WAIT_OBJECT_0 + 6,
        obj7 = WAIT_OBJECT_0 + 7,

        timeout = WAIT_TIMEOUT,
        error = WAIT_FAILED,
        ok = obj1
    };
}


struct sp_event : protected sp_event_base
{
    sp_event(bool autoreset, bool initial);
    ~sp_event();

    void signal();
    void reset();

    sp_event_result::TYPE wait(time_t time);

    friend struct sp_multievent;
};

struct sp_multievent : protected sp_multievent_base
{
    sp_multievent(sp_event** events, int count);
    ~sp_multievent();

    int count() const;

    void signalOne(int ev);
    void signalAll();

    void resetOne(int ev);
    void resetAll();

    sp_event_result::TYPE waitOne(int ev, time_t time);
    sp_event_result::TYPE waitAny(time_t time);
    sp_event_result::TYPE waitAll(time_t time);
};

struct sp_semaphore : protected sp_semaphore_base
{

};

struct sp_autosection
{
    sp_autosection(sp_criticalsection& section) : m_Section(section)
    {
        m_Section.enter();
    }
    ~sp_autosection()
    {
        m_Section.leave();
    }

private:
    sp_criticalsection& m_Section;
};

struct sp_cshelper
{
    /* todo */
};

class sp_thread : protected sp_thread_base
{
public:
    sp_thread();
    ~sp_thread();
    int getID() const;

    void create(sp_thread_proc proc, void* data);
    void terminate();
    void join();

    void set_name(const std::string& name);

    bool joinable() const;
};
