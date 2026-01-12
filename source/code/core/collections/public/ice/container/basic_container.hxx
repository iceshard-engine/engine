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

        template<ice::concepts::Container Self>
            requires ice::concepts::ContiguousContainer<Self> || ice::concepts::IterableContainer<Self>
        constexpr auto first(this Self&& self) noexcept -> ice::container::ValueRef<Self>
        {
            if constexpr (ice::concepts::ContiguousContainer<Self>)
            {
                return self.data()[0];
            }
            else
            {
                return *self.begin();
            }
        }

        template<ice::concepts::Container Self>
            requires ice::concepts::ContiguousContainer<Self> || ice::concepts::ReverseIterableContainer<Self>
        constexpr auto last(this Self&& self) noexcept -> ice::container::ValueRef<Self>
        {
            if constexpr (ice::concepts::ContiguousContainer<Self>)
            {
                return self.data()[self.size() - 1];
            }
            else
            {
                return *self.rbegin();
            }
        }
    };

} // namespace ice::container
