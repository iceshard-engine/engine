#pragma once
#include <ice/base.hxx>
#include <ice/data.hxx>
#include <span>

namespace ice
{

    template<typename T, ::std::size_t Extent = ::std::dynamic_extent>
    using Span = ::std::span<T, Extent>;

    template<typename T>
    constexpr auto size(ice::Span<T> const& span) noexcept -> ice::u32
    {
        return static_cast<ice::u32>(span.size());
    }

    template<typename T, template<typename> typename Container>
    constexpr auto make_span(Container<T> const& cont) noexcept -> ice::Span<T const>
    {
        return { cont.data(), cont.size() };
    }

    template<typename T, ::std::size_t Size, template<typename, ::std::size_t> typename Container>
    constexpr auto make_span(Container<T, Size> const& cont) noexcept -> ice::Span<T const, Size>
    {
        return cont;
    }

    template<typename T, ::std::size_t Size>
    constexpr auto make_span(T(&array_obj)[Size]) noexcept -> ice::Span<T const, Size>
    {
        return { array_obj };
    }

    template<typename T>
    constexpr auto data_view(ice::Span<T> span, ice::u32 alignment = alignof(T)) noexcept -> ice::Data
    {
        return Data{
            .location = span.data(),
            .size = static_cast<ice::u32>(span.size_bytes()),
            .alignment = alignment,
        };
    }

    template<typename T>
    constexpr auto data_view(ice::Span<T const> span, ice::u32 alignment = alignof(T)) noexcept -> ice::Data
    {
        return Data{
            .location = span.data(),
            .size = static_cast<ice::u32>(span.size_bytes()),
            .alignment = alignment,
        };
    }

} // namespace ice
