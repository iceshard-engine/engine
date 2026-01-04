/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/types/ncount.hxx>
#include <ice/types/nindex.hxx>

namespace ice::concepts
{

    template<typename CharType>
    concept SupportedCharType = std::is_same_v<CharType, char>
        //|| std::is_same_v<CharType, char8_t>
        || std::is_same_v<CharType, wchar_t>;

    template<typename SizeType>
    concept SupportedSizeType = std::is_same_v<SizeType, ice::ncount>;

    template<typename T>
    concept StringType = SupportedCharType<typename T::CharType>
        && SupportedSizeType<typename T::SizeType>
        && requires(T const& t)
    {
        typename T::StringType;
        typename T::ValueType;
        typename T::ConstIterator;
        typename T::ConstReverseIterator;
        { t.data() } -> std::convertible_to<typename T::CharType const*>;
        { t.size() } -> std::convertible_to<typename T::SizeType>;
    };

    template<typename T>
    concept MutableStringType = StringType<T>
        && requires(T& t, typename T::SizeType size_value)
    {
        { t.data() } -> std::convertible_to<typename T::CharType*>;
        { t.capacity() } -> std::convertible_to<typename T::SizeType>;
        { t.resize(size_value) } -> std::convertible_to<void>;
    };

    template<typename T>
    concept ResizableStringType = MutableStringType<T>
        && requires(T& t, typename T::SizeType capacity_value)
    {
        { t.set_capacity(capacity_value) } -> std::convertible_to<void>;
    };

} // namespace ice::concepts

namespace ice::string::detail
{

    template<typename CharType>
    constexpr auto strptr_size(CharType const* str) noexcept -> ice::ncount::base_type
    {
        ice::ncount::base_type result{};
        if (str != nullptr)
        {
            CharType const* it = str;
            while (*it != CharType{ 0 })
            {
                it += 1;
            }

            result = static_cast<ice::ncount::base_type>(it - str);
        }
        return result;
    }

} // namespace string::detail

namespace ice
{

    using ice::concepts::StringType;

} // namespace ice
