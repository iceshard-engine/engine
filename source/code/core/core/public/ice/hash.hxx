#pragma once
#include <cstdint>
#include <ice/hash/murmur2.hxx>
#include <ice/hash/murmur3.hxx>

namespace ice
{

    template<typename T>
    constexpr auto hash(T value) noexcept -> uint64_t;

    template<>
    constexpr auto hash(std::string_view value) noexcept -> uint64_t;

    template<typename T>
    constexpr auto hash32(T value) noexcept -> uint32_t;

    template<>
    constexpr auto hash32(std::string_view value) noexcept -> uint32_t;


    template<typename T>
    constexpr auto hash(T value) noexcept -> uint64_t
    {
        return uint64_t{ value };
    }

    template<>
    constexpr auto hash(std::string_view value) noexcept -> uint64_t
    {
        return ice::detail::murmur2_hash::cexpr_murmur2_x64_64(value, 0x8642DA39).h[0];
    }

    template<typename T>
    constexpr auto hash32(T value) noexcept -> uint32_t
    {
        return uint32_t{ value };
    }

    template<>
    constexpr auto hash32(std::string_view value) noexcept -> uint32_t
    {
        return ice::detail::murmur3_hash::cexpr_murmur3_x86_32(value, 0x428639DA).h[0];
    }

} // namespace ice
