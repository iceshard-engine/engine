#pragma once
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/log_tag.hxx>

namespace ice
{

    class AssetStorage;
    class ResourceTracker;

    bool asset_tilemap_oven_tmx(
        void*,
        ice::Allocator&,
        ice::ResourceTracker const&,
        ice::Resource_v2 const&,
        ice::Data,
        ice::Memory&
    ) noexcept;

    bool asset_tilemap_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept;

    constexpr ice::LogTagDefinition LogTag_TiledOven = ice::create_log_tag(ice::LogTag::Asset, "Tiled TMX Oven");

} // namespace ice
