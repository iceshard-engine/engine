/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_types.hxx>
#include <ice/asset_category.hxx>

namespace ice
{

    static constexpr ice::AssetCategory AssetCategory_StaticObject = ice::make_asset_category("ice/object/static-object");
    static constexpr ice::AssetCategory AssetCategory_DynamicObject = ice::make_asset_category("ice/object/dynamic-object");
    static constexpr ice::AssetCategory AssetCategory_Tileset = ice::make_asset_category("ice/object/tileset");

} // namespace ice
