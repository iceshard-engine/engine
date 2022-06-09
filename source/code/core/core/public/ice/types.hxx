#pragma once
#include <cstdint>
#include <cmath>

namespace ice
{

    using c8utf = char8_t;

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

    using usize = ice::u64;
    using isize = ice::i64;

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

} // namespace ice
