#pragma once
#include <ice/container/container_concepts.hxx>

namespace ice::container
{

    template<typename T>
    struct Span;

    struct ContiguousContainer
    {
        template<ice::concepts::ContiguousContainer Self>
        constexpr bool is_empty(this Self const& self) noexcept
        {
            return self.size() == 0;
        }

        template<ice::concepts::ContiguousContainer Self>
        constexpr bool not_empty(this Self const& self) noexcept
        {
            return self.is_empty() == false;
        }

        template<ice::concepts::ContiguousContainer Self>
        constexpr auto first(this Self&& self) noexcept -> ice::container::ValueRef<Self>
        {
            return self.data()[0];
        }

        template<ice::concepts::ContiguousContainer Self>
        constexpr auto last(this Self&& self) noexcept -> ice::container::ValueRef<Self>
        {
            return self.data()[self.size() - 1];
        }

        // Accessing Data with Spans
        template<ice::concepts::ContiguousContainer Self>
        constexpr auto subspan(
            this Self&& self,
            ice::nindex from,
            ice::ncount count = ice::ncount_none
        ) noexcept -> ice::container::SpanType<Self>
        {
            ice::ncount const item_count = self.size();
            ice::nindex const from_start = ice::min<ice::nindex>(from, item_count);
            ice::ncount const remaining_count = item_count - from_start;
            return { self.data() + from_start, count.min_value_or(remaining_count, remaining_count) };
        }

        template<ice::concepts::ContiguousContainer Self>
        constexpr auto subspan(
            this Self&& self,
            ice::ref32 refval
        ) noexcept -> ice::container::SpanType<Self>
        {
            return self.subspan(refval.offset, refval.size);
        }

        template<ice::concepts::ContiguousContainer Self>
        constexpr auto headspan(
            this Self&& self,
            ice::ncount count = 1
        ) noexcept -> ice::container::SpanType<Self>
        {
            // If 'count' is not valid we default to '0' so the returned span is empty.
            return { self.data(), count.min_value_or(self.size(), 0_count) };
        }

        template<ice::concepts::ContiguousContainer Self>
        constexpr auto tailspan(
            this Self&& self,
            ice::nindex offset = 1
        ) noexcept -> ice::container::SpanType<Self>
        {
            ice::nindex const max_offset = self.size();
            offset = offset.min_value_or(max_offset, max_offset);

            // If 'offset' is not valid we default to 'max_size' so the returned span is empty.
            return { self.data() + max_offset, max_offset - offset };
        }

        // Operators
        template<ice::concepts::ContiguousContainer Self>
        constexpr auto operator[](this Self&& self, ice::nindex index) noexcept -> ice::container::ValueRef<Self>
        {
            return self.data()[index];
        }
    };

} // namespace ice::container
