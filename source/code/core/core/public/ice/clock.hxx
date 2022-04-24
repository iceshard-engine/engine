#pragma once
#include <ice/base.hxx>

namespace ice
{

    struct Clock
    {
        ice::i64 previous_timestamp;
        ice::i64 latest_timestamp;
    };

    struct SystemClock : ice::Clock
    {
    };

    struct CustomClock : ice::Clock
    {
        ice::Clock const* base_clock;
        ice::f32 modifier;
    };

    struct Timer
    {
        ice::Clock const* clock;
        ice::i64 step;
        ice::i64 last_tick_timestamp;
    };

    struct Timeline
    {
        ice::Clock const* clock;
        ice::i64 initial_timestap;
    };

    struct Stopwatch
    {
        ice::Clock const* clock;

        ice::i64 initial_timestamp;
        ice::i64 final_timestamp;
    };

    namespace clock
    {

        auto clock_frequency() noexcept -> ice::f32;

        auto create_clock() noexcept -> ice::SystemClock;

        auto create_clock(
            ice::Clock const& clock,
            ice::f32 modifier
        ) noexcept -> ice::CustomClock;

        void update(ice::SystemClock& clock) noexcept;

        void update(ice::CustomClock& clock) noexcept;

        void update_max_delta(
            ice::CustomClock& clock,
            ice::f32 max_elapsed_seconds
        ) noexcept;

        auto elapsed(ice::Clock const& clock) noexcept -> ice::f32;

    } // namespace clock

    namespace timer
    {

        auto create_timer(
            ice::Clock const& clock,
            ice::f32 step_seconds
        ) noexcept -> ice::Timer;

        auto create_timer(
            ice::Clock const& clock,
            ice::f32 step_seconds,
            ice::i64 initial_timestamp
        ) noexcept -> ice::Timer;

        bool update(ice::Timer& timer) noexcept;

        bool update_by_step(ice::Timer& timer) noexcept;

        auto elapsed(ice::Timer const& timer) noexcept -> ice::f32;

        auto alpha(ice::Timer const& timer) noexcept -> ice::f32;

    } // namespace timer

    namespace timeline
    {

        auto create_timeline(ice::Clock const& clock) noexcept -> ice::Timeline;

        auto elapsed(ice::Timeline const& timeline) noexcept -> ice::f32;

    } // namespace timeline

    namespace stopwatch
    {

        auto create_stopwatch(ice::Clock const& clock) noexcept -> ice::Stopwatch;

        void start(ice::Stopwatch& stopwatch) noexcept;

        void stop(ice::Stopwatch& stopwatch) noexcept;

    } // namespace stopwatch


} // namespace core
