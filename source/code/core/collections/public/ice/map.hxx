#pragma once
#include <ice/collections.hxx>

namespace ice
{

    namespace map
    {

        template<typename K, typename V>
        inline void reserve(ice::Map<K, V>& map, ice::u32 size) noexcept;

        template<typename K, typename V>
        inline void clear(ice::Map<K, V>& map) noexcept;

        template<typename K, typename V>
        inline void set(ice::Map<K, V>& map, K const& key, V&& value) noexcept;

        template<typename K, typename V>
        inline void remove(ice::Map<K, V>& map, K const& key) noexcept;

        template<typename K, typename V>
        inline bool has(ice::Map<K, V> const& map, K const& key) noexcept;

        template<typename K, typename V>
        inline auto get(ice::Map<K, V> const& map, K const& key, V const& fallback_value) noexcept -> V const&;

        template<typename K, typename V>
        inline auto get(ice::Map<K, V*> const& map, K const& key, nullptr_t) noexcept -> V*;

        template<typename K, typename V>
        inline auto begin(ice::Map<K, V> const& map) noexcept -> typename ice::Map<K, V>::const_iterator_type;

        template<typename K, typename V>
        inline auto end(ice::Map<K, V> const& map) noexcept -> typename ice::Map<K, V>::const_iterator_type;



        template<typename K, typename V>
        inline void reserve(ice::Map<K, V>& map, ice::u32 size) noexcept
        {
            map.reserve(size);
        }

        template<typename K, typename V>
        inline void clear(ice::Map<K, V>& map) noexcept
        {
            map.clear();
        }

        template<typename K, typename V>
        inline void set(ice::Map<K, V>& map, K const& key, V&& value) noexcept
        {
            map.emplace(key, ice::forward<V>(value));
        }

        template<typename K, typename V>
        inline void remove(ice::Map<K, V>& map, K const& key) noexcept
        {
            map.erase(key);
        }

        template<typename K, typename V>
        inline bool has(ice::Map<K, V> const& map, K const& key) noexcept
        {
            return map.contains(key);
        }

        template<typename K, typename V>
        inline auto get(ice::Map<K, V> const& map, K const& key, V const& fallback_value) noexcept -> V const&
        {
            if (ice::map::has(map, key))
            {
                return map.at(key);
            }
            else
            {
                return fallback_value;
            }
        }

        template<typename K, typename V>
        inline auto get(ice::Map<K, V*> const& map, K const& key, nullptr_t) noexcept -> V*
        {
            if (ice::map::has(map, key))
            {
                return map.at(key);
            }
            else
            {
                return nullptr;
            }
        }

        template<typename K, typename V>
        inline auto begin(ice::Map<K, V> const& map) noexcept -> typename ice::Map<K, V>::const_iterator_type
        {
            return map.begin();
        }

        template<typename K, typename V>
        inline auto end(ice::Map<K, V> const& map) noexcept -> typename ice::Map<K, V>::const_iterator_type
        {
            return map.end();
        }

    } // namespace hash

    using map::begin;
    using map::end;

} // namespace ice::pod
