/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/stringid.hxx>
#include <ice/string/string.hxx>
#include <ice/resource_flags.hxx>
#include <ice/asset_category_details.hxx>

namespace ice
{

    constexpr auto make_asset_category(ice::String name) noexcept -> ice::AssetCategory;

    constexpr auto asset_category_hint(ice::detail::AssetCategory<false>) noexcept -> ice::String;

    constexpr auto asset_category_hint(ice::detail::AssetCategory<true> const& category) noexcept -> ice::String;



    constexpr auto make_asset_category(ice::String name) noexcept -> ice::AssetCategory
    {
        return ice::detail::make_asset_category(name);
    }

    constexpr auto asset_category_hint(ice::detail::AssetCategory<false>) noexcept -> ice::String
    {
        return "<no_debug_info_available>";
    }

    constexpr auto asset_category_hint(ice::detail::AssetCategory<true> const& category) noexcept -> ice::String
    {
        return category.name;
    }

} // namespace ice
