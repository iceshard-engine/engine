/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/basic_container.hxx>

namespace ice::container
{

    struct AssociativeContainer : ice::container::BasicContainer
    {
        template<ice::concepts::AssociativeContainer Self, ice::concepts::HashableKeyType KeyType>
        bool has(this Self const& self, KeyType const& key) noexcept
        {
            return self.find(ice::hash(key)) != nullptr;
        }

        template<ice::concepts::AssociativeContainer Self, ice::concepts::HashableKeyType KeyType>
        bool missing(this Self const& self, KeyType const& key) noexcept
        {
            return self.find(ice::hash(key)) == nullptr;
        }

        template<
            ice::concepts::AssociativeContainer Self,
            ice::concepts::HashableKeyType KeyType,
            typename InValueType = typename std::remove_reference_t<Self>::ValueType
        >
        auto get(
            this Self const& self,
            KeyType const& key,
            InValueType const& fallback_value
        ) noexcept -> ice::container::ValueRef<Self const>
        {
            ice::container::ValuePtr<Self const> result = self.find(ice::hash(key));
            // TODO: We need to rethink all 'get' methods since
            //   we will always run into lifetime issue with this approach.
            return result != nullptr ? *result : fallback_value;
        }

        template<
            ice::concepts::AssociativeContainer Self,
            ice::concepts::HashableKeyType KeyType>
        auto try_get(
            this Self&& self,
            KeyType const& key
        ) noexcept -> ice::container::ValuePtr<Self>
        {
            return self.find(ice::hash(key));
        }
    };


    struct AssociativeResizableContainer : ice::container::AssociativeContainer
    {
        template<
            ice::concepts::AssociativeResizableContainer Self,
            ice::concepts::HashableKeyType KeyType,
            typename InValueType = typename std::remove_reference_t<Self>::ValueType
        >
            requires(std::convertible_to<InValueType, typename Self::ValueType>)
        auto set(
            this Self& self,
            KeyType const& key,
            InValueType&& in_value
        ) noexcept -> ice::container::ValueRef<Self>
        {
            return self.store(ice::hash(key), std::forward<InValueType>(in_value));
        }

        template<
            ice::concepts::AssociativeResizableContainer Self,
            ice::concepts::HashableKeyType KeyType,
            typename InValueType = typename std::remove_reference_t<Self>::ValueType
        >
            requires(std::convertible_to<InValueType, typename Self::ValueType>)
        void set_if_missing(
            this Self& self,
            KeyType const& key,
            InValueType&& in_value
        ) noexcept
        {
            ice::u64 const key_hash = ice::hash(key);
            if (self.missing(key_hash))
            {
                self.set(key_hash, std::forward<InValueType>(in_value));
            }
        }

        template<
            ice::concepts::AssociativeContainer Self,
            ice::concepts::HashableKeyType KeyType,
            typename InValueType = typename std::remove_reference_t<Self>::ValueType
        >
        auto get_or_set(
            this Self&& self,
            KeyType const& key,
            InValueType&& in_value
        ) noexcept -> ice::container::ValueRef<Self>
        {
            ice::u64 const key_hash = ice::hash(key);
            if (self.missing(key_hash))
            {
                return self.set(key_hash, std::forward<InValueType>(in_value));
            }
            else
            {
                ice::container::ValuePtr<Self> result = self.try_get(key_hash);
                ICE_ASSERT_CORE(result != nullptr);
                return *result;
            }
        }

        template<
            ice::concepts::AssociativeContainer Self,
            ice::concepts::HashableKeyType KeyType>
        bool remove(this Self&& self, KeyType const& key) noexcept
        {
            return self.remove(ice::hash(key));
        }
    };

} // namespace ice::container
