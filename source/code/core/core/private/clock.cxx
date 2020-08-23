#include <core/clock.hxx>
#include <core/platform/windows.hxx>

namespace core
{

    namespace clock
    {

        auto clock_frequency() noexcept -> float
        {
            static int64_t cpu_frequency = []() noexcept
            {
                LARGE_INTEGER large_int;
                QueryPerformanceFrequency(&large_int);

                return large_int.QuadPart;
            }();

            return static_cast<float>(cpu_frequency);
        }

        auto core::clock::create_clock() noexcept -> SystemClock
        {
            LARGE_INTEGER large_int;
            QueryPerformanceCounter(&large_int);
            return SystemClock{ large_int.QuadPart, large_int.QuadPart };
        }

        auto create_clock(Clock const& clock, float modifier) noexcept -> CustomClock
        {
            auto result = CustomClock{
                .base_clock = &clock,
                .modifier = modifier
            };
            result.previous_timestamp = clock.latest_timestamp;
            result.latest_timestamp = clock.latest_timestamp;
            return result;
        }

        void update(SystemClock& clock) noexcept
        {
            LARGE_INTEGER large_int;
            QueryPerformanceCounter(&large_int);

            clock.previous_timestamp = clock.latest_timestamp;
            clock.latest_timestamp = large_int.QuadPart;
        }

        void update(CustomClock& c) noexcept
        {
            c.previous_timestamp = c.latest_timestamp;
            c.latest_timestamp += (c.base_clock->latest_timestamp - c.base_clock->previous_timestamp) * c.modifier;
        }

        void update_max_delta(CustomClock& c, float max_elapsed_seconds)
        {
            auto const max_delta_ticks = static_cast<int64_t>((max_elapsed_seconds / c.modifier) * clock_frequency());
            auto const base_clock_delta_ticks = c.base_clock->latest_timestamp - c.base_clock->previous_timestamp;
            c.previous_timestamp = c.latest_timestamp;
            c.latest_timestamp += std::min(base_clock_delta_ticks, max_delta_ticks) * c.modifier;
        }

        auto elapsed(Clock const& c) noexcept -> float
        {
            return static_cast<float>(c.latest_timestamp - c.previous_timestamp) / clock_frequency();
        }

    } // namespace clock

    namespace timer
    {

        auto create_timer(Clock const& clock, float step_seconds) noexcept -> Timer
        {
            return Timer{
                .clock = &clock,
                .step = static_cast<int64_t>(core::clock::clock_frequency() * step_seconds),
                .last_tick_timestamp = clock.latest_timestamp,
            };
        }

        bool update(Timer& timer) noexcept
        {
            auto const time_passed_since_tick = timer.clock->latest_timestamp - timer.last_tick_timestamp;
            if (time_passed_since_tick > timer.step)
            {
                timer.last_tick_timestamp = timer.clock->latest_timestamp;
                return true;
            }
            else
            {
                return false;
            }
        }

        bool update_by_step(Timer& timer) noexcept
        {
            auto const time_passed_since_tick = timer.clock->latest_timestamp - timer.last_tick_timestamp;
            if (time_passed_since_tick > timer.step)
            {
                timer.last_tick_timestamp += timer.step;
                return true;
            }
            else
            {
                return false;
            }
        }

        auto elapsed(Timer const& timer) noexcept -> float
        {
            return static_cast<float>(timer.clock->latest_timestamp - timer.last_tick_timestamp)
                / core::clock::clock_frequency();
        }

        auto alpha(Timer const& timer) noexcept -> float
        {
            return static_cast<float>(timer.clock->latest_timestamp - timer.last_tick_timestamp)
                / timer.step;
        }

    } // namespace timer

    namespace timeline
    {

        auto create_timeline(Clock const& clock, float speed) noexcept -> Timeline
        {
            return Timeline{
                .clock = &clock,
                .initial_timestap = clock.latest_timestamp
            };
        }

        auto elapsed(Timeline const& timeline) noexcept -> float
        {
            return (
                static_cast<float>(timeline.clock->latest_timestamp - timeline.initial_timestap) / core::clock::clock_frequency()
            );
        }

    } // namespace timeline

    namespace stopwatch
    {

        auto create_stopwatch(Clock const& clock) noexcept
        {
            return Stopwatch{ &clock, 0, 0 };
        }

        void start(Stopwatch& stopwatch) noexcept
        {
            stopwatch.initial_timestamp = stopwatch.clock->latest_timestamp;
            stopwatch.final_timestamp = stopwatch.initial_timestamp;
        }

        void stop(Stopwatch& stopwatch) noexcept
        {
            stopwatch.final_timestamp = stopwatch.clock->latest_timestamp;
        }

    } // namespace stopwatch

} // namespace core
