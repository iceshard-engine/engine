/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/clock.hxx>
#include <ice/os/windows.hxx>
#include <ice/os/unix.hxx>

#if ISP_UNIX
#include <chrono>
#endif

namespace ice
{

    inline auto operator+(ice::Timestamp left, ice::Timestamp right) noexcept -> ice::Timestamp
    {
        return { left.value + right.value };
    }

    inline auto operator+=(ice::Timestamp& left, ice::Timestamp right) noexcept -> ice::Timestamp&
    {
        left.value += right.value;
        return left;
    }

    inline auto operator-(ice::Timestamp left, ice::Timestamp right) noexcept -> ice::Timestamp
    {
        return { left.value - right.value };
    }

    inline auto operator*(ice::Timestamp left, ice::f32 right) noexcept -> ice::Timestamp
    {
        return { static_cast<ice::i64>(left.value * right) };
    }

    inline auto operator<=>(ice::Timestamp left, ice::Timestamp right) noexcept
    {
        return left.value <=> right.value;
    }

    inline auto operator*(ice::ClockFrequency freq, ice::Tns time) noexcept -> ice::Timestamp
    {
        return { ice::i64((freq.value * time.value) / ice::Tns::Constant_Precision) };
    }

    namespace detail
    {

        auto clock_frequency_1o0() noexcept -> ice::f64
        {
            static ice::f64 const cpu_frequency_value = []() noexcept
            {
                return 1.0 / ice::clock::clock_frequency().value;
            }();
            return cpu_frequency_value;
        }

        auto clock_or_now(ice::Clock const* clock) noexcept -> ice::Timestamp
        {
            return clock != nullptr ? clock->_ts_latest : ice::clock::now();
        }

        auto timestamp_or_now(ice::Timestamp ts) noexcept -> ice::Timestamp
        {
            return ts.value > 0 ? ts : ice::clock::now();
        }

        auto timestamp_sub_fp(ice::Timestamp left, ice::Timestamp right) noexcept -> ice::f64
        {
            return static_cast<ice::f64>(left.value - right.value);
        }

        template<ice::TimeType T>
        auto elapsed_timestamp(ice::Timestamp from, ice::Timestamp to) noexcept -> T
        {
            return T{
                static_cast<T::ValueType>(timestamp_sub_fp(to, from) * (clock_frequency_1o0() * T::Constant_Precision))
            };
        }

        template<typename T>
        auto elapsed_alpha(ice::Timestamp from, ice::Timestamp to, ice::Timestamp range) noexcept -> T
        {
            return static_cast<T>(
                timestamp_sub_fp(to, from) / static_cast<ice::f64>(range.value)
            );
        }

    } // namespace detail

    namespace clock
    {

#if ISP_WINDOWS

        auto now() noexcept -> ice::Timestamp
        {
            LARGE_INTEGER large_int;
            QueryPerformanceCounter(&large_int);

            return { large_int.QuadPart };
        }

        auto clock_frequency() noexcept -> ice::ClockFrequency
        {
            static ice::i64 cpu_frequency = []() noexcept
            {
                LARGE_INTEGER large_int;
                QueryPerformanceFrequency(&large_int);

                return large_int.QuadPart;
            }();

            return { static_cast<ice::f64>(cpu_frequency) };
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
                ._clock_base = &clock,
                .modifier = modifier
            };
            result._ts_previous = clock._ts_latest;
            result._ts_latest = clock._ts_latest;
            return result;
        }

        void update(ice::SystemClock& clock) noexcept
        {
            LARGE_INTEGER large_int;
            QueryPerformanceCounter(&large_int);

            clock._ts_previous = clock._ts_latest;
            clock._ts_latest.value = large_int.QuadPart;
        }

#elif ISP_UNIX

        auto now() noexcept -> ice::Timestamp
        {
            return { std::chrono::high_resolution_clock::now().time_since_epoch().count() };
        }

        auto clock_frequency() noexcept -> ice::ClockFrequency
        {
            return { 1'000'000'000llu };
        }

        auto create_clock() noexcept -> ice::SystemClock
        {
            ice::Timestamp const now = ice::clock::now();
            return ice::SystemClock{ now, now };
        }

        auto create_clock(ice::Clock const& clock, ice::f32 modifier) noexcept -> ice::CustomClock
        {
            ice::CustomClock result{
                ._clock_base = &clock,
                .modifier = modifier
            };
            result._ts_previous = clock._ts_latest;
            result._ts_latest = clock._ts_latest;
            return result;
        }

        void update([[maybe_unused]] ice::SystemClock& clock) noexcept
        {
            clock._ts_previous = clock._ts_latest;
            clock._ts_latest = ice::clock::now();
        }

#endif // ISP_WINDOWS

        void update(ice::CustomClock& clock) noexcept
        {
            clock._ts_previous = clock._ts_latest;
            clock._ts_latest += (clock._clock_base->_ts_latest - clock._clock_base->_ts_previous) * clock.modifier;
        }

        void update_max_delta(
            ice::CustomClock& clock,
            ice::Tns max_delta
        ) noexcept
        {
            ice::Timestamp const delta_ticks = (clock._clock_base->_ts_latest - clock._clock_base->_ts_previous) * clock.modifier;
            ice::Timestamp const delta_ticks_max = clock_frequency() * max_delta;

            clock._ts_previous = clock._ts_latest;
            clock._ts_latest += ice::min(delta_ticks, delta_ticks_max);
        }

        auto elapsed(ice::Clock const& clock) noexcept -> ice::Tns
        {
            return ice::clock::elapsed(clock._ts_previous, clock._ts_latest);
        }

        auto elapsed(ice::Timestamp from, ice::Timestamp to) noexcept -> ice::Tns
        {
            return ice::detail::elapsed_timestamp<ice::Tns>(from, to);
        }

    } // namespace clock

    namespace timer
    {

        auto create_timer(
            ice::Clock const& clock,
            ice::Tns timer_step
        ) noexcept -> ice::Timer
        {
            return ice::Timer{
                ._clock_base = &clock,
                ._timer_step = ice::clock::clock_frequency() * timer_step,
                ._ts_latest = clock._ts_latest,
            };
        }

        auto create_timer(
            ice::Clock const& clock,
            ice::Tns timer_step,
            ice::Timestamp _ts_initial
        ) noexcept -> ice::Timer
        {
            return ice::Timer{
                ._clock_base = &clock,
                ._timer_step = ice::clock::clock_frequency() * timer_step,
                ._ts_latest = _ts_initial,
            };
        }

        bool update(ice::Timer& timer) noexcept
        {
            ice::Timestamp const time_passed_since_tick = timer._clock_base->_ts_latest - timer._ts_latest;
            if (time_passed_since_tick > timer._timer_step)
            {
                timer._ts_latest = timer._clock_base->_ts_latest;
                return true;
            }
            else
            {
                return false;
            }
        }

        bool update_by_step(ice::Timer& timer) noexcept
        {
            ice::Timestamp const time_passed_since_tick = timer._clock_base->_ts_latest - timer._ts_latest;
            if (time_passed_since_tick > timer._timer_step)
            {
                timer._ts_latest += timer._timer_step;
                return true;
            }
            else
            {
                return false;
            }
        }

        auto elapsed(ice::Timer const& timer) noexcept -> ice::Tns
        {
            return ice::detail::elapsed_timestamp<ice::Tns>(timer._ts_latest, timer._clock_base->_ts_latest);
        }

        auto elapsed_us(ice::Timer const& timer) noexcept -> ice::Tus
        {
            return ice::detail::elapsed_timestamp<ice::Tus>(
                timer._ts_latest,
                timer._clock_base->_ts_latest
            );
        }

        auto alpha(ice::Timer const& timer) noexcept -> ice::f32
        {
            return ice::detail::elapsed_alpha<ice::f32>(
                timer._ts_latest,
                timer._clock_base->_ts_latest,
                timer._timer_step
            );
        }

    } // namespace timer

    namespace timeline
    {

        auto create_timeline(ice::Clock const& clock) noexcept -> ice::Timeline
        {
            return ice::Timeline{
                ._clock_base = &clock,
                ._ts_initial = clock._ts_latest
            };
        }

        void reset(ice::Timeline& timeline) noexcept
        {
            timeline._ts_initial = timeline._clock_base->_ts_latest;
        }

        auto elapsed(ice::Timeline const& timeline) noexcept -> ice::Tns
        {
            return ice::detail::elapsed_timestamp<ice::Tns>(
                timeline._ts_initial,
                timeline._clock_base->_ts_latest
            );
        }

        auto elapsed_us(ice::Timeline const& timeline) noexcept -> ice::Tus
        {
            return ice::detail::elapsed_timestamp<ice::Tus>(
                timeline._ts_initial,
                timeline._clock_base->_ts_latest
            );
        }

    } // namespace timeline

    namespace stopwatch
    {

        auto create_stopwatch() noexcept -> ice::Stopwatch
        {
            return ice::Stopwatch{ nullptr, {0}, {0} };
        }

        auto create_stopwatch(ice::Clock const& clock) noexcept -> ice::Stopwatch
        {
            return ice::Stopwatch{ &clock, {0}, {0} };
        }

        auto elapsed(ice::Stopwatch const& stopwatch) noexcept -> ice::Tns
        {
            return ice::detail::elapsed_timestamp<ice::Tns>(
                stopwatch._ts_initial,
                ice::detail::timestamp_or_now(stopwatch._ts_final)
            );
        }

        auto elapsed_us(ice::Stopwatch const& stopwatch) noexcept -> ice::Tus
        {
            return ice::detail::elapsed_timestamp<ice::Tus>(
                stopwatch._ts_initial,
                ice::detail::timestamp_or_now(stopwatch._ts_final)
            );
        }

        void start(ice::Stopwatch& stopwatch) noexcept
        {
            stopwatch._ts_initial = ice::detail::clock_or_now(stopwatch.clock);
            stopwatch._ts_final = {0};
        }

        void stop(ice::Stopwatch& stopwatch) noexcept
        {
            stopwatch._ts_final = ice::detail::clock_or_now(stopwatch.clock);
        }

    } // namespace stopwatch

} // namespace core
