/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    struct ShardContainer
    {
        using Iterator = ice::Array<ice::Shard>::Iterator;
        using ConstIterator = ice::Array<ice::Shard>::ConstIterator;

        inline explicit ShardContainer(ice::Allocator& alloc) noexcept;
        inline ShardContainer(ice::ShardContainer&& other) noexcept;
        inline ShardContainer(ice::ShardContainer const& other) noexcept;
        inline ~ShardContainer() noexcept;

        inline auto operator=(ice::ShardContainer&& other) noexcept -> ice::ShardContainer&;
        inline auto operator=(ice::ShardContainer const& other) noexcept -> ice::ShardContainer&;

        inline operator ice::Span<ice::Shard const>() const noexcept { return _data; }

        ice::Array<ice::Shard> _data;
    };

    namespace shards
    {

        inline void reserve(ice::ShardContainer& container, ice::u32 new_capacity) noexcept;

        inline void resize(ice::ShardContainer& container, ice::u32 new_size) noexcept;

        inline void clear(ice::ShardContainer& container) noexcept;

        inline void push_back(ice::ShardContainer& container, ice::Shard value) noexcept;

        inline void push_back(ice::ShardContainer& container, ice::Span<ice::Shard const> values) noexcept;

        inline void remove_all_of(ice::ShardContainer& container, ice::ShardID value) noexcept;

        inline auto begin(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator;

        inline auto end(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator;


        inline auto empty(ice::ShardContainer const& container) noexcept -> bool;

        inline auto size(ice::ShardContainer const& container) noexcept -> ice::u32;

        inline auto capacity(ice::ShardContainer const& container) noexcept -> ice::u32;

        inline auto count(ice::ShardContainer const& container, ice::ShardID expected_shard) noexcept -> ice::u32;

        inline bool contains(ice::ShardContainer const& container, ice::ShardID expected_shard) noexcept;

        inline auto find_first_of(ice::ShardContainer const& container, ice::ShardID expected_shard, ice::u32 offset = ~0) noexcept -> ice::Shard;

        inline auto find_last_of(ice::ShardContainer const& container, ice::ShardID expected_shard, ice::u32 offset = ~0) noexcept -> ice::Shard;

        template<typename Fn>
        inline auto for_each(ice::ShardContainer const& container, ice::ShardID shard_type, Fn&& callback) noexcept -> ice::u32;

        template<typename Fn, typename... Args>
        inline auto for_each(ice::ShardContainer const& container, ice::ShardID shard_type, Fn&& callback, Args&&... args) noexcept -> ice::u32;

        template<typename T, ice::ContainerLogic Logic>
        inline auto inspect_all(ice::ShardContainer const& container, ice::ShardID shard_type, ice::Array<T, Logic>& payloads) noexcept -> ice::u32;

        template<typename T, typename Fn>
        inline auto inspect_each(ice::ShardContainer const& container, ice::ShardID shard_type, Fn&& callback) noexcept -> ice::u32;

        template<typename T>
        inline bool inspect_first(ice::ShardContainer const& container, ice::ShardID shard, T& payload) noexcept;

        template<typename T, ice::usize::base_type Size>
        inline bool inspect_first(ice::ShardContainer const& container, ice::ShardID shard_type, T(&payload)[Size]) noexcept;

        template<typename T>
        inline bool inspect_last(ice::ShardContainer const& container, ice::ShardID shard, T& payload) noexcept;

        inline auto begin(ice::ShardContainer const& container) noexcept -> ice::ShardContainer::ConstIterator;

        inline auto end(ice::ShardContainer const& container) noexcept -> ice::ShardContainer::ConstIterator;

    } // namespace shard

    inline ShardContainer::ShardContainer(ice::Allocator& alloc) noexcept
        : _data{ alloc }
    {
    }

    inline ShardContainer::ShardContainer(ShardContainer&& other) noexcept
        : _data{ ice::move(other._data) }
    {
    }

    inline ShardContainer::ShardContainer(ShardContainer const& other) noexcept
        : _data{ other._data }
    {
    }

    inline ShardContainer::~ShardContainer() noexcept
    {
    }

    inline auto ShardContainer::operator=(ShardContainer&& other) noexcept -> ice::ShardContainer&
    {
        if (this != &other)
        {
            _data = ice::move(other._data);
        }
        return *this;
    }

    inline auto ShardContainer::operator=(ShardContainer const& other) noexcept -> ice::ShardContainer&
    {
        if (this != &other)
        {
            _data = other._data;
        }
        return *this;
    }

    namespace shards
    {

        inline void reserve(ice::ShardContainer& container, ice::u32 new_capacity) noexcept
        {
            container._data.reserve(new_capacity);
        }

        inline void resize(ice::ShardContainer& container, ice::u32 new_size) noexcept
        {
            container._data.resize(new_size);
        }

        inline void clear(ice::ShardContainer& container) noexcept
        {
            container._data.clear();
        }

        inline void push_back(ice::ShardContainer& container, ice::Shard value) noexcept
        {
            container._data.push_back(value);
        }

        inline void push_back(ice::ShardContainer& container, ice::Span<ice::Shard const> values) noexcept
        {
            ice::array::push_back(container._data, values);
        }

        inline void remove_all_of(ice::ShardContainer& container, ice::ShardID value) noexcept
        {
            ice::Array<ice::Shard>& data = container._data;
            ice::ncount count = data.size();

            for (ice::u32 idx = 0; idx < count; ++idx)
            {
                if (data[idx] == value)
                {
                    count -= 1;
                    data[idx] = data[count];
                }
            }

            data.resize(count);
        }

        inline auto begin(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator
        {
            return container._data.begin();
        }

        inline auto end(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator
        {
            return container._data.end();
        }


        inline auto empty(ice::ShardContainer const& container) noexcept -> bool
        {
            return container._data.is_empty();
        }

        inline auto size(ice::ShardContainer const& container) noexcept -> ice::u32
        {
            return container._data.size().u32();
        }

        inline auto capacity(ice::ShardContainer const& container) noexcept -> ice::u32
        {
            return container._data.capacity().u32();
        }

        inline auto count(ice::ShardContainer const& container, ice::ShardID expected_shard) noexcept -> ice::u32
        {
            ice::u32 count = 0;
            for (ice::Shard const shard : container._data)
            {
                count += (shard == expected_shard);
            }
            return count;
        }

        inline bool contains(ice::ShardContainer const& container, ice::ShardID expected_shard) noexcept
        {
            return ice::shards::count(container, expected_shard) > 0;
        }

        inline auto find_first_of(ice::ShardContainer const& container, ice::ShardID shard, ice::u32 offset) noexcept -> ice::Shard
        {
            auto it = container._data.begin();
            auto const end = container._data.end();

            if (offset != ~0)
            {
                it = std::next(it, ice::min(offset, ice::shards::size(container)));
            }

            ice::Shard result = ice::Shard_Invalid;
            while (it != end)
            {
                if ((*it) == shard)
                {
                    result = *it;
                    it = end;
                }
                else
                {
                    it += 1;
                }
            }
            return result;
        }

        inline auto find_last_of(ice::ShardContainer const& container, ice::ShardID shard, ice::u32 offset) noexcept -> ice::Shard
        {
            auto it = container._data.rbegin();
            auto const end = container._data.rend();

            if (offset != ~0)
            {
                it = std::next(it, ice::min(offset, ice::shards::size(container)));
            }

            ice::Shard result = ice::Shard_Invalid;
            while (it != end)
            {
                if ((*it) == shard)
                {
                    result = *it;
                    it = end;
                }
                else
                {
                    it += 1;
                }
            }
            return result;
        }

        template<typename Fn>
        inline auto for_each(ice::ShardContainer const& container, ice::ShardID shard_type, Fn&& callback) noexcept -> ice::u32
        {
            ice::u32 count = 0;
            for (ice::Shard const shard : container._data)
            {
                if (shard == shard_type)
                {
                    ice::forward<Fn>(callback)(shard);
                }
            }
            return count;
        }

        template<typename Fn, typename... Args>
        inline auto for_each(ice::ShardContainer const& container, ice::ShardID shard_type, Fn&& callback, Args&&... args) noexcept -> ice::u32
        {
            ice::u32 count = 0;
            for (ice::Shard const shard : container._data)
            {
                if (shard == shard_type)
                {
                    ice::forward<Fn>(callback)(shard, ice::forward<Args>(args)...);
                }
            }
            return count;
        }

        template<typename T, ice::ContainerLogic Logic>
        inline auto inspect_all(ice::ShardContainer const& container, ice::ShardID shard_type, ice::Array<T, Logic>& payloads) noexcept -> ice::u32
        {
            T payload;
            ice::u32 count = 0;
            for (ice::Shard const shard : container._data)
            {
                if (shard == shard_type && ice::shard_inspect(shard, payload))
                {
                    payloads.push_back(payload);
                    count += 1;
                }
            }
            return count;
        }

        template<typename T, typename Fn>
        inline auto inspect_each(ice::ShardContainer const& container, ice::ShardID shard_type, Fn&& callback) noexcept -> ice::u32
        {
            T payload;
            ice::u32 count = 0;
            for (ice::Shard const shard : container._data)
            {
                if (shard == shard_type && ice::shard_inspect(shard, payload))
                {
                    ice::forward<Fn>(callback)(payload);
                }
            }
            return count;
        }

        template<typename T>
        inline bool inspect_first(ice::ShardContainer const& container, ice::ShardID shard_type, T& payload) noexcept
        {
            ice::Shard const shard = ice::shards::find_first_of(container, shard_type);
            return ice::shard_inspect(shard, payload);
        }

        template<typename T, ice::usize::base_type Size>
        inline bool inspect_first(ice::ShardContainer const& container, ice::ShardID shard_type, T(&payload)[Size]) noexcept
        {
            auto it = container._data.begin();
            auto const end = container._data.end();

            ice::u32 idx = 0;
            while (it != end && idx < Size)
            {
                if ((*it) == shard_type)
                {
                    ice::shard_inspect(*it, payload[idx]);
                    it += 1;
                    idx += 1;
                }
                else
                {
                    it += 1;
                }
            }

            return idx == Size;
        }

        template<typename T>
        inline bool inspect_last(ice::ShardContainer const& container, ice::ShardID shard_type, T& payload) noexcept
        {
            ice::Shard const shard = ice::shards::find_last_of(container, shard_type);
            return ice::shard_inspect(shard, payload);
        }

        inline auto begin(ice::ShardContainer const& container) noexcept -> ice::ShardContainer::ConstIterator
        {
            return container._data.begin();
        }

        inline auto end(ice::ShardContainer const& container) noexcept -> ice::ShardContainer::ConstIterator
        {
            return container._data.end();
        }
    }

    using shards::begin;
    using shards::end;

} // namespace ice
