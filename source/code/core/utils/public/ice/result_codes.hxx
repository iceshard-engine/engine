/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/hash.hxx>
#include <ice/build/build.hxx>
#include <ice/concept/strong_type_value.hxx>

namespace ice
{

    template<bool DebugInfo>
    struct ResultCode;

    enum class ResultSeverity : ice::u8
    {
        Success,
        Info,
        Warning,
        Error
    };

    namespace detail
    {

        struct ResultCategory
        {
            ice::u8 value;
        };

        struct ResultValue
        {
            ice::u32 severity : 8;
            ice::u32 id : 24;
        };

        struct ResultCode_Tag;

    } // namespace detail


    template<bool DebugInfo>
    struct ResultCode
    {
        ice::detail::ResultValue value;

        constexpr operator bool() const noexcept;

        static constexpr auto create(ice::ResultSeverity sev, std::string_view desc)
        {
            ice::u32 const res_hash = ice::hash32(desc);
            return ResultCode{
                .value = {
                    .severity = static_cast<std::underlying_type_t<ice::ResultSeverity>>(sev),
                    .id = (res_hash >> 8 ^ (res_hash * 3)) & 0x00ff'ffff
                }
            };
        }
    };

    template<>
    struct ResultCode<true>
    {
        ice::detail::ResultValue value;
        char const* description;

        inline constexpr operator bool() const noexcept;

        static constexpr auto create(ice::ResultSeverity sev, std::string_view desc)
        {
            ice::u32 const res_hash = ice::hash32(desc);
            return ResultCode{
                .value = {
                    .severity = static_cast<std::underlying_type_t<ice::ResultSeverity>>(sev),
                    .id = (res_hash >> 8 ^ (res_hash * 3)) & 0x00ff'ffff
                },
                .description = desc.data()
            };
        }
    };


    using ResCode = ResultCode<ice::build::is_debug || ice::build::is_develop>;

    struct Res
    {
        static constexpr ice::ResultSeverity Info = ResultSeverity::Info;
        static constexpr ice::ResultSeverity Warning = ResultSeverity::Warning;
        static constexpr ice::ResultSeverity Error = ResultSeverity::Error;

        static constexpr ice::ResCode Success = { .value = { .severity = 0, .id = 1 } };
        static constexpr ice::ResCode E_InvalidArgument = ResCode::create(ResultSeverity::Error, "Invalid Argument");
        static constexpr ice::ResCode E_ValueOutOfRange = ResCode::create(ResultSeverity::Error, "Value Out Of Range");
    };


    constexpr auto operator==(ice::detail::ResultValue left, ice::detail::ResultValue right) noexcept
    {
        return left.severity == right.severity && left.id == right.id;
    }

    constexpr auto operator==(ice::detail::ResultValue left, ice::ResultSeverity right) noexcept
    {
        return left.severity == static_cast<std::underlying_type_t<ice::ResultSeverity>>(right);
    }

    constexpr bool operator==(ice::ResCode left, ice::ResCode right) noexcept
    {
        return left.value == right.value;
    }

    template<bool DebugInfo>
    inline constexpr ice::ResultCode<DebugInfo>::operator bool() const noexcept
    {
        return this->value == Res::Success.value;
    }

    inline constexpr ice::ResultCode<true>::operator bool() const noexcept
    {
        return this->value == Res::Success.value;
    }

} // namespace ice
