/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

namespace ice
{

    namespace queue
    {

        template<typename Type, ice::ContainerLogic Logic>
        inline auto take_front(ice::Queue<Type, Logic>& queue, ice::Span<Type> out_values) noexcept -> ice::u32
        {
            ice::ncount const taken_items = ice::min(out_values.size(), queue.size());

            // (offset, end][0, remaining)
            ice::ncount const first_part = ice::min<ice::ncount>(queue._offset + taken_items, queue._capacity);
            ice::ncount const second_part = (queue._offset + taken_items) - first_part;
            ice::ncount const first_part_count = first_part - queue._offset;

            if constexpr (Logic == ContainerLogic::Complex)
            {
                ice::mem_move_n_to(out_values.begin(), queue._data + queue._offset, first_part_count);
                ice::mem_move_n_to(out_values.begin() + first_part_count, queue._data, second_part);
            }
            else
            {
                ice::memcpy(out_values.begin(), queue._data + queue._offset, ice::size_of<Type> * first_part_count);
                ice::memcpy(out_values.begin() + first_part_count, queue._data, ice::size_of<Type> * second_part);
            }

            queue.pop_front(taken_items.u32());
            return taken_items.u32();
        }

#if 0
        template<typename Type, ice::ContainerLogic Logic, typename Fn>
        inline void for_each(ice::Queue<Type, Logic> const& queue, Fn&& fn) noexcept
        {
            if (queue._count == 0)
            {
                return;
            }

            ice::u32 const first_part = ice::min(queue._offset + queue._count, queue._capacity);
            ice::u32 const second_part = (queue._offset + queue._count) - first_part;

            for (ice::u32 idx = queue._offset; idx < first_part; ++idx)
            {
                ice::forward<Fn>(fn)(queue._data[idx]);
            }

            for (ice::u32 idx = 0; idx < second_part; ++idx)
            {
                ice::forward<Fn>(fn)(queue._data[idx]);
            }
        }

        template<typename Type, ice::ContainerLogic Logic, typename Fn>
        inline void for_each_reverse(ice::Queue<Type, Logic> const& queue, Fn&& fn) noexcept
        {
            if (queue._count == 0)
            {
                return;
            }

            ice::u32 const first_part = ice::min(queue._offset + queue._count, queue._capacity);
            ice::u32 const second_part = (queue._offset + queue._count) - first_part;

            if (second_part > 0)
            {
                for (ice::u32 idx = second_part - 1; idx > 0; --idx)
                {
                    ice::forward<Fn>(fn)(queue._data[idx]);
                }

                ice::forward<Fn>(fn)(queue._data[0]);
            }

            for (ice::u32 idx = first_part - 1; idx > queue._offset; --idx)
            {
                ice::forward<Fn>(fn)(queue._data[idx]);
            }

            ice::forward<Fn>(fn)(queue._data[queue._offset]);
        }
#endif

    } // namespace queue

} // namespace ice
