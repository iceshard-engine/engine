/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/config.hxx>
#include <ice/log_tag.hxx>
#include <ice/math.hxx>
#include <ice/resource.hxx>
#include <ice/task.hxx>

namespace ice
{

    class AssetStorage;
    class ResourceTracker;

    auto asset_tilemap_oven_tmx(
        void*,
        ice::Allocator&,
        ice::ResourceTracker const&,
        ice::LooseResource const&,
        ice::Data,
        ice::Memory&
    ) noexcept -> ice::Task<bool>;

    auto asset_tilemap_loader(
        void*,
        ice::Allocator& alloc,
        ice::AssetStorage&,
        ice::Config const& meta,
        ice::Data data,
        ice::Memory& out_data
    ) noexcept -> ice::Task<bool>;

    constexpr ice::LogTagDefinition LogTag_TiledOven = ice::create_log_tag(ice::LogTag::Asset, "Tiled TMX Oven");

} // namespace ice
