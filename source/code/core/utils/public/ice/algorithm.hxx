/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <algorithm>
#include <numeric>

namespace ice
{

    template<typename T, typename U = T>
    constexpr auto accumulate(ice::Span<T const> range, U val) noexcept
    {
        return ::std::accumulate(ice::begin(range), ice::end(range), val);
    }

} // namespace ice
