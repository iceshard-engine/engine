/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/i18n_detail.hxx>

namespace ice
{

    // a.b.c/error.message lang=ja-JP|Fallback Message
    class I18NReference
    {
    public:
        constexpr I18NReference() noexcept = default;
        constexpr I18NReference(ice::String fallback) noexcept;
        constexpr I18NReference(ice::StringType auto const& fallback) noexcept;

        static constexpr auto from_string(ice::String value) noexcept -> ice::I18NReference;

        [[nodiscard]]
        constexpr auto reference() const noexcept -> ice::String;

        [[nodiscard]]
        constexpr auto path() const noexcept -> ice::String;

        [[nodiscard]]
        constexpr auto key() const noexcept -> ice::String;

        [[nodiscard]]
        constexpr auto flags() const noexcept -> ice::i18n::I18NFlags;

        [[nodiscard]]
        constexpr auto fallback() const noexcept -> ice::String;

        [[nodiscard]]
        constexpr auto fallback_offset() const noexcept -> ice::nindex;

    public:
        char const* _data = nullptr;
        ice::u32 _hash = 0;
        ice::u8 _path = 0;
        ice::u8 _key = 0;
        ice::u8 _flags = 0;
        ice::u8 _fallback = 0;
    };

    constexpr I18NReference::I18NReference(ice::StringType auto const& fallback) noexcept
        : I18NReference{ ice::String{ fallback} }
    { }

    constexpr I18NReference::I18NReference(ice::String fallback) noexcept
        : _data{ fallback.data() }
        , _fallback{ fallback.size().u8() }
    {
        ICE_ASSERT_CORE(fallback.find_first_of('/') == ice::nindex_none);
    }

    constexpr auto I18NReference::from_string(ice::String value) noexcept -> ice::I18NReference
    {
        if (value.is_empty())
        {
            return {};
        }

        I18NReference result{};
        if (value.not_empty())
        {
            // Assign the data
            result._data = value.data();
            result._hash = ice::hash32(value);

            // We assume this is always valid, since we don't want to handle invalid i18n strings anyway
            ice::i18n::parse(
                value,
                result._hash,
                result._path,
                result._key,
                result._flags,
                result._fallback
            );
        }
        return result;
    }

    constexpr auto I18NReference::reference() const noexcept -> ice::String
    {
        // We don't want to subtract anything unless we have at least one character.
        return { _data, static_cast<ice::ncount::base_type>(_path + _key) };
    }

    constexpr auto I18NReference::path() const noexcept -> ice::String
    {
        return { _data, static_cast<ice::ncount::base_type>(_path - static_cast<bool>(_path)) };
    }

    constexpr auto I18NReference::key() const noexcept -> ice::String
    {
        return { _data + _path, static_cast<ice::ncount::base_type>(_key) };
    }

    constexpr auto I18NReference::flags() const noexcept -> ice::i18n::I18NFlags
    {
        return ice::i18n::I18NFlags{ ice::String{ _data + _path + _key + static_cast<u8>(_flags != 0), _flags } };
    }

    constexpr auto I18NReference::fallback() const noexcept -> ice::String
    {
        return { _data + fallback_offset(), _fallback };
    }

    constexpr auto I18NReference::fallback_offset() const noexcept -> ice::nindex
    {
        ice::u8 const fallback_prefix = _path + _key + _flags;
        if (_fallback == 0)
        {
            return fallback_prefix;
        }
        else if (fallback_prefix == 0)
        {
            return 0_index;
        }
        else
        {
            return ice::nindex{ fallback_prefix + static_cast<u8>(_flags != 0) + 1u };
        }
    }

    constexpr auto operator""_i18n(char const* text, size_t size) noexcept -> I18NReference
    {
        return ice::I18NReference::from_string(ice::String{ text, size });
    }


    // Compile-Time Unit Tests

    static_assert(I18NReference{ "Fallback Only"_str }.fallback() == "Fallback Only"_str, "Assigning a string directly always makes it a 'fallback' value");
    static_assert(I18NReference{ "Fallback Only"_str }.reference() == ""_str, "Assigned a string results in an empty 'reference' value");
    static_assert(I18NReference{ "Fallback Only"_str }.path() == ""_str, "Assigned a string results in an empty 'path' value");
    static_assert(I18NReference{ "Fallback Only"_str }.key() == ""_str, "Assigned a string results in an empty 'key' value");
    static_assert(I18NReference{ "Fallback Only"_str }.flags() == ""_str, "Assigned a string results in an empty 'flags' value");
    // static_assert(I18NString{ "a/b"_str }); TODO: Hide behind a special define to test expected errors!

    static_assert("a/b no-builtin"_i18n.fallback() == ""_str, "Assigning the shortest reference returns an empty 'fallback' value");
    static_assert("a/b no-builtin"_i18n.reference() == "a/b"_str, "Assigning the shortest reference is valid with one character on both 'path' and 'key' values");
    static_assert("a/b no-builtin"_i18n.path() == "a"_str, "Assigning the shortest reference returns the proper 'path' value");
    static_assert("a/b no-builtin"_i18n.key() == "b"_str, "Assigning the shortest reference returns the proper 'key' value");
    static_assert("a/b no-builtin"_i18n.flags() == "no-builtin"_str, "Assigning the shorest reference with 'no-builtin' flags returns that value");

    static_assert("builtin.core.i18n.test/test.id"_i18n.fallback() == ""_str, "Assigning a proper test reference without flags returns an empty 'fallback' value");
    static_assert("builtin.core.i18n.test/test.id"_i18n.reference() == "builtin.core.i18n.test/test.id"_str, "Assigning a proper test reference returns it's value");
    static_assert("builtin.core.i18n.test/test.id"_i18n.path() == "builtin.core.i18n.test"_str, "Assigning a proper test reference the 'path' value");
    static_assert("builtin.core.i18n.test/test.id"_i18n.key() == "test.id"_str, "Assigning a proper test reference returns the 'key' value");
    static_assert("builtin.core.i18n.test/test.id"_i18n.flags() == ""_str, "Assigning a proper test reference returns empty 'flags' value");

    static_assert("builtin.core.i18n.test/test.id no-flags"_i18n.fallback() == ""_str, "Assigning a reference with flags an empty 'fallback' value");
    static_assert("builtin.core.i18n.test/test.id no-flags"_i18n.reference() == "builtin.core.i18n.test/test.id"_str, "Assigning a reference with flags returns it's value");
    static_assert("builtin.core.i18n.test/test.id no-flags"_i18n.path() == "builtin.core.i18n.test"_str, "Assigning a reference with flags the 'path' value");
    static_assert("builtin.core.i18n.test/test.id no-flags"_i18n.key() == "test.id"_str, "Assigning reference with flags returns the 'key' value");
    static_assert("builtin.core.i18n.test/test.id no-flags"_i18n.flags() == "no-flags"_str, "Assigning reference with flags returns expected 'flags' value");

    static_assert("builtin.core.i18n.test/test.id|Expected Fallback"_i18n.fallback() == "Expected Fallback"_str, "Assigning a reference with fallback returns expected 'fallback' value");
    static_assert("builtin.core.i18n.test/test.id|Expected Fallback"_i18n.reference() == "builtin.core.i18n.test/test.id"_str, "Assigning a reference with fallback returns expected 'reference' value");
    static_assert("builtin.core.i18n.test/test.id|Expected Fallback"_i18n.path() == "builtin.core.i18n.test"_str, "Assigning a reference with fallback the 'path' value");
    static_assert("builtin.core.i18n.test/test.id|Expected Fallback"_i18n.key() == "test.id"_str, "Assigning reference with fallback returns the 'key' value");
    static_assert("builtin.core.i18n.test/test.id|Expected Fallback"_i18n.flags() == ""_str, "Assigning reference with fallback returns empty 'flags' value");

    static_assert("builtin.core.i18n.test/test.id no-flags|Expected Fallback"_i18n.fallback() == "Expected Fallback"_str, "Assigning a reference with flags and fallback returns expected 'fallback' value");
    static_assert("builtin.core.i18n.test/test.id no-flags|Expected Fallback"_i18n.reference() == "builtin.core.i18n.test/test.id"_str, "Assigning a reference with flags and fallback returns expected 'reference' value");
    static_assert("builtin.core.i18n.test/test.id no-flags|Expected Fallback"_i18n.path() == "builtin.core.i18n.test"_str, "Assigning a reference with flags and fallback the 'path' value");
    static_assert("builtin.core.i18n.test/test.id no-flags|Expected Fallback"_i18n.key() == "test.id"_str, "Assigning reference with flags and fallback returns the 'key' value");
    static_assert("builtin.core.i18n.test/test.id no-flags|Expected Fallback"_i18n.flags() == "no-flags"_str, "Assigning reference with flags and fallback returns empty expected 'flags' value");

    // Ensure this fails from time to time remove 'no-builtin' to check
    // static_assert("a/b |Fallback Message"_i18n.fallback() == "|Fallback Messag"_str, "Fallback message is parsed correctly after a space in the flags' part");

} // namespace ice
