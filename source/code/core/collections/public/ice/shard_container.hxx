/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
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

        ice::Array<ice::Shard> _data;
    };

    namespace shards
    {

        inline void reserve(ice::ShardContainer& container, ice::u32 new_capacity) noexcept;

        inline void resize(ice::ShardContainer& container, ice::u32 new_size) noexcept;

        inline void clear(ice::ShardContainer& container) noexcept;

        inline void push_back(ice::ShardContainer& container, ice::Shard value) noexcept;

        inline void push_back(ice::ShardContainer& container, ice::Span<ice::Shard const> values) noexcept;

        inline void remove_all_of(ice::ShardContainer& container, ice::Shard value) noexcept;

        inline auto begin(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator;

        inline auto end(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator;


        inline auto empty(ice::ShardContainer const& container) noexcept -> bool;

        inline auto size(ice::ShardContainer const& container) noexcept -> ice::u32;

        inline auto capacity(ice::ShardContainer const& container) noexcept -> ice::u32;

        inline auto count(ice::ShardContainer const& container, ice::Shard expected_shard) noexcept -> ice::u32;

        inline bool contains(ice::ShardContainer const& container, ice::Shard expected_shard) noexcept;

        inline auto find_first_of(ice::ShardContainer const& container, ice::Shard expected_shard, ice::u32 offset = ~0) noexcept -> ice::Shard;

        inline auto find_last_of(ice::ShardContainer const& container, ice::Shard expected_shard, ice::u32 offset = ~0) noexcept -> ice::Shard;

        template<typename T, ice::ContainerLogic Logic>
        inline auto inspect_all(ice::ShardContainer const& container, ice::Shard shard_type, ice::Array<T, Logic>& payloads) noexcept -> ice::u32;

        template<typename T, typename Fn>
        inline auto inspect_each(ice::ShardContainer const& container, ice::Shard shard_type, Fn&& callback) noexcept -> ice::u32;

        template<typename T>
        inline bool inspect_first(ice::ShardContainer const& container, ice::Shard shard, T& payload) noexcept;

        template<typename T, ice::usize::base_type Size>
        inline bool inspect_first(ice::ShardContainer const& container, ice::Shard shard_type, T(&payload)[Size]) noexcept;

        template<typename T>
        inline bool inspect_last(ice::ShardContainer const& container, ice::Shard shard, T& payload) noexcept;

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
            ice::array::reserve(container._data, new_capacity);
        }

        inline void resize(ice::ShardContainer& container, ice::u32 new_size) noexcept
        {
            ice::array::resize(container._data, new_size);
        }

        inline void clear(ice::ShardContainer& container) noexcept
        {
            ice::array::clear(container._data);
        }

        inline void push_back(ice::ShardContainer& container, ice::Shard value) noexcept
        {
            ice::array::push_back(container._data, value);
        }

        inline void push_back(ice::ShardContainer& container, ice::Span<ice::Shard const> values) noexcept
        {
            ice::array::push_back(container._data, values);
        }

        inline void remove_all_of(ice::ShardContainer& container, ice::Shard value) noexcept
        {
            ice::Array<ice::Shard>& data = container._data;
            ice::u32 count = ice::array::count(data);

            for (ice::u32 idx = 0; idx < count; ++idx)
            {
                if (data[idx] == value)
                {
                    count -= 1;
                    data[idx] = data[count];
                }
            }

            ice::array::resize(data, count);
        }

        inline auto begin(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator
        {
            return ice::array::begin(container._data);
        }

        inline auto end(ice::ShardContainer& container) noexcept -> ice::ShardContainer::Iterator
        {
            return ice::array::end(container._data);
        }


        inline auto empty(ice::ShardContainer const& container) noexcept -> bool
        {
            return ice::array::empty(container._data);
        }

        inline auto size(ice::ShardContainer const& container) noexcept -> ice::u32
        {
            return ice::array::count(container._data);
        }

        inline auto capacity(ice::ShardContainer const& container) noexcept -> ice::u32
        {
            return ice::array::capacity(container._data);
        }

        inline auto count(ice::ShardContainer const& container, ice::Shard expected_shard) noexcept -> ice::u32
        {
            ice::u32 count = 0;
            for (ice::Shard const shard : container._data)
            {
                count += (shard == expected_shard);
            }
            return count;
        }

        inline bool contains(ice::ShardContainer const& container, ice::Shard expected_shard) noexcept
        {
            return ice::shards::count(container, expected_shard) > 0;
        }

        inline auto find_first_of(ice::ShardContainer const& container, ice::Shard shard, ice::u32 offset) noexcept -> ice::Shard
        {
            auto it = ice::array::begin(container._data);
            auto const end = ice::array::end(container._data);

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

        inline auto find_last_of(ice::ShardContainer const& container, ice::Shard shard, ice::u32 offset) noexcept -> ice::Shard
        {
            auto it = ice::array::rbegin(container._data);
            auto const end = ice::array::rend(container._data);

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

        template<typename T, ice::ContainerLogic Logic>
        inline auto inspect_all(ice::ShardContainer const& container, ice::Shard shard_type, ice::Array<T, Logic>& payloads) noexcept -> ice::u32
        {
            T payload;
            ice::u32 count = 0;
            for (ice::Shard const shard : container._data)
            {
                if (shard == shard_type && ice::shard_inspect(shard, payload))
                {
                    ice::array::push_back(payloads, payload);
                    count += 1;
                }
            }
            return count;
        }

        template<typename T, typename Fn>
        inline auto inspect_each(ice::ShardContainer const& container, ice::Shard shard_type, Fn&& callback) noexcept -> ice::u32
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
        inline bool inspect_first(ice::ShardContainer const& container, ice::Shard shard_type, T& payload) noexcept
        {
            ice::Shard const shard = ice::shards::find_first_of(container, shard_type);
            return ice::shard_inspect(shard, payload);
        }

        template<typename T, ice::usize::base_type Size>
        inline bool inspect_first(ice::ShardContainer const& container, ice::Shard shard_type, T(&payload)[Size]) noexcept
        {
            auto it = ice::array::begin(container._data);
            auto const end = ice::array::end(container._data);

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
        inline bool inspect_last(ice::ShardContainer const& container, ice::Shard shard_type, T& payload) noexcept
        {
            ice::Shard const shard = ice::shards::find_last_of(container, shard_type);
            return ice::shard_inspect(shard, payload);
        }

        inline auto begin(ice::ShardContainer const& container) noexcept -> ice::ShardContainer::ConstIterator
        {
            return ice::array::begin(container._data);
        }

        inline auto end(ice::ShardContainer const& container) noexcept -> ice::ShardContainer::ConstIterator
        {
            return ice::array::end(container._data);
        }
    }

    using shards::begin;
    using shards::end;

} // namespace ice
