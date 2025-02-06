/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

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

    void Trait::send(ice::Shard shard) noexcept
    {
        _context.send({ shard });
    }

    void Trait::send(ice::ShardID shardid) noexcept
    {
        _context.send({ shardid });
    }

    void Trait::send(ice::ShardID shardid, ice::Asset asset) noexcept
    {
        // We extract the pointer from the handle leaving it 'unreleased'
        //   preserving the ref-count until the trait expires the asset.
        ice::AssetHandle* handle = ice::exchange(asset._handle, nullptr);
        _context.send({ shardid | handle, detail::on_expire_asset });
    }

    void Trait::send(ice::ShardID shardid, ice::ResourceHandle handle) noexcept
    {
        // We extract the pointer from the handle leaving it 'unreleased'
        //   preserving the ref-count until the trait expires the resource.
        ice::Resource* resource = ice::exchange(handle._resource, nullptr);
        _context.send({ shardid | resource, detail::on_expire_resource });
    }

} // namespace ice
