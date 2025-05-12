/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/concept/strong_type_value.hxx>

namespace ice
{

    //! \brief Type tag to enable utility functions for strongly typed numeric values.
    struct StrongNumeric { };

    template<typename T>
    using StrongNumericBase = typename ice::detail::ExtractMemberType<decltype(&T::value)>::Type;

    //! \brief Concept used to determine if a struct is considerd a strong number wrapper.
    template<typename T>
    concept StrongNumericType = std::is_pod_v<T>
        && ice::detail::HasAliasTypeTag<T, ice::StrongNumeric>
        && ice::detail::OnlyMemberValue<T>
        && std::is_arithmetic_v<ice::StrongNumericBase<T>>;


    namespace detail::numeric
    {

        enum Operator
        {
            Add, Sub, Neg, Mul, Div,
            AddEq, SubEq, MulEq, DivEq,
        };

        //template<typename T, Operator Op> requires ice::StrongNumericType<T>
        //constexpr static bool IsOperatorEnabled = true;

        //template<typename T>
        //constexpr static bool IsAddEnabled = IsOperatorEnabled<T, Operator::Add>;

        //template<typename T>
        //constexpr static bool IsSubEnabled = IsOperatorEnabled<T, Operator::Sub>;

        //template<typename T>
        //constexpr static bool IsNegateEnabled = IsOperatorEnabled<T, Operator::Neg>;

        //template<typename T>
        //constexpr static bool IsMulEnabled = IsOperatorEnabled<T, Operator::Mul>;

        //template<typename T>
        //constexpr static bool IsDivEnabled = IsOperatorEnabled<T, Operator::Div>;

        //template<typename T>
        //constexpr static bool IsAddEqEnabled = IsOperatorEnabled<T, Operator::AddEq>;

        //template<typename T>
        //constexpr static bool IsSubEqEnabled = IsOperatorEnabled<T, Operator::SubEq>;

        //template<typename T>
        //constexpr static bool IsMulEqEnabled = IsOperatorEnabled<T, Operator::MulEq>;

        //template<typename T>
        //constexpr static bool IsDivEqEnabled = IsOperatorEnabled<T, Operator::DivEq>;

    } // namespace detail

    template<typename T> requires ice::StrongNumericType<T>
    constexpr auto operator==(T left, T right) noexcept -> bool
    {
        return left.value == right.value;
    }

    template<typename T> requires ice::StrongNumericType<T>
    constexpr auto operator<=>(T left, T right) noexcept
    {
        return left.value <=> right.value;
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsAddEnabled<T>
    constexpr auto operator+(T left, T right) noexcept -> T
    {
        return T{ left.value + right.value };
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsSubEnabled<T>
    constexpr auto operator-(T left, T right) noexcept -> T
    {
        return T{ left.value - right.value };
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsNegateEnabled<T>
    constexpr auto operator-(T left) noexcept -> T
    {
        return T{ -left.value };
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsMulEnabled<T>
    constexpr auto operator*(T left, T right) noexcept -> T
    {
        return T{ left.value * right.value };
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsMulEnabled<T>
    constexpr auto operator*(T left, ice::StrongNumericBase<T> right) noexcept -> T
    {
        return T{ left.value * right };
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsDivEnabled<T>
    constexpr auto operator/(T left, T right) noexcept -> T
    {
        return T{ left.value / right.value };
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsDivEnabled<T>
    constexpr auto operator/(T left, ice::StrongNumericBase<T> right) noexcept -> T
    {
        return T{ left.value / right };
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsAddEqEnabled<T>
    constexpr auto operator+=(T& left, T right) noexcept -> T&
    {
        left.value += right.value;
        return left;
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsSubEqEnabled<T>
    constexpr auto operator-=(T& left, T right) noexcept -> T&
    {
        left.value -= right.value;
        return left;
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsMulEqEnabled<T>
    constexpr auto operator*=(T& left, T right) noexcept -> T&
    {
        left.value *= right.value;
        return left;
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsMulEqEnabled<T>
    constexpr auto operator*=(T& left, ice::StrongNumericBase<T> right) noexcept -> T&
    {
        left.value *= right;
        return left;
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsDivEqEnabled<T>
    constexpr auto operator/=(T& left, T right) noexcept -> T&
    {
        left.value /= right.value;
        return left;
    }

    template<typename T> requires StrongNumericType<T>// && ice::detail::numeric::IsDivEqEnabled<T>
    constexpr auto operator/=(T& left, ice::StrongNumericBase<T> right) noexcept -> T&
    {
        left.value /= right;
        return left;
    }

} // namespace ice
