/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/math.hxx>
#include <ice/task.hxx>
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/log_tag.hxx>

namespace ice
{

    class AssetStorage;
    class ResourceTracker;

    auto asset_tilemap_oven_tmx(
        void*,
        ice::Allocator&,
        ice::ResourceTracker const&,
        ice::Resource_v2 const&,
        ice::Data,
        ice::Memory&
    ) noexcept -> ice::Task<bool>;

    auto asset_tilemap_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Metadata const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept -> ice::Task<bool>;

    constexpr ice::LogTagDefinition LogTag_TiledOven = ice::create_log_tag(ice::LogTag::Asset, "Tiled TMX Oven");

} // namespace ice
