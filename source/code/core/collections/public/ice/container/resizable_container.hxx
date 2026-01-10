#pragma once
#include <ice/container/container_concepts.hxx>

namespace ice::container
{

    struct ResizableContainer
    {
        template<ice::concepts::ResizableContainer Self>
        constexpr void clear(this Self& self) noexcept
        {
            self.resize(0);
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
        inline void grow(this Self& self, ice::ncount min_capacity = ice::ncount_none) noexcept
        {
            ice::ncount new_capacity = self.capacity() * 2 + 4;
            if (new_capacity < min_capacity)
            {
                new_capacity = min_capacity;
            }
            self.set_capacity(new_capacity);
        }
    };

} // namespace ice
