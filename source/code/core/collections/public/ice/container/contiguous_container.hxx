#pragma once
#include <ice/container/basic_container.hxx>

namespace ice::container
{

    template<typename T>
    struct Span;

    struct ContiguousContainer : ice::container::BasicContainer
    {
        // Accessing Data with Spans
        template<ice::concepts::Container Self>
        constexpr auto subspan(
            this Self&& self,
            ice::nindex from,
            ice::ncount count = ice::ncount_none
        ) noexcept -> ice::container::SpanType<Self>
        {
            ice::ncount const item_count = self.size();
            ice::nindex const from_start = ice::min<ice::nindex>(from, item_count);
            ice::ncount const remaining_count = item_count - from_start;
            return { self.data() + from_start.native(), count.min_value_or(remaining_count, remaining_count)};
        }

        template<ice::concepts::Container Self>
        constexpr auto subspan(
            this Self&& self,
            ice::ref32 refval
        ) noexcept -> ice::container::SpanType<Self>
        {
            return self.subspan(refval.offset, refval.size);
        }

        template<ice::concepts::Container Self>
        constexpr auto headspan(
            this Self&& self,
            ice::ncount count = 1
        ) noexcept -> ice::container::SpanType<Self>
        {
            // If 'count' is not valid we default to '0' so the returned span is empty.
            return { self.data(), count.min_value_or(self.size(), 0_count) };
        }

        template<ice::concepts::Container Self>
        constexpr auto tailspan(
            this Self&& self,
            ice::nindex offset = 1
        ) noexcept -> ice::container::SpanType<Self>
        {
            ice::nindex const max_offset = self.size();
            offset = offset.min_value_or(max_offset, max_offset);

            // If 'offset' is not valid we default to 'max_size' so the returned span is empty.
            return { self.data() + offset, max_offset - offset };
        }

        // Iteration interface
        template<ice::concepts::Container Self>
        constexpr auto begin(this Self&& self) noexcept -> ice::container::Iterator<Self>
        {
            return { self.data() };
        }

        template<ice::concepts::Container Self>
        constexpr auto end(this Self&& self) noexcept -> ice::container::Iterator<Self>
        {
            return { self.data() + self.size() };
        }

        template<ice::concepts::Container Self>
        constexpr auto rbegin(this Self&& self) noexcept -> ice::container::ReverseIterator<Self>
        {
            return ice::container::ReverseIterator<Self>{ self.data() + self.size() };
        }

        template<ice::concepts::Container Self>
        constexpr auto rend(this Self&& self) noexcept -> ice::container::ReverseIterator<Self>
        {
            return ice::container::ReverseIterator<Self>{ self.data() };
        }

        // Operators
        template<ice::concepts::Container Self>
        constexpr auto operator[](this Self&& self, ice::nindex index) noexcept -> ice::container::ValueRef<Self>
        {
            return self.data()[index];
        }

        // Data API
        template<ice::concepts::Container Self>
        inline auto meminfo(this Self const& self) noexcept -> ice::meminfo
        {
            return ice::meminfo_of<typename Self::ValueType> * self.size();
        }

        template<ice::concepts::ResizableContainer Self> requires (ice::concepts::TrivialContainerLogic<Self>)
        inline auto memset(this Self const& self, ice::u8 value) noexcept -> ice::Memory
        {
            ice::Memory mem{ self.data(), self.size(), ice::align_of<ice::container::ValueType<Self>> };
            ice::memset(mem.location, value, mem.size.value);
            return mem;
        }
    };

} // namespace ice::container
