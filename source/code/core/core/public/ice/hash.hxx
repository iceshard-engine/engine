#pragma once
#include <cstdint>
#include <ice/types.hxx>
#include <ice/hash/murmur2.hxx>
#include <ice/hash/murmur3.hxx>

namespace ice
{

    template<typename T>
    constexpr auto hash(T value) noexcept -> ice::u64;

    template<>
    constexpr auto hash(std::string_view value) noexcept -> ice::u64;

    template<>
    constexpr auto hash(char* const value) noexcept -> ice::u64;

    template<typename T>
    constexpr auto hash32(T value) noexcept -> uint32_t;

    template<>
    constexpr auto hash32(std::string_view value) noexcept -> ice::u32;

    template<>
    constexpr auto hash32(char const* value) noexcept -> ice::u32;


    template<typename T>
    constexpr auto hash(T value) noexcept -> ice::u64
    {
        return static_cast<ice::u64>(value);
    }

    template<>
    constexpr auto hash(std::string_view value) noexcept -> ice::u64
    {
        using namespace ice::detail::murmur2_hash;

        mm2_x64_64 const result = cexpr_murmur2_x64_64(value, 0x8642DA39);
        return result.h[0];
    }

    template<>
    constexpr auto hash(char const* value) noexcept -> ice::u64
    {
        using namespace ice::detail::murmur2_hash;

        mm2_x64_64 const result = cexpr_murmur2_x64_64(value, 0x8642DA39);
        return result.h[0];
    }

    template<>
    constexpr auto hash(std::u8string_view value) noexcept -> ice::u64
    {
        using namespace ice::detail::murmur2_hash;

        mm2_x64_64 const result = cexpr_murmur2_x64_64(value, 0x8642DA39);
        return result.h[0];
    }

    template<>
    constexpr auto hash(char8_t const* value) noexcept -> ice::u64
    {
        using namespace ice::detail::murmur2_hash;

        mm2_x64_64 const result = cexpr_murmur2_x64_64(value, 0x8642DA39);
        return result.h[0];
    }

    template<typename T>
    constexpr auto hash32(T value) noexcept -> ice::u32
    {
        return static_cast<ice::u32>(value);
    }

    template<>
    constexpr auto hash32(std::string_view value) noexcept -> ice::u32
    {
        using namespace ice::detail::murmur3_hash;

        mm3_x86_h32 const result = cexpr_murmur3_x86_32(value, 0x428639DA);
        return result.h[0];
    }

    template<>
    constexpr auto hash32(char const* value) noexcept -> ice::u32
    {
        using namespace ice::detail::murmur3_hash;

        mm3_x86_h32 const result = cexpr_murmur3_x86_32(value, 0x428639DA);
        return result.h[0];
    }

    template<typename T>
    constexpr auto hash_from_ptr(T* ptr) noexcept -> ice::u64
    {
        return hash(reinterpret_cast<ice::uptr>(ptr));
    }

} // namespace ice
