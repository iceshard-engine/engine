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

} // namespace ice
