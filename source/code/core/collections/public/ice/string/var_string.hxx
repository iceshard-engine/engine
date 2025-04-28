/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/string/string.hxx>

namespace ice::string
{

    template<typename T>
    concept VarStringType = requires(T t) {
        typename T::ConstIterator;
        std::is_same_v<typename T::TypeTag, ice::VarStringTag>;
        { t._data } -> std::convertible_to<typename T::ConstIterator>;
    };

    template<typename CharType>
    inline void set_capacity(ice::HeapVarString<CharType>& str, ice::ucount new_capacity) noexcept;

    template<typename CharType>
    inline void reserve(ice::HeapVarString<CharType>& str, ice::ucount min_capacity) noexcept;

    template<typename CharType>
    inline void grow(ice::HeapVarString<CharType>& str, ice::ucount min_capacity = 0) noexcept;

    template<typename CharType>
    inline void resize(ice::HeapVarString<CharType>& str, ice::ucount new_size) noexcept;

    template<typename CharType>
    inline void shrink(ice::HeapVarString<CharType>& str) noexcept;

    template<typename CharType>
    inline void clear(ice::HeapVarString<CharType>& str) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapVarString<CharType>& str, CharType character) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapVarString<CharType>& str, CharType const* cstr) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapVarString<CharType>& str, ice::HeapVarString<CharType> const& other) noexcept;

    template<typename CharType>
    inline void push_back(ice::HeapVarString<CharType>& str, ice::BasicString<CharType> cstr) noexcept;

    template<typename CharType>
    inline void pop_back(ice::HeapVarString<CharType>& str, ice::ucount count = 1) noexcept;


    inline auto size(ice::string::VarStringType auto const& str) noexcept -> ice::ucount;

    inline auto capacity(ice::string::VarStringType auto const& str) noexcept -> ice::ucount;

    inline bool empty(ice::string::VarStringType auto const& str) noexcept;

    template<ice::string::VarStringType StringType>
    inline auto begin(StringType const& str) noexcept -> typename StringType::ConstIterator;

    template<ice::string::VarStringType StringType>
    inline auto end(StringType const& str) noexcept -> typename StringType::ConstIterator;

    template<typename CharType>
    inline auto rbegin(ice::VarStringBase<CharType> const& str) noexcept -> typename ice::VarStringBase<CharType>::ConstReverseIterator;

    template<typename CharType>
    inline auto rend(ice::VarStringBase<CharType> const& str) noexcept -> typename ice::VarStringBase<CharType>::ConstReverseIterator;

    template<typename CharType>
    inline auto front(ice::VarStringBase<CharType> const& str) noexcept -> typename ice::VarStringBase<CharType>::ValueType;

    template<typename CharType>
    inline auto back(ice::VarStringBase<CharType> const& str) noexcept -> typename ice::VarStringBase<CharType>::ValueType;

    template<typename CharType>
    inline auto substr(ice::VarStringBase<CharType> const& str, ice::ucount pos, ice::ucount len = ice::String_NPos) noexcept -> ice::BasicString<CharType>;

    template<typename CharType>
    inline auto substr_clone(ice::VarStringBase<CharType> const& str, ice::ucount pos, ice::ucount len = ice::String_NPos) noexcept -> ice::VarStringBase<CharType>;

    template<typename CharType>
    inline auto find_first_of(ice::VarStringBase<CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<typename CharType>
    inline auto find_first_of(ice::VarStringBase<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;

    template<typename CharType>
    inline auto find_last_of(ice::VarStringBase<CharType> const& str, CharType character_value) noexcept -> ice::ucount;

    template<typename CharType>
    inline auto find_last_of(ice::VarStringBase<CharType> const& str, ice::BasicString<CharType> character_values) noexcept -> ice::ucount;


    inline auto data_view(ice::string::VarStringType auto const& str) noexcept -> ice::Data;

    template<typename CharType>
    inline auto memory(ice::VarStringBase<CharType>& str) noexcept -> ice::Memory;

    //! \return Extracts the memory allocated by the heap string.
    //! \note The 'size' member contains the 'capacity' of the heap string.
    template<typename CharType>
    inline auto extract_memory(ice::VarStringBase<CharType>& str) noexcept -> ice::Memory;

    inline auto serialize(ice::string::VarStringType auto const& str, ice::Memory target) noexcept -> ice::Memory;

} // namespace ice::string

namespace ice::data
{

    template<typename CharType = char>
    inline auto read_varstring(ice::Data data, ice::VarStringBase<CharType>& out_str) noexcept -> ice::Data;

} // namespace ice::data

namespace ice
{

    using ice::string::size;
    using ice::string::begin;
    using ice::string::end;

} // namespace ice

#include "impl/var_string.inl"
