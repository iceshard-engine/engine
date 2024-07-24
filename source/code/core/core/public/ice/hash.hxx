/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config.hxx>
#include <ice/constants.hxx>
#include <ice/hash/murmur2.hxx>
#include <ice/hash/murmur3.hxx>

namespace ice
{

    template<typename T>
    constexpr auto hash(T value) noexcept -> ice::u64;

    template<>
    constexpr auto hash(std::u8string_view value) noexcept -> ice::u64;

    template<>
    constexpr auto hash(ice::utf8 const* value) noexcept -> ice::u64;

    template<>
    constexpr auto hash(std::string_view value) noexcept -> ice::u64;

    template<>
    constexpr auto hash(char const* value) noexcept -> ice::u64;



    template<typename T>
    constexpr auto hash32(T value) noexcept -> uint32_t;

    template<>
    constexpr auto hash32(std::u8string_view value) noexcept -> ice::u32;

    template<>
    constexpr auto hash32(ice::utf8 const* value) noexcept -> ice::u32;

    template<>
    constexpr auto hash32(std::string_view value) noexcept -> ice::u32;

    template<>
    constexpr auto hash32(char const* value) noexcept -> ice::u32;


    template<typename T>
    constexpr auto hash_from_ptr(T* ptr) noexcept -> ice::u64;


    // IMPLEMENTATION DETAILS


    template<typename T>
    constexpr auto hash(T value) noexcept -> ice::u64
    {
        return static_cast<ice::u64>(value);
    }

    template<>
    constexpr auto hash(std::u8string_view value) noexcept -> ice::u64
    {
        using namespace ice::detail::murmur2_hash;

        mm2_x64_64 const result = cexpr_murmur2_x64_64(value, ice::config::Hash64_DefaultSeed);
        return result.h[0];
    }

    template<>
    constexpr auto hash(ice::utf8 const* value) noexcept -> ice::u64
    {
        return ice::hash(std::u8string_view{ value });
    }

    template<>
    constexpr auto hash(std::string_view value) noexcept -> ice::u64
    {
        using namespace ice::detail::murmur2_hash;

        mm2_x64_64 const result = cexpr_murmur2_x64_64(value, ice::config::Hash64_DefaultSeed);
        return result.h[0];
    }

    template<>
    constexpr auto hash(char const* value) noexcept -> ice::u64
    {
        return ice::hash(std::string_view{ value });
    }


    template<typename T>
    constexpr auto hash32(T value) noexcept -> ice::u32
    {
        return static_cast<ice::u32>(value);
    }

    template<>
    constexpr auto hash32(std::u8string_view value) noexcept -> ice::u32
    {
        using namespace ice::detail::murmur3_hash;

        mm3_x86_h32 const result = cexpr_murmur3_x86_32(value, ice::config::Hash32_DefaultSeed);
        return result.h[0];
    }

    template<>
    constexpr auto hash32(ice::utf8 const* value) noexcept -> ice::u32
    {
        return ice::hash32(std::u8string_view{ value });
    }

    template<>
    constexpr auto hash32(std::string_view value) noexcept -> ice::u32
    {
        using namespace ice::detail::murmur3_hash;

        mm3_x86_h32 const result = cexpr_murmur3_x86_32(value, ice::config::Hash32_DefaultSeed);
        return result.h[0];
    }

    template<>
    constexpr auto hash32(char const* value) noexcept -> ice::u32
    {
        return ice::hash32(std::string_view{ value });
    }


    template<typename T>
    constexpr auto hash_from_ptr(T* ptr) noexcept -> ice::u64
    {
        return hash(reinterpret_cast<ice::uptr>(ptr));
    }


    // COMPILE TILE CHECKS


    namespace _validation
    {

        // Check if numeric values behave in hashing scenarios as expected.
        static_assert(ice::hash32(0x0000'0000) == ice::hash(0x0000'0000));
        static_assert(ice::hash32(0xffff'ffff) == ice::hash(0xffff'ffff));

        static_assert(ice::hash32(0x0000'0000) == ice::hash(0x0000'0000'0000'0000));
        static_assert(ice::hash32(0xffff'ffff) != ice::hash(0xffff'ffff'ffff'ffff));

        static_assert(ice::hash32(0xffff'ffff) == ice::u32_max);
        static_assert(ice::hash(0xffff'ffff'ffff'ffff) == ice::u64_max);

        // Check string values behave is hasing scenarios as expected.
        static_assert(ice::hash32(u8"test_string_1") == ice::hash32(u8"test_string_1"));
        static_assert(ice::hash32(u8"test_string_1") != ice::hash32(u8"test_string_2"));

        static_assert(ice::hash32(u8"test_string_1") != ice::hash(u8"test_string_1"));
        static_assert(ice::hash32(u8"test_string_1") != ice::hash(u8"test_string_2"));

        static_assert(ice::hash(u8"test_string_1") == ice::hash(u8"test_string_1"));
        static_assert(ice::hash(u8"test_string_1") != ice::hash(u8"test_string_2"));

        static_assert(ice::hash32(u8"") == ice::hash32(u8""));
        static_assert(ice::hash32(std::u8string_view{}) == ice::hash32(std::u8string_view{}));

        static_assert(ice::hash32(u8"") != ice::hash(u8""));
        static_assert(ice::hash32(std::u8string_view{}) != ice::hash(std::u8string_view{}));

        static_assert(ice::hash(u8"") == ice::hash(u8""));
        static_assert(ice::hash(std::u8string_view{}) == ice::hash(std::u8string_view{}));

    } // namespace _validation

} // namespace ice
