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
        constexpr auto first(this Self&& self) noexcept -> ice::container::ValueRef<Self>
        {
            return self.data()[0];
        }

        template<ice::concepts::Container Self>
        constexpr auto last(this Self&& self) noexcept -> ice::container::ValueRef<Self>
        {
            return self.data()[self.size() - 1];
        }
    };

} // namespace ice::container
