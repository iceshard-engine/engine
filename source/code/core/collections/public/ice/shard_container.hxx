/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/shard.hxx>
#include <ice/array.hxx>

namespace ice
{

    struct ShardContainer : public ice::Array<ice::Shard>
    {
        using ice::Array<ice::Shard>::Array;
        using ice::Array<ice::Shard>::push_back;

        constexpr bool contains(ice::ShardID shardid) const noexcept;
        constexpr auto count_of(ice::ShardID shardid) const noexcept -> ice::ncount;

        constexpr auto find_first_of(
            ice::ShardID shardid,
            ice::nindex offset = ice::nindex_none
        ) const noexcept -> ice::Shard;

        constexpr auto find_last_of(
            ice::ShardID shardid,
            ice::nindex offset = ice::nindex_none
        ) const noexcept -> ice::Shard;

        template<typename Fn, typename... Args>
        inline constexpr auto for_each(
            ice::ShardID shardid,
            Fn&& callback,
            Args&&... args
        ) const noexcept -> ice::ncount;

        template<typename T>
        inline constexpr bool inspect_first(
            ice::ShardID shardid,
            T& payload
        ) const noexcept;

        template<typename T>
        inline constexpr bool inspect_last(
            ice::ShardID shard,
            T& payload
        ) const noexcept;

        template<typename T, ice::ContainerLogic Logic>
        inline constexpr auto inspect_all(
            ice::ShardID shardid,
            ice::Array<T, Logic>& payloads
        ) const noexcept -> ice::ncount;

        template<typename T, typename Fn>
        inline constexpr auto inspect_each(
            ice::ShardID shardid,
            Fn&& callback
        ) noexcept -> ice::ncount;

        inline constexpr void remove_all_of(
            this ShardContainer& self,
            ice::ShardID shardid
        ) noexcept;

        template<std::size_t Count>
        inline constexpr void push_back(ice::Shard const(&shards_array)[Count]) noexcept;
    };

    inline constexpr bool ShardContainer::contains(ice::ShardID expected_shard) const noexcept
    {
        return this->find_first_of(expected_shard) != Shard_Invalid;
    }

    inline constexpr auto ShardContainer::count_of(ice::ShardID shardid) const noexcept -> ice::ncount
    {
        ice::u32 count = 0;
        for (ice::Shard const shard : (*this))
        {
            count += (shard == shardid);
        }
        return { count, sizeof(ice::Shard) };
    }

    inline constexpr auto ice::ShardContainer::find_first_of(
        ice::ShardID shardid,
        ice::nindex offset
    ) const noexcept -> ice::Shard
    {
        for (ice::Shard shard : tailspan(offset.min_value_or(size(), 0_index)))
        {
            if (shard == shardid)
            {
                return shard;
            }
        }
        return ice::Shard_Invalid;
    }

    inline constexpr auto ShardContainer::find_last_of(
        ice::ShardID shardid,
        ice::nindex offset
    ) const noexcept -> ice::Shard
    {
        ice::ncount const size = this->size();
        ice::Span const headlist = this->headspan(size - offset.min_value_or(size, 0_count));

        auto it = headlist.rbegin();
        auto const end = headlist.rend();
        while (it != end && *it != shardid)
        {
            it += 1;
        }
        return it != end ? *it : Shard_Invalid;
    }

    template<typename Fn, typename... Args>
    inline constexpr auto ShardContainer::for_each(
        ice::ShardID shardid,
        Fn&& callback,
        Args&&... args
    ) const noexcept -> ice::ncount
    {
        ice::u32 count = 0;
        for (ice::Shard const shard : this->tailspan(0))
        {
            if (shard == shardid)
            {
                ice::forward<Fn>(callback)(shard, ice::forward<Args>(args)...);
            }
        }
        return { count, sizeof(ice::Shard) };
    }

    template<typename T, ice::ContainerLogic Logic>
    inline constexpr auto ShardContainer::inspect_all(
        ice::ShardID shardid,
        ice::Array<T, Logic>& payloads
    ) const noexcept -> ice::ncount
    {
        T payload;
        ice::u32 count = 0;
        for (ice::Shard const shard : this->tailspan(0))
        {
            if (shard == shardid && ice::shard_inspect(shard, payload))
            {
                payloads.push_back(payload);
                count += 1;
            }
        }
        return { count, sizeof(ice::ShardID) };
    }

    template<typename T>
    inline constexpr bool ShardContainer::inspect_first(
        ice::ShardID shardid,
        T& payload
    ) const noexcept
    {
        ice::Shard const shard = this->find_first_of(shardid);
        return ice::shard_inspect(shard, payload);
    }

    template<typename T>
    inline constexpr bool ShardContainer::inspect_last(
        ice::ShardID shardid,
        T& payload
    ) const noexcept
    {
        ice::Shard const shard = this->find_last_of(shardid);
        return ice::shard_inspect(shard, payload);
    }

    template<typename T, typename Fn>
    inline constexpr auto ShardContainer::inspect_each(
        ice::ShardID shardid,
        Fn&& callback
    ) noexcept -> ice::ncount
    {
        T payload;
        ice::u32 count = 0;
        for (ice::Shard const shard : this->tailspan(0))
        {
            if (shard == shardid && ice::shard_inspect(shard, payload))
            {
                ice::forward<Fn>(callback)(payload);
            }
        }
        return { count, sizeof(ice::Shard) };
    }

    inline constexpr void ShardContainer::remove_all_of(
        this ShardContainer& self,
        ice::ShardID shardid
    ) noexcept
    {
        // We move shards from the end of the array to the locations we want to removed.
        //   Finaly we resize the array ensuring the tail values are no longer accessed.
        ice::u32 count = self.size().u32();
        for (ice::u32 idx = 0; idx < count; ++idx)
        {
            if (self[idx] == shardid)
            {
                count -= 1;
                self[idx] = self[count];
            }
        }
        self.resize(count);
    }

    template<std::size_t Count>
    inline constexpr void ShardContainer::push_back(ice::Shard const(&shards_array)[Count]) noexcept
    {
        this->push_back(ice::Span{ shards_array });
    }

} // namespace ice
