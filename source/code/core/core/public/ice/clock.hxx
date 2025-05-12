/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/clock_types.hxx>

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
        ice::Timestamp _ts_previous;
        ice::Timestamp _ts_latest;
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
    //! \pre A Custom clock is only valid if the CustomClock::_clock_base member is \b not-null.
    //! \remark The user is allowed to set the modifier to 0.0f or negative values, reversing time. <em>(whoaaaa...)</em>
    //!
    //! \note To have consistent time changes, udpate child clocks the exact same number of times like their parents.
    struct CustomClock : ice::Clock
    {
        ice::Clock const* _clock_base;
        ice::f32 modifier;
    };

    struct Timer
    {
        ice::Clock const* _clock_base;
        ice::Timestamp _timer_step;
        ice::Timestamp _ts_latest;
    };

    struct Timeline
    {
        ice::Clock const* _clock_base;
        ice::Timestamp _ts_initial;
    };

    struct Stopwatch
    {
        ice::Clock const* clock;

        ice::Timestamp _ts_initial;
        ice::Timestamp _ts_final;
    };

    namespace clock
    {

        auto now() noexcept -> ice::Timestamp;

        auto clock_frequency() noexcept -> ice::ClockFrequency;

        auto create_clock() noexcept -> ice::SystemClock;

        auto create_clock(
            ice::Clock const& clock,
            ice::f32 modifier
        ) noexcept -> ice::CustomClock;

        void update(ice::SystemClock& clock) noexcept;

        void update(ice::CustomClock& clock) noexcept;

        void update_max_delta(
            ice::CustomClock& clock,
            ice::Tns max_delta
        ) noexcept;

        auto elapsed(ice::Clock const& clock) noexcept -> ice::Tns;

        auto elapsed(ice::Timestamp from, ice::Timestamp to) noexcept -> ice::Tns;

    } // namespace clock

    namespace timer
    {

        auto create_timer(
            ice::Clock const& clock,
            ice::Tns timer_step
        ) noexcept -> ice::Timer;

        auto create_timer(
            ice::Clock const& clock,
            ice::Tns timer_step,
            ice::Timestamp initial_timestamp
        ) noexcept -> ice::Timer;

        bool update(ice::Timer& timer) noexcept;

        bool update_by_step(ice::Timer& timer) noexcept;

        auto elapsed(ice::Timer const& timer) noexcept -> ice::Tns;

        auto alpha(ice::Timer const& timer) noexcept -> ice::f32;

    } // namespace timer

    namespace timeline
    {

        auto create_timeline(ice::Clock const& clock) noexcept -> ice::Timeline;

        void reset(ice::Timeline& timeline) noexcept;

        auto elapsed(ice::Timeline const& timeline) noexcept -> ice::Tns;

    } // namespace timeline

    namespace stopwatch
    {

        // Always uses system clock
        auto create_stopwatch() noexcept -> ice::Stopwatch;
        auto create_stopwatch(ice::Clock const& clock) noexcept -> ice::Stopwatch;

        auto elapsed(ice::Stopwatch const& stopwatch) noexcept -> ice::Tns;

        void start(ice::Stopwatch& stopwatch) noexcept;

        void stop(ice::Stopwatch& stopwatch) noexcept;

    } // namespace stopwatch

} // namespace ice
