/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>

namespace ice
{

    //! \brief Simple object representing a clock by using two timestamps.
    //!
    //! This type should be used usualy as a const reference pointing to a system or custom clock object.
    //!
    //! \see ice::SystemClock for how to access current time values.
    //! \see ice::CustomClock for how to create custom speed clocks.
    struct Clock
    {
        ice::i64 previous_timestamp;
        ice::i64 latest_timestamp;
    };

    //! \brief A system clock provides access to the actual time on the running system.
    //!
    //! Updating a system clock will always result in the latest timestamp value available in the Clock::latest_timestamp member.
    //!   This moves the previous Clock::latest_timestamp value to the Clock::previous_timestamp member.
    struct SystemClock : ice::Clock { };

    //! \brief A custom clock allows to control the "speed" of calculated time.
    //!
    //! This works by calculating the update values from the parent clock.
    //!   The parents current time difference is taken, and a modifier is applied.
    //!   The result of this operation is the stored under the inherited Clock::latest_timestamp member.
    //!   This moves the previous Clock::latest_timestamp value to the Clock::previous_timestamp member.
    //!
    //! \pre A Custom clock is only valid if the CustomClock::base_clock member is \b not-null.
    //! \remark The user is allowed to set the modifier to 0.0f or negative values, reversing time. <em>(whoaaaa...)</em>
    //!
    //! \note To have consistent time changes, udpate child clocks the exact same number of times like their parents.
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

        auto clock_frequency() noexcept -> ice::u64;

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

} // namespace ice
