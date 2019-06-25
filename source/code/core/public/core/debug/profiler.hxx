#pragma once
#include <string_view>
#include <chrono>

namespace core
{
namespace detail
{

//! Defines the maximum count of profiler contexts if the application
//! \note This value is constant because it's for debugging purposes only.
constexpr size_t profiler_context_count = 20;

//! Import some std commom types
using profiler_clock = std::chrono::steady_clock;

//! Defines a single profiler (thread) contexts.
struct profiler_context;

//! Defines a single profiler event;
struct profiler_event
{
    profiler_clock::time_point begin;
    profiler_clock::time_point end;
    int profiler_scope;
    int profiler_color; // Used for visualization
    char name[64];
    std::string_view file_name;
    int file_line;
};

using profiler_event_callback = void(*)(void*, int, const profiler_event&);

}

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

//! Creates a scoped profiler which stars profiling on construction and ends at destruction.
class Profiler
{
public:
    //! The profiler name shouldn't be a temporary.
    Profiler(std::string_view name) noexcept;

    //! The profiler name shouldn't be a temporary.
    Profiler(std::string_view name, std::string_view file, int line) noexcept;

    //! The profiler name shouldn't be a temporary.
    Profiler(std::string_view name, int color) noexcept;

    //! The profiler name shouldn't be a temporary.
    Profiler(std::string_view name, int color, std::string_view file, int line) noexcept;

    ~Profiler() noexcept;

private:
    const std::string_view _name;

    //! The profiler context.
    detail::profiler_context* _context;
};

}
