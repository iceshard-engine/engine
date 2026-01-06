#pragma once
#include <ice/container/container_concepts.hxx>

namespace ice::container
{

    struct ContiguousContainer
    {
        template<ice::concepts::ContiguousContainer Self>
        constexpr auto front(this Self& self) noexcept -> ice::container::ValueRef<Self>
        {
            return self.data()[0];
        }
    };

} // namespace ice::container
