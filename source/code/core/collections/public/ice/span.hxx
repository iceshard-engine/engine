#pragma once
#include <ice/base.hxx>
#include <span>

namespace ice
{

    template<typename T, std::size_t Extent = std::dynamic_extent>
    using Span = std::span<T, Extent>;

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

    template<typename T, std::size_t Size, template<typename, std::size_t> typename Container>
    constexpr auto make_span(Container<T, Size> const& cont) noexcept -> ice::Span<T const, Size>
    {
        return cont;
    }

} // namespace ice
