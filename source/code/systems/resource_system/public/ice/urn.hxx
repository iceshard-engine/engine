#pragma once
#include <ice/string.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    struct URN
    {
        constexpr explicit URN(ice::String name) noexcept;
        constexpr explicit URN(ice::StringID_Arg name) noexcept;

        ice::StringID name;
    };


    constexpr URN::URN(ice::String name) noexcept
        : name{ ice::stringid(name) }
    { }

    constexpr URN::URN(ice::StringID_Arg name) noexcept
        : name{ name }
    { }

    constexpr auto operator""_urn(char const* urn_raw, std::size_t len) noexcept
    {
        return URN{ ice::String{ urn_raw, len } };
    }

} // namespace ice
