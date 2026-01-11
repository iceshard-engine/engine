/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string/editable_operations.hxx>

namespace ice::string
{

    using ice::concepts::ResizableStringType;

    struct ResizableOperations : ice::string::MutableOperations
    {
        // Capacity and Size Helpers

        template<ResizableStringType Self>
        inline void shrink(this Self& self) noexcept
        {
            self.set_capacity(self.size() + 1);
        }

        template<ResizableStringType Self>
        inline void reserve(this Self& self, ice::ncount min_capacity) noexcept
        {
            if (min_capacity > self.capacity())
            {
                self.set_capacity(min_capacity);
            }
        }

        template<ResizableStringType Self>
        inline void grow(this Self& self, ice::ncount min_capacity = ncount_none) noexcept
        {
            ice::ncount const new_capacity = ice::max(self.capacity() * 2 + 8, min_capacity);
            self.set_capacity(new_capacity);
        }
    };

} // namespace ice::string
