/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/container_concepts.hxx>

namespace ice::container
{

    struct BasicContainer
    {
        template<ice::concepts::Container Self>
        constexpr bool is_empty(this Self const& self) noexcept
        {
            return self.size() == 0;
        }

        template<ice::concepts::Container Self>
        constexpr bool not_empty(this Self const& self) noexcept
        {
            return self.is_empty() == false;
        }
    };

} // namespace ice::container
