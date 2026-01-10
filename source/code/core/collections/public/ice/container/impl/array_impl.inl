#include "array.hxx"
/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

namespace ice
{

    namespace array
    {

        template<typename Type, ice::ContainerLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_back(ice::Array<Type, Logic>& arr, ice::Array<Type, Logic> const& items) noexcept
        {
            return ice::array::push_back(arr, items.tailspan(0));
        }

        template<typename Type, ice::ContainerLogic Logic>
            requires std::copy_constructible<Type>
        inline void push_back(ice::Array<Type, Logic>& arr, ice::Span<Type const> items) noexcept
        {
            ice::u32 const required_capacity = arr._count + items.size().u32();
            if (required_capacity > arr._capacity)
            {
                arr.grow(required_capacity);
            }

            ice::u32 const missing_items = required_capacity - arr._count;
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_copy_construct_n_at<Type>(
                    Memory{ .location = arr._data + arr._count, .size = ice::size_of<Type> * missing_items, .alignment = ice::align_of<Type> },
                    items.data(),
                    items.size().u32()
                );
            }
            else
            {
                ice::memcpy(
                    Memory{ .location = arr._data + arr._count, .size = ice::size_of<Type> * missing_items, .alignment = ice::align_of<Type> },
                    items.data_view()
                );
            }

            arr._count += items.size().u32();
        }

        template<typename Type, ice::ContainerLogic Logic, typename Source>
            requires std::copy_constructible<Type> && (std::is_same_v<Type, Source> == false)
        inline void push_back(ice::Array<Type, Logic>& arr, ice::Span<Source const> items, Type(*fn)(Source const&) noexcept) noexcept
        {
            ice::u32 const required_capacity = arr._count + items.size();
            if (required_capacity > arr._capacity)
            {
                arr.grow(required_capacity);
            }

            ice::u32 const missing_items = required_capacity - arr._count;
            for (ice::u32 src_idx = 0; src_idx < missing_items; ++src_idx)
            {
                ice::array::push_back(arr, fn(items[src_idx]));
            }
        }

        template<typename Type, ice::ContainerLogic Logic>
        inline void pop_back(ice::Array<Type, Logic>& arr, ice::u32 count /*= 1*/) noexcept
        {
            count = ice::min(count, arr._count);
            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_destruct_n_at(arr._data + (arr._count - count), count);
            }

            if (arr._count > count)
            {
                arr._count -= count;
            }
            else
            {
                arr._count = 0;
            }
        }

    } // namespace array

} // namespace ice
