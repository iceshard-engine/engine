#pragma once
#include <ice/container/basic_container.hxx>

namespace ice::container
{

    struct AssociativeContainer : ice::container::BasicContainer
    {
        template<ice::concepts::AssociativeContainer Self>
        bool has(this Self const& self, ice::container::KeyTypeArg<Self> key) noexcept
        {
            return self.find(key) != nullptr;
        }

        template<ice::concepts::AssociativeContainer Self, ice::concepts::HashableKeyType KeyType>
        bool has(this Self const& self, KeyType&& key) noexcept
        {
            return self.has(ice::hash(ice::forward<KeyType>(key)));
        }

        template<ice::concepts::AssociativeContainer Self>
        bool missing(this Self const& self, ice::container::KeyTypeArg<Self> key) noexcept
        {
            return self.find(key) == nullptr;
        }

        template<ice::concepts::AssociativeContainer Self, ice::concepts::HashableKeyType KeyType>
        bool missing(this Self const& self, KeyType&& key) noexcept
        {
            return self.missing(ice::hash(ice::forward<KeyType>(key)));
        }
    };

} // namespace ice::container
