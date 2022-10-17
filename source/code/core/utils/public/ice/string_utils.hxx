#pragma once
#include <ice/string/string.hxx>
#include <ice/result_codes.hxx>
#include <ice/math.hxx>
#include <charconv>
#include <numeric>

namespace ice
{

    template<typename StrType>
    struct FromCharsResult
    {
        ice::ResCode ec;
        StrType remaining;

        constexpr operator bool() const noexcept
        {
            return ec == Res::Success;
        }
    };

    template<typename T>
        requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
    auto from_chars(ice::String str, T& out_value) noexcept -> ice::FromCharsResult<ice::String>
    {
        ice::ResCode res = ice::Res::Success;
        std::from_chars_result const fc_res = std::from_chars(
            ice::string::begin(str),
            ice::string::end(str),
            out_value
        );

        if (fc_res.ec == std::errc::result_out_of_range)
        {
            res = ice::Res::E_ValueOutOfRange;
        }
        else if (fc_res.ec == std::errc::invalid_argument)
        {
            res = ice::Res::E_InvalidArgument;
        }

        return {
            .ec = res,
            .remaining = ice::String{ fc_res.ptr, ice::string::end(str) }
        };
    }

    template<typename T>
        requires (std::is_integral_v<T> || std::is_floating_point_v<T>)
    auto from_chars(char const* str_beg, char const* str_end, T& out_value) noexcept -> ice::FromCharsResult<char const*>
    {
        ice::ResCode res = ice::Res::Success;
        std::from_chars_result const fc_res = std::from_chars(
            str_beg,
            str_end,
            out_value
        );

        if (fc_res.ec == std::errc::result_out_of_range)
        {
            res = ice::Res::E_ValueOutOfRange;
        }
        else if (fc_res.ec == std::errc::invalid_argument)
        {
            res = ice::Res::E_InvalidArgument;
        }

        return {
            .ec = res,
            .remaining = fc_res.ptr
        };
    }

} // namespace ice
