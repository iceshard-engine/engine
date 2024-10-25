/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types.hxx>
#include <ice/concept/strong_type_integral.hxx>

namespace ice
{

    constexpr auto operator""_Ts(long double time) noexcept -> ice::Ts;
    constexpr auto operator""_Ts(unsigned long long time) noexcept -> ice::Ts;
    constexpr auto operator""_Tms(unsigned long long time) noexcept -> ice::Tms;
    constexpr auto operator""_Tus(unsigned long long time) noexcept -> ice::Tus;
    constexpr auto operator""_Tns(unsigned long long time) noexcept -> ice::Tns;

    //! \brief Represents time interval of seconds.
    //! \details Can be used for user-facing time values, sleeps or waits.
    //! \note It's better to avoid sleeps and waits entirely!
    struct Ts
    {
        using TypeTag = ice::StrongNumeric;
        using ValueType = ice::f64;

        ValueType value;

        constexpr operator Tms() const noexcept;
        constexpr operator Tus() const noexcept;
        constexpr operator Tns() const noexcept;

        static constexpr ValueType Constant_Precision = 1.0;
    };

    //! \brief Represents time interval of milliseconds.
    //! \details Can be used when calculating time passage, usable with sleeps or waits.
    //! \note It's better to avoid sleeps and waits entirely!
    struct Tms
    {
        using TypeTag = ice::StrongNumeric;
        using ValueType = ice::i64;

        ValueType value;

        constexpr operator Ts() const noexcept;
        constexpr operator Tus() const noexcept;
        constexpr operator Tns() const noexcept;

        static constexpr ValueType Constant_Precision = 1000;
    };

    //! \brief Represents time interval of microseconds.
    //! \details Can be used when calculating time passage, usable with sleeps or waits but discourage
    //!     due to resolution problems between systems.
    //! \note It's better to avoid sleeps and waits entirely!
    struct Tus
    {
        using TypeTag = ice::StrongNumeric;
        using ValueType = ice::i64;

        ValueType value;

        constexpr operator Ts() const noexcept;
        constexpr explicit operator Tms() const noexcept;
        constexpr operator Tns() const noexcept;

        static constexpr ValueType Constant_Precision = 1000'000;
    };

    //! \brief Represents time interval of nanoseconds.
    //! \details Can be used when calculating time passing, not usable for sleeps or waits,
    //!     since most systems don't provide wait resolutions at this scale.
    struct Tns
    {
        using TypeTag = ice::StrongNumeric;
        using ValueType = ice::i64;

        ValueType value;

        constexpr operator Ts() const noexcept;
        constexpr explicit operator Tms() const noexcept;
        constexpr explicit operator Tus() const noexcept;

        static constexpr ValueType Constant_Precision = 1000'000'000;
    };

    //! \brief Represents platform native timestamp with undefined representation.
    //! \note Timestamps should not be presented to users, as these values often represent a point in time with
    //!     CPU ticks and differ from system to system.
    struct Timestamp{ ice::i64 value; };

    //! \brief Represent the current systems clock frequency. Can be used to transform timestamps to time values.
    struct ClockFrequency { ice::f64 value; };


    ////////////////////////////////////////////////////////////////////////////////////////////////


    constexpr Ts::operator Tms() const noexcept { return { static_cast<Tms::ValueType>(value * Tms::Constant_Precision) }; }
    constexpr Ts::operator Tus() const noexcept { return { static_cast<Tms::ValueType>(value * Tus::Constant_Precision) }; }
    constexpr Ts::operator Tns() const noexcept { return { static_cast<Tms::ValueType>(value * Tns::Constant_Precision) }; }

    constexpr Tms::operator Ts() const noexcept { return { static_cast<Ts::ValueType>((value * Ts::Constant_Precision) / Tms::Constant_Precision) }; }
    constexpr Tms::operator Tus() const noexcept { return { static_cast<Tms::ValueType>((value * Tus::Constant_Precision) / Tms::Constant_Precision) }; }
    constexpr Tms::operator Tns() const noexcept { return { static_cast<Tms::ValueType>((value * Tns::Constant_Precision) / Tms::Constant_Precision) }; }

    constexpr Tus::operator Ts() const noexcept { return { static_cast<Ts::ValueType>((value * Ts::Constant_Precision) / Tus::Constant_Precision) }; }
    constexpr Tus::operator Tms() const noexcept { return { static_cast<Tms::ValueType>((value * Tms::Constant_Precision) / Tus::Constant_Precision) }; }
    constexpr Tus::operator Tns() const noexcept { return { static_cast<Tms::ValueType>((value * Tns::Constant_Precision) / Tus::Constant_Precision) }; }

    constexpr Tns::operator Ts() const noexcept { return { static_cast<Ts::ValueType>((value * Ts::Constant_Precision) / Tns::Constant_Precision) }; }
    constexpr Tns::operator Tms() const noexcept { return { static_cast<Tms::ValueType>((value * Tms::Constant_Precision) / Tns::Constant_Precision) }; }
    constexpr Tns::operator Tus() const noexcept { return { static_cast<Tms::ValueType>((value * Tus::Constant_Precision) / Tns::Constant_Precision) }; }

    template<typename T>
    concept TimeType = std::is_same_v<T, Ts> || std::is_same_v<T, Tms> || std::is_same_v<T, Tus> || std::is_same_v<T, Tns>;

    namespace detail
    {

        template<ice::TimeType T0, ice::TimeType T1>
        struct TimeTypeTraits
        {
            using HighestPrecision = std::conditional_t<T0::Constant_Precision >= T1::Constant_Precision, T0, T1>;
            using LowestPrecision = std::conditional_t<T0::Constant_Precision <= T1::Constant_Precision, T0, T1>;
        };

        template<ice::TimeType T0, ice::TimeType T1>
        using TTHighestPrecisionType = typename TimeTypeTraits<T0, T1>::HighestPrecision;

        template<ice::TimeType T0, ice::TimeType T1>
        using TTLowestPrecisionType = typename TimeTypeTraits<T0, T1>::LowestPrecision;

    } // namespace detail

    constexpr auto operator<=>(ice::TimeType auto left, TimeType auto right) noexcept
    {
        using Type = ice::detail::TTHighestPrecisionType<decltype(left), decltype(right)>;
        Type const left_op = left;
        Type const right_op = right;
        return left_op.value <=> right_op.value;
    }

    constexpr auto operator+(ice::TimeType auto left, TimeType auto right) noexcept
    {
        using Type = ice::detail::TTHighestPrecisionType<decltype(left), decltype(right)>;
        Type const left_op = left;
        Type const right_op = right;
        return Type{ left_op.value + right_op.value };
    }

    constexpr auto operator-(ice::TimeType auto left, TimeType auto right) noexcept
    {
        using Type = ice::detail::TTHighestPrecisionType<decltype(left), decltype(right)>;
        Type const left_op = left;
        Type const right_op = right;
        return Type{ left_op.value - right_op.value };
    }

    constexpr auto operator==(ice::TimeType auto left, TimeType auto right) noexcept
    {
        using Type = ice::detail::TTLowestPrecisionType<decltype(left), decltype(right)>;
        Type const left_op = (Type) left;
        Type const right_op = (Type) right;
        return left_op.value == right_op.value;
    }

    constexpr auto operator""_Ts(long double time) noexcept -> ice::Ts
    {
        return { static_cast<ice::Ts::ValueType>(time) };
    }

    constexpr auto operator""_Ts(unsigned long long time) noexcept -> ice::Ts
    {
        return { static_cast<ice::Ts::ValueType>(time) };
    }

    constexpr auto operator""_Tms(unsigned long long time) noexcept -> ice::Tms
    {
        return { static_cast<ice::Tms::ValueType>(time) };
    }

    constexpr auto operator""_Tus(unsigned long long time) noexcept -> ice::Tus
    {
        return { static_cast<ice::Tus::ValueType>(time) };
    }

    constexpr auto operator""_Tns(unsigned long long time) noexcept -> ice::Tns
    {
        return { static_cast<ice::Tns::ValueType>(time) };
    }


    static constexpr ice::detail::TTLowestPrecisionType<Tms, Tus> t0{ 1 };
    static constexpr Tns t1 = t0;

    namespace detail::ct_tests
    {

        static_assert(1.0_Ts == 1_Ts);
        // static_assert(0.5_Ts * 4_Ts == 2_Ts);

        static_assert(1_Ts == 1000_Tms);
        static_assert(1_Ts == 1000'000_Tus);
        static_assert(1_Ts == 1000'000'000_Tns);
        static_assert(0.001_Ts == 1_Tms);
        static_assert(0.001_Ts == 1000_Tus);
        static_assert(0.001_Ts == 1000'000_Tns);
        static_assert(0.000'001_Ts == 1_Tus);
        static_assert(0.000'001_Ts == 1000_Tns);
        static_assert(0.000'000'001_Ts == 1_Tns);

        static_assert(1_Ts == 1000_Tms);
        static_assert(1_Ts == 1000'000_Tus);
        static_assert(1_Ts == 1000'000'000_Tns);

        static_assert(1_Tms == 0.001_Ts);
        static_assert(1_Tms == 1000_Tus);
        static_assert(1_Tms == 1000'000_Tns);

        static_assert(1_Tus == 0.000'001_Ts);
        static_assert(1_Tus == 0_Tms);
        static_assert(1_Tus == 1000_Tns);

        static_assert(1_Tns == 0.000'000'001_Ts);
        static_assert(1_Tns == 0_Tms);
        static_assert(1_Tns == 0_Tus);

        static_assert(1_Tns + 1_Tns == 2_Tns);
        static_assert(1_Tns + 1_Tus == 1001_Tns);
        static_assert(1_Tns + 1_Tms == 1000'001_Tns);

        static_assert(1_Ts + 1_Tms == 1001_Tms);
        static_assert(1_Ts + 1_Tus == 1000'001_Tus);
        static_assert(1_Ts + 1_Tns == 1000'000'001_Tns);

        static_assert(1_Ts - 1_Tms == 999_Tms);
        static_assert(1_Ts - 1_Tus == 999'999_Tus);
        static_assert(1_Ts - 1_Tns == 999'999'999_Tns);

        static_assert(2_Tms - 1_Tms == 1_Tms);
        static_assert(2_Tms - 1_Tus == 1'999_Tus);
        static_assert(2_Tms - 1_Tns == 1'999'999_Tns);

        static_assert(1_Tns * 2 == 2_Tns);
        static_assert(1_Tus * 2 == 2_Tus);
        static_assert(1_Tms * 2 == 2_Tms);

        static_assert(2_Tms * 2 - 1_Tms == 3_Tms);
        static_assert(2_Tms * 2 - 1_Tus == 3'999_Tus);
        static_assert(2_Tms * 2 - 1_Tns == 3'999'999_Tns);

    } // namespace detail::ct_tests


} // namespace ice
