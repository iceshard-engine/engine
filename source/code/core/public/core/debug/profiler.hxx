#pragma once
#include <core/base.hxx>
#include <chrono>

namespace core::debug
{


    namespace detail
    {

        //! \brief Number of available profiler contexts.
        //! \todo Make this value dynamic?
        constexpr size_t profiler_context_count = 20;


        //! \brief Declaration of a profiler context object.
        //! \remarks These objects are thread local states for tracking Profiler events.
        struct profiler_context;

        //! \brief The clock type used by the Profiler type
        using profiler_clock = std::chrono::steady_clock;


        //! \brief An single profiling event.
        struct profiler_event
        {
            //! \brief Event name.
            std::string_view event_name;

            //! \brief Start timestamp.
            profiler_clock::time_point event_begin;

            //! \brief End timestamp.
            profiler_clock::time_point event_end;

            //! \brief Scope level.
            int event_scope;

            //! \brief Custom user data.
            const void* user_data;
        };


        using profiler_event_callback = void(*)(void*, int, const profiler_event&);

    } // namespace detail


    //! Creates a new profiler context for the current thread.
    void initialize_profiling_context() noexcept;

    //! Releases the existing profiler context on the current thread.
    void release_profiling_context() noexcept;

    //! Clears the current profiler context from all captured events.
    void clear_profiling_context() noexcept;

    //! Saves all captured events on all initialized profiling contexts.
    void save_profiling_context() noexcept;

    //! Calls the given callback for each captured profiler event on the current thread context.
    void visit_profiling_context(void* data, detail::profiler_event_callback callback) noexcept(false);

    //! Saves all captured events on all initialized profiling contexts.
    void save_all_profiling_contexts() noexcept;

    //! Visits all captured events on all initialized profiling contexts.
    void visit_all_profiling_contexts(void* data, detail::profiler_event_callback callback) noexcept(false);


    //! \brief A scoped Profiler type.
    class Profiler
    {
    public:
        //! \brief New profiler with a name.
        Profiler(std::string_view name) noexcept;

        //! \brief New profiler with a name and user data.
        Profiler(std::string_view name, const void* user_data) noexcept;

        //! \brief Publishes the profiling resutls as an event.
        ~Profiler() noexcept;

    private:
        const std::string_view _name;

        //! \brief Current profiler context.
        detail::profiler_context* _context;
    };


} // namespace core::debug
