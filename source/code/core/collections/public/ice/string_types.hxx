#pragma once
#include <ice/mem_allocator.hxx>
//#include <string_view>

namespace ice
{

    //! \brief Null position used by string iterator types.
    static constexpr ice::ucount String_NPos = ice::ucount_max;

    //! \brief Constant string type.
    template<typename CharType = char>
    struct BasicString
    {
        using ValueType = CharType const;
        using ConstIterator = CharType const*;
        using ConstReverseIterator = std::reverse_iterator<CharType const*>;

        ice::ucount _size;
        CharType const* _data;

        constexpr BasicString() noexcept;
        constexpr BasicString(BasicString&& other) noexcept;
        constexpr BasicString(BasicString const& other) noexcept;

        constexpr BasicString(CharType const* str_ptr) noexcept;
        constexpr BasicString(CharType const* str_beg, CharType const* str_end) noexcept;
        constexpr BasicString(CharType const* str_ptr, ice::ucount size) noexcept;
        template<ice::ucount Size>
        constexpr BasicString(CharType const(&str_arr)[Size]) noexcept;

        constexpr auto operator=(BasicString&& other) noexcept -> BasicString&;
        constexpr auto operator=(BasicString const& other) noexcept -> BasicString&;

        constexpr auto operator[](ice::ucount index) const noexcept -> CharType const&;

        constexpr bool operator==(BasicString other) const noexcept;
        constexpr bool operator!=(BasicString other) const noexcept;
    };

    //! \brief Constant string type.
    using String = ice::BasicString<char>;

    //! \brief A heap allocated string object.
    //!
    //! \note Depending on the use case an allocator not related to system heap may be provided.
    //!   This is still a valid use case.
    template<typename CharType = char>
    struct HeapString
    {
        using ValueType = CharType;
        using Iterator = CharType*;
        using ReverseIterator = std::reverse_iterator<CharType*>;
        using ConstIterator = CharType const*;
        using ConstReverseIterator = std::reverse_iterator<CharType const*>;

        ice::Allocator* _allocator;
        ice::ucount _capacity;
        ice::ucount _size;
        CharType* _data;

        inline explicit HeapString(ice::Allocator& allocator) noexcept;
        inline HeapString(ice::Allocator& allocator, ice::BasicString<CharType> string) noexcept;
        inline ~HeapString() noexcept;

        inline HeapString(HeapString&& other) noexcept;
        inline HeapString(HeapString const& other) noexcept;

        inline auto operator=(HeapString&& other) noexcept -> HeapString&;
        inline auto operator=(HeapString const& other) noexcept -> HeapString&;
        inline auto operator=(ice::BasicString<CharType> str) noexcept -> HeapString&;

        inline auto operator[](ice::ucount index) noexcept -> CharType&;
        inline auto operator[](ice::ucount index) const noexcept -> CharType const&;

        inline operator ice::BasicString<CharType>() const noexcept;
    };

    //! \brief A static capacity string object.
    //!
    //! \note Because the capacity is static some string operations are not implemented or may behave differently.
    //!   For example, assining a string of length 20 to a static static string of capacity 12, will drop the last 8 characers.
    template<ice::ucount Capacity = 12, typename CharType = char>
    struct StaticString
    {
        using ValueType = CharType;
        using Iterator = CharType*;
        using ReverSeIterator = std::reverse_iterator<CharType*>;
        using ConstIterator = CharType const*;
        using ConstReverseIterator = std::reverse_iterator<CharType const*>;

        static constexpr ice::ucount Constant_Capacity = Capacity;

        ice::ucount _size;
        CharType _data[Constant_Capacity];

        constexpr StaticString() noexcept;
        constexpr ~StaticString() noexcept = default;

        constexpr StaticString(ice::BasicString<CharType> str) noexcept;

        template<ice::ucount ArraySize>
        constexpr StaticString(CharType const (&str_arr)[ArraySize]) noexcept;

        template<ice::ucount OtherCapacity>
        constexpr StaticString(ice::StaticString<OtherCapacity, CharType> const& other) noexcept;

        template<ice::ucount OtherCapacity>
        constexpr auto operator=(ice::StaticString<OtherCapacity, CharType> const& other) noexcept -> StaticString&;
        constexpr auto operator=(ice::BasicString<CharType> str) noexcept -> StaticString&;

        constexpr auto operator[](ice::ucount idx) noexcept -> CharType&;
        constexpr auto operator[](ice::ucount idx) const noexcept -> CharType const&;

        constexpr operator ice::BasicString<CharType>() const noexcept;
    };

} // namespace ice
