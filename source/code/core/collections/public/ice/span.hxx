/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/mem_memory.hxx>
#include <ice/container_logic.hxx>
#include <ice/container/contiguous_container.hxx>
#include <array>

namespace ice
{

    //! \brief A view into an array of objects laid out in contiguous memory.
    template<typename Type>
    struct Span : public ice::container::ContiguousContainer
    {
        using ValueType = Type;
        using ConstContainerValueType = Type;
        using Iterator = Type*;
        using ReverseIterator = std::reverse_iterator<Type*>;
        using ConstIterator = Type const*;
        using ConstReverseIterator = std::reverse_iterator<Type const*>;
        using SizeType = ice::ncount;
        using ContainerTag = ice::concepts::ContiguousContainerTag;

        SizeType::base_type _count;
        ValueType* _data;

        constexpr Span() noexcept;
        constexpr Span(ice::Span<Type>&& other) noexcept = default;
        constexpr Span(ice::Span<Type> const& other) noexcept = default;
        constexpr Span(Type* location, ice::ncount count) noexcept;
        constexpr Span(Type* from, Type* to) noexcept;

        template<ice::u64 Size>
        constexpr Span(Type(&location)[Size]) noexcept;

        constexpr auto operator=(ice::Span<Type>&& other) noexcept -> ice::Span<Type>& = default;
        constexpr auto operator=(ice::Span<Type> const& other) noexcept -> ice::Span<Type>& = default;

        // API Requirements Of: Contiguous Container
        template<typename Self>
        constexpr auto data(this Self& self) noexcept -> ice::container::ValuePtr<Self> { return self._data; }
        constexpr auto size(this Span const& self) noexcept -> ice::ncount { return { self._count, sizeof(ValueType) }; }

        // API Requirements Of: Data and Memory
        constexpr auto data_view(this Span const& self) noexcept -> ice::Data;
        constexpr auto memory_view(this Span const& self) noexcept -> ice::Memory
            requires(not std::is_const_v<ValueType>);

        // Implicit type conversions
        constexpr operator ice::Span<Type const>() noexcept { return { _data, _count }; }
        constexpr operator ice::Span<Type const>() const noexcept { return { _data, _count }; }
    };

    template<typename T> Span(ice::Span<T>&&) noexcept -> Span<T>;
    template<typename T> Span(ice::Span<T> const&) noexcept -> Span<T>;
    template<typename T, ContainerLogic Logic, template<typename, ContainerLogic> typename Container> Span(Container<T, Logic> const&) noexcept -> Span<T>;
    template<typename T, ice::u64 Size> Span(T(&)[Size]) noexcept -> Span<T>;

    template<typename Type, ice::u64 Size>
    static constexpr auto make_span(std::array<Type, Size>& std_array) noexcept -> Span<Type>;
    template<typename Type, ice::u64 Size>
    static constexpr auto make_span(std::array<Type, Size> const& std_array) noexcept -> Span<Type const>;

    template<typename Type>
    constexpr Span<Type>::Span() noexcept
        : _count{ 0 }
        , _data{ nullptr }
    { }

    template<typename Type>
    constexpr Span<Type>::Span(Type* location, ice::ncount count) noexcept
        : _count{ count.native() }
        , _data{ location }
    { }

    template<typename Type>
    constexpr Span<Type>::Span(Type* from, Type* to) noexcept
        : _count{ static_cast<ice::u64>(to - from) }
        , _data{ from }
    { }

    template<typename Type>
    template<ice::u64 Size>
    constexpr Span<Type>::Span(Type(&location)[Size]) noexcept
        : _count{ Size }
        , _data{ location }
    { }

    template<typename Type>
    inline constexpr auto Span<Type>::data_view(this Span const& self) noexcept -> ice::Data
    {
        return ice::Data{
            .location = self.data(),
            .size = self.size(),
            .alignment = ice::align_of<Type>
        };
    }

    template<typename Type>
    inline constexpr auto Span<Type>::memory_view(this Span const& self) noexcept -> ice::Memory
        requires(not std::is_const_v<ValueType>)
    {
        return ice::Data{
            .location = self.data(),
            .size = self.size(),
            .alignment = ice::align_of<Type>
        };
    }

    template<typename Type, ice::u64 Size>
    inline constexpr auto make_span(std::array<Type, Size>& std_array) noexcept -> Span<Type>
    {
        return Span<Type>{ std_array.data(), std_array.size() };
    }

    template<typename Type, ice::u64 Size>
    inline constexpr auto make_span(std::array<Type, Size> const& std_array) noexcept -> Span<Type const>
    {
        return Span<Type const>{ std_array.data(), std_array.size() };
    }

    static_assert(ice::TrivialContainerLogicAllowed<ice::Span<ice::u32>>);

    // TODO: Move to a data reader type
    namespace data
    {

        template<typename T>
            requires (std::is_trivially_copyable_v<T> && !std::is_pointer_v<T>)
        inline auto read_span(
            ice::Data source,
            ice::ncount count,
            ice::Span<T const>& out_value
        ) noexcept -> ice::Data
        {
            ICE_ASSERT_CORE(source.alignment >= ice::align_of<T>);
            out_value._count = count;
            out_value._data = reinterpret_cast<T const*>(source.location);

            source.location = out_value.data() + count;
            source.size = ice::usize::subtract(source.size, out_value.size());
            source.alignment = ice::align_of<T>;
            return source;
        }

    } // namespace data

} // namespace ice
