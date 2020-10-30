#pragma once
#include <ice/base.hxx>
#include <span>

namespace ice
{

    template<typename T, std::size_t Extent = std::dynamic_extent>
    using Span = std::span<T, Extent>;

} // namespace ice
