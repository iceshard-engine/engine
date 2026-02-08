/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/container_concepts.hxx>

namespace ice::container
{

    struct ResizableContainer
    {
        template<ice::concepts::ResizableContainer Self>
        constexpr bool is_full(this Self const& self) noexcept
        {
            return self.size() == self.capacity();
        }

        template<ice::concepts::ResizableContainer Self>
        constexpr bool not_full(this Self const& self) noexcept
        {
            return self.size() < self.capacity();
        }

        template<ice::concepts::ResizableContainer Self>
        constexpr void reserve(this Self& self, ice::ncount min_capacity) noexcept
        {
            if (self.capacity() < min_capacity)
            {
                self.set_capacity(min_capacity);
            }
        }

        template<ice::concepts::ResizableContainer Self>
        constexpr void grow(this Self& self, ice::ncount min_capacity = ice::ncount_none) noexcept
        {
            ice::ncount new_capacity = self.capacity() * 2 + 4;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }
            self.set_capacity(new_capacity);
        }

        template<ice::concepts::ResizableContainer Self>
        constexpr void shrink(this Self& self) noexcept
        {
            self.set_capacity(self.size());
        }
    };

} // namespace ice
