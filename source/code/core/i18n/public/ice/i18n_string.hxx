/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/i18n_reference.hxx>
#include <ice/string.hxx>

namespace ice
{

    struct I18NString : ice::string::ReadOnlyOperations
    {
        using CharType = char;
        using ValueType = CharType const;
        using ConstIterator = ValueType*;
        using ConstReverseIterator = std::reverse_iterator<ValueType*>;
        using Iterator = ConstIterator;
        using ReverseIterator = ConstReverseIterator;
        using SizeType = ice::ncount;
        using StringType = ice::String;

        I18NString(ice::String text) noexcept;
        I18NString(ice::I18NReference reference) noexcept;

        bool resolve() noexcept;
        auto data() const noexcept -> ValueType*;
        auto size() const noexcept -> SizeType;

        operator ice::String() const noexcept { return { data(), size() }; }

    public:
        // Keep one pointer, and depending on the offset, either each for the dynamic values or the static values.
        void const* _data = nullptr;
        ice::i32 _data_offset = 0; // > 0 == not-resolved, < 0 == resolved
        ice::i32 _size_offset = 0;
    };

} // namespace ice
