#pragma once
#include <core/base.hxx>

namespace core
{

    template<typename T = int64_t>
    struct Clock { };

    template<>
    struct Clock<int64_t>
    {
        int64_t const frequency;
        int64_t previous_timestamp;
        int64_t latest_timestamp;
    };

    template<typename T>
    struct Clock<Clock<T>>
    {
        Clock<T> const* base_clock;
        int64_t previous_timestamp;
        int64_t latest_timestamp;
        float modifier;
    };

    template<typename T = Clock<>>
    struct Timer
    {
        T const* clock;
        float const frequency = 1.0f;
        int64_t last_tick_timestamp;
    };

    template<typename T = Clock<>>
    struct Timeline
    {
        T const* clock;
        int64_t const initial_timestap;
        float const speed = 1.0f;
    };

    namespace clock
    {

        auto create_clock() noexcept -> Clock<>;

        template<typename T>
        auto create_clock(T const& clock, float modifier) noexcept
        {
            return Clock<T> {
                .base_clock = &clock,
                .previous_timestamp = clock.latest_timestamp,
                .latest_timestamp = clock.latest_timestamp,
                .modifier = modifier
            };
        }

        template<typename T>
        void update(T& c) noexcept
        {
            c.previous_timestamp = c.latest_timestamp;
            c.latest_timestamp += (c.base_clock->latest_timestamp - c.base_clock->previous_timestamp) * c.modifier;
        }

        template<>
        void update(Clock<>& c) noexcept;

        template<typename T>
        auto frequency(T const& clock) noexcept -> float
        {
            return frequency(*clock.base_clock);
        }

        template<>
        inline auto frequency(Clock<> const& clock) noexcept -> float
        {
            return clock.frequency;
        }

        template<typename R = float, typename T>
        auto elapsed(Clock<T> const& c) noexcept -> float
        {
            if constexpr (std::is_same_v<float, R>)
            {
                return static_cast<float>(c.latest_timestamp - c.previous_timestamp) / frequency(c);
            }
            else
            {
                return static_cast<R>(static_cast<float>(c.latest_timestamp - c.previous_timestamp) / frequency(c));
            }
        }

    } // namespace clock

    namespace timer
    {

        template<typename T = Clock<>>
        auto create_timer(T const& clock, float frequency) noexcept -> Timer<T>
        {
            return Timer<T>{
                .clock = &clock,
                .frequency = frequency,
                .last_tick_timestamp = clock.latest_timestamp,
            };
        }

        template<typename T>
        bool update(Timer<Clock<T>>& timer) noexcept
        {
            auto const time_passed_since_tick = (timer.clock->latest_timestamp - timer.last_tick_timestamp) / core::clock::frequency(*timer.clock);
            if (time_passed_since_tick > timer.frequency)
            {
                timer.last_tick_timestamp = timer.clock->latest_timestamp;
                return true;
            }
            else
            {
                return false;
            }
        }

    } // namespace timer

    namespace timeline
    {

        template<typename T = Clock<>>
        auto create_timeline(T const& clock, float speed = 1.0f) noexcept -> Timeline<T>
        {
            return Timeline<T>{
                .clock = &clock,
                .initial_timestap = clock.latest_timestamp,
                .speed = speed
            };
        }

        template<typename T>
        auto elapsed(Timeline<Clock<T>> const& timeline) noexcept -> float
        {
            return timeline.speed * (
                static_cast<float>(timeline.clock->latest_timestamp - timeline.initial_timestap) / core::clock::frequency(*timeline.clock)
            );
        }

    } // namespace timeline

} // namespace core
