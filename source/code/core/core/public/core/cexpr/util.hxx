#pragma once
#include <core/base.hxx>

//! \brief Utility namespace containing constexpr functions,
namespace core::cexpr
{


    namespace detail
    {

        template<typename T>
        constexpr auto roundup_helper(T value, T new_value) noexcept -> T
        {
            return value <= new_value ? new_value : roundup_helper(value, new_value << 1);
        }

    } // namespace detail


    //! \brief A roundup function to the next power of 2 value given the input/
    template<typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type, typename = typename std::enable_if<std::is_unsigned<T>::value>::type>
    constexpr auto power_of_two_roundup(T value, T min_value = 2) noexcept -> T
    {
        return detail::roundup_helper(value, min_value);
    }


} // namespace core
