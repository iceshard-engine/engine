#pragma once
#include <core/base.hxx>

namespace core
{

    struct Clock
    {
        int64_t previous_timestamp;
        int64_t latest_timestamp;
    };

    struct SystemClock : Clock { };

    struct CustomClock : Clock
    {
        Clock const* base_clock;
        float modifier;
    };

    struct Timer
    {
        Clock const* clock;
        int64_t step;
        int64_t last_tick_timestamp;
    };

    struct Timeline
    {
        Clock const* clock;
        int64_t initial_timestap;
    };

    struct Stopwatch
    {
        Clock const* clock;

        int64_t initial_timestamp;
        int64_t final_timestamp;
    };

    namespace clock
    {

        auto clock_frequency() noexcept -> float;

        auto create_clock() noexcept -> SystemClock;

        auto create_clock(Clock const& clock, float modifier) noexcept -> CustomClock;

        void update(SystemClock& c) noexcept;

        void update(CustomClock& c) noexcept;

        void update_max_delta(CustomClock& c, float max_elapsed_seconds);

        auto elapsed(Clock const& c) noexcept -> float;

    } // namespace clock

    namespace timer
    {

        auto create_timer(Clock const& clock, float step_seconds) noexcept -> Timer;

        bool update(Timer& timer) noexcept;

        bool update_by_step(Timer& timer) noexcept;

        auto elapsed(Timer const& timer) noexcept -> float;

        auto alpha(Timer const& timer) noexcept -> float;

    } // namespace timer

    namespace timeline
    {

        auto create_timeline(Clock const& clock, float speed = 1.0f) noexcept -> Timeline;

        auto elapsed(Timeline const& timeline) noexcept -> float;

    } // namespace timeline

    namespace stopwatch
    {

        auto create_stopwatch(Clock const& clock) noexcept;

        void start(Stopwatch& stopwatch) noexcept;

        void stop(Stopwatch& stopwatch) noexcept;

    } // namespace stopwatch


} // namespace core