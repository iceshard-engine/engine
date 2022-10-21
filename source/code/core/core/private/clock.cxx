#include <ice/clock.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>

#include <ice/stringid.hxx>

namespace ice
{

    namespace clock
    {

#if ISP_WINDOWS

        auto clock_frequency() noexcept -> ice::f32
        {
            static ice::i64 cpu_frequency = []() noexcept
            {
                LARGE_INTEGER large_int;
                QueryPerformanceFrequency(&large_int);

                return large_int.QuadPart;
            }();

            return static_cast<ice::f32>(cpu_frequency);
        }

        auto create_clock() noexcept -> ice::SystemClock
        {
            LARGE_INTEGER large_int;
            QueryPerformanceCounter(&large_int);
            return ice::SystemClock{ large_int.QuadPart, large_int.QuadPart };
        }

        auto create_clock(
            ice::Clock const& clock,
            ice::f32 modifier
        ) noexcept -> ice::CustomClock
        {
            ice::CustomClock result{
                .base_clock = &clock,
                .modifier = modifier
            };
            result.previous_timestamp = clock.latest_timestamp;
            result.latest_timestamp = clock.latest_timestamp;
            return result;
        }

        void update(ice::SystemClock& clock) noexcept
        {
            LARGE_INTEGER large_int;
            QueryPerformanceCounter(&large_int);

            clock.previous_timestamp = clock.latest_timestamp;
            clock.latest_timestamp = large_int.QuadPart;
        }

#elif ISP_UNIX


        auto clock_frequency() noexcept -> ice::f32
        {
            return static_cast<ice::f32>(1.f);
        }

        auto create_clock() noexcept -> ice::SystemClock
        {
            return ice::SystemClock{ 0, 0 };
        }

        auto create_clock(ice::Clock const& clock, ice::f32 modifier) noexcept -> ice::CustomClock
        {
            ice::CustomClock result{
                .base_clock = &clock,
                .modifier = modifier
            };
            result.previous_timestamp = clock.latest_timestamp;
            result.latest_timestamp = clock.latest_timestamp;
            return result;
        }

        void update([[maybe_unused]] ice::SystemClock& clock) noexcept
        {
        }

#endif // ISP_WINDOWS

        void update(ice::CustomClock& clock) noexcept
        {
            clock.previous_timestamp = clock.latest_timestamp;
            clock.latest_timestamp += static_cast<ice::i64>(
                (clock.base_clock->latest_timestamp - clock.base_clock->previous_timestamp) * clock.modifier
            );
        }

        void update_max_delta(
            ice::CustomClock& clock,
            ice::f32 max_elapsed_seconds
        ) noexcept
        {
            ice::i64 const max_delta_ticks = static_cast<ice::i64>((max_elapsed_seconds / clock.modifier) * clock_frequency());
            ice::i64 const base_clock_delta_ticks = clock.base_clock->latest_timestamp - clock.base_clock->previous_timestamp;
            clock.previous_timestamp = clock.latest_timestamp;
            clock.latest_timestamp += static_cast<ice::i64>(std::min(base_clock_delta_ticks, max_delta_ticks) * clock.modifier);
        }

        auto elapsed(ice::Clock const& clock) noexcept -> ice::f32
        {
            return static_cast<ice::f32>(clock.latest_timestamp - clock.previous_timestamp) / clock_frequency();
        }

    } // namespace clock

    namespace timer
    {

        auto create_timer(
            ice::Clock const& clock,
            ice::f32 step_seconds
        ) noexcept -> ice::Timer
        {
            return ice::Timer{
                .clock = &clock,
                .step = static_cast<ice::i64>(ice::clock::clock_frequency() * step_seconds),
                .last_tick_timestamp = clock.latest_timestamp,
            };
        }

        auto create_timer(
            ice::Clock const& clock,
            ice::f32 step_seconds,
            ice::i64 initial_timestamp
        ) noexcept -> ice::Timer
        {
            return ice::Timer{
                .clock = &clock,
                .step = static_cast<ice::i64>(ice::clock::clock_frequency() * step_seconds),
                .last_tick_timestamp = initial_timestamp,
            };
        }

        bool update(ice::Timer& timer) noexcept
        {
            ice::i64 const time_passed_since_tick = timer.clock->latest_timestamp - timer.last_tick_timestamp;
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

        bool update_by_step(ice::Timer& timer) noexcept
        {
            ice::i64 const time_passed_since_tick = timer.clock->latest_timestamp - timer.last_tick_timestamp;
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

        auto elapsed(ice::Timer const& timer) noexcept -> ice::f32
        {
            return static_cast<ice::f32>(timer.clock->latest_timestamp - timer.last_tick_timestamp)
                / ice::clock::clock_frequency();
        }

        auto alpha(ice::Timer const& timer) noexcept -> ice::f32
        {
            return static_cast<ice::f32>(timer.clock->latest_timestamp - timer.last_tick_timestamp)
                / timer.step;
        }

    } // namespace timer

    namespace timeline
    {

        auto create_timeline(ice::Clock const& clock) noexcept -> ice::Timeline
        {
            return ice::Timeline{
                .clock = &clock,
                .initial_timestap = clock.latest_timestamp
            };
        }

        auto elapsed(ice::Timeline const& timeline) noexcept -> ice::f32
        {
            return static_cast<ice::f32>(timeline.clock->latest_timestamp - timeline.initial_timestap)
                / ice::clock::clock_frequency();
        }

    } // namespace timeline

    namespace stopwatch
    {

        auto create_stopwatch(ice::Clock const& clock) noexcept -> ice::Stopwatch
        {
            return ice::Stopwatch{ &clock, 0, 0 };
        }

        void start(ice::Stopwatch& stopwatch) noexcept
        {
            stopwatch.initial_timestamp = stopwatch.clock->latest_timestamp;
            stopwatch.final_timestamp = stopwatch.initial_timestamp;
        }

        void stop(ice::Stopwatch& stopwatch) noexcept
        {
            stopwatch.final_timestamp = stopwatch.clock->latest_timestamp;
        }

    } // namespace stopwatch

} // namespace core
