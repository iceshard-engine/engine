/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/shard.hxx>

namespace ice
{

    namespace detail
    {

        void on_expire_resource(void*, ice::Shard shard) noexcept
        {
            // We create an empty handle and set the resource pointer so when getting out of scope it will properly clean-up.
            ice::ResourceHandle handle{};
            handle._resource = ice::shard_shatter<ice::Resource*>(shard, nullptr);
        }

        void on_expire_asset(void*, ice::Shard shard) noexcept
        {
            // We create an empty handle and set the handle pointer so when getting out of scope it will properly clean-up.
            ice::Asset asset{};
            asset._handle = ice::shard_shatter<ice::AssetHandle*>(shard, nullptr);
        }

    } // namespace detail

    Trait::Trait(ice::TraitContext& context) noexcept
        : _context{ context }
    {
    }

    void Trait::send(ice::Shard shard, SendMode mode) noexcept
    {
        _context.send({ shard, {}, {}, mode });
    }

    void Trait::send(ice::ShardID shardid, SendMode mode) noexcept
    {
        _context.send({ ice::Shard{ shardid }, {}, {}, mode });
    }

    void Trait::send(ice::ShardID shardid, ice::String value, SendMode mode) noexcept
    {
        this->send(shardid | ice::string::begin(value), mode);
    }

    void Trait::send(ice::ShardID shardid, ice::Asset asset, SendMode mode) noexcept
    {
        // We extract the pointer from the handle leaving it 'unreleased'
        //   preserving the ref-count until the trait expires the asset.
        ice::AssetHandle* handle = ice::exchange(asset._handle, nullptr);
        _context.send({ shardid | handle, detail::on_expire_asset, nullptr, mode });
    }

    void Trait::send(ice::ShardID shardid, ice::ResourceHandle handle, SendMode mode) noexcept
    {
        // We extract the pointer from the handle leaving it 'unreleased'
        //   preserving the ref-count until the trait expires the resource.
        ice::Resource* resource = ice::exchange(handle._resource, nullptr);
        _context.send({ shardid | resource, detail::on_expire_resource, nullptr, mode });
    }

    auto Trait::entities() noexcept -> ice::ecs::EntityIndex&
    {
        return _context.world().entities();
    }

    auto Trait::entity_operations() noexcept -> ice::ecs::EntityOperations&
    {
        return _context.world().entity_operations();
    }

    auto Trait::entity_queries() noexcept -> ice::ecs::QueryStorage&
    {
        return _context.world().entity_queries_storage();
    }

} // namespace ice
