#pragma once
#include <ice/shard.hxx>
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    struct ShardContainer
    {
        explicit ShardContainer(ice::Allocator& alloc) noexcept;
        ShardContainer(ShardContainer&& other) noexcept;
        ShardContainer(ShardContainer const& other) noexcept;
        ~ShardContainer() noexcept;

        auto operator=(ShardContainer&& other) noexcept -> ice::ShardContainer&;
        auto operator=(ShardContainer const& other) noexcept -> ice::ShardContainer&;

        ice::pod::Array<ice::Shard> _data;
    };

    namespace shard
    {

        void reserve(ShardContainer& container, ice::u32 new_capacity) noexcept;

        void resize(ShardContainer& container, ice::u32 new_size) noexcept;

        void push_back(ShardContainer& container, ice::Shard value) noexcept;

        void push_back(ShardContainer& container, ice::Span<ice::Shard const> values) noexcept;

        void transform_all(ShardContainer& container, ice::Shard old_shard, ice::Shard new_shard) noexcept;

        void remove_all(ShardContainer& container, ice::Shard value) noexcept;


        auto empty(ShardContainer const& container) noexcept -> bool;

        auto size(ShardContainer const& container) noexcept -> ice::u32;

        bool contains(ShardContainer const& container, ice::Shard shard) noexcept;

        auto count(ShardContainer const& container, ice::Shard shard) noexcept -> ice::u32;

        auto find_first_of(ShardContainer const& container, ice::Shard shard, ice::u32 offset = ~0) noexcept -> ice::Shard;

        auto find_last_of(ShardContainer const& container, ice::Shard shard, ice::u32 offset = ~0) noexcept -> ice::Shard;

        template<typename T>
        auto inspect_all(ShardContainer const& container, ice::Shard shard, ice::pod::Array<T>& payloads) noexcept -> ice::u32;

        template<typename T>
        bool inspect_first(ShardContainer const& container, ice::Shard shard, T& payload) noexcept;

        template<typename T>
        bool inspect_last(ShardContainer const& container, ice::Shard shard, T& payload) noexcept;

    } // namespace shard

} // namespace ice
