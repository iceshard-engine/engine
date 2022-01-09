#pragma once
#include <cstdint>
#include <cmath>

namespace ice
{

    using f32 = std::float_t;
    using f64 = std::double_t;

    using i8 = std::int8_t;
    using i16 = std::int16_t;
    using i32 = std::int32_t;
    using i64 = std::int64_t;

    using u8 = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    using uptr = std::uintptr_t;

    constexpr ice::f32 const f32_max = std::numeric_limits<ice::f32>::max();
    constexpr ice::f64 const f64_max = std::numeric_limits<ice::f64>::max();

    constexpr ice::i8 const i8_max = std::numeric_limits<ice::i8>::max();
    constexpr ice::i16 const i16_max = std::numeric_limits<ice::i16>::max();
    constexpr ice::i32 const i32_max = std::numeric_limits<ice::i32>::max();
    constexpr ice::i64 const i64_max = std::numeric_limits<ice::i64>::max();

    constexpr ice::u8 const u8_max = std::numeric_limits<ice::u8>::max();
    constexpr ice::u16 const u16_max = std::numeric_limits<ice::u16>::max();
    constexpr ice::u32 const u32_max = std::numeric_limits<ice::u32>::max();
    constexpr ice::u64 const u64_max = std::numeric_limits<ice::u64>::max();

    namespace detail
    {

        enum NumberCastFlags : ice::u8
        {
            NC_NONE,
            NC_SIZE = 0x1,
            NC_SIGN = 0x2,
            NC_REPR = 0x4,
        };

        template<typename To, NumberCastFlags CastFlags, typename From>
        constexpr auto number_cast(From value) noexcept -> To
        {
            static_assert(
                (std::is_integral_v<From> || std::is_floating_point_v<To>) && (std::is_integral_v<To> || std::is_floating_point_v<To>),
                "Only numeric values can be casted with this function!"
            );
            static_assert(
                (CastFlags & NC_REPR) != NC_NONE || (std::is_integral_v<To> ^ std::is_integral_v<From>) == 0,
                "Cannot cast to different representation without specyfing the 'CT_REPR' flag!"
            );
            static_assert(
                (CastFlags & NC_SIZE) != NC_NONE || sizeof(To) >= sizeof(From),
                "Cannot cast to smaller type without specyfing the 'CT_SIZE' flag!"
            );
            static_assert(
                (CastFlags & NC_SIGN) != NC_NONE || (std::is_signed_v<To> ^ std::is_signed_v<From>) == 0,
                "Cannot change signedness without specyfing the 'CT_SIGN' flag!"
            );

            return static_cast<To>(value);
        }

    } // namespace detail

    using detail::NumberCastFlags::NC_NONE;
    using detail::NumberCastFlags::NC_SIZE;
    using detail::NumberCastFlags::NC_SIGN;
    using detail::NumberCastFlags::NC_REPR;

    template<detail::NumberCastFlags CastFlags, typename From>
    constexpr auto as_i32(From from) noexcept -> ice::i32
    {
        return detail::number_cast<ice::i32, CastFlags>(from);
    }

} // namespace ice
