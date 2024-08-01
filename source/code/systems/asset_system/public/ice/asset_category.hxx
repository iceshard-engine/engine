/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/stringid.hxx>
#include <ice/string/string.hxx>
#include <ice/resource_flags.hxx>

namespace ice
{

    namespace detail
    {

        static constexpr bool Constant_UseAssetCategoryDebugDefinition = ice::build::is_debug || ice::build::is_develop;

        template<bool DebugImpl>
        struct AssetCategory
        {
            using ArgType = AssetCategory;

            ice::u64 identifier;
        };

        template<>
        struct AssetCategory<true>
        {
            using ArgType = const AssetCategory&;

            ice::u64 identifier;
            ice::String name;
        };

        constexpr auto make_asset_category(ice::String name) noexcept
        {
            ice::detail::murmur2_hash::mm2_x64_64 const result =
                ice::detail::murmur2_hash::cexpr_murmur2_x64_64(name, 0xAA44EELL);

            if constexpr (Constant_UseAssetCategoryDebugDefinition)
            {
                return ice::detail::AssetCategory<true>
                {
                    .identifier = result.h[0],
                    .name = name
                };
            }
            else
            {
                return ice::detail::AssetCategory<false>{
                    .identifier = result.h[0]
                };
            }
        }

        struct AssetCategoryPicker
        {
            using AssetCategory_Type = AssetCategory<ice::build::is_debug || ice::build::is_develop>;

            using AssetCategory_Arg = AssetCategory_Type::ArgType;
        };

    } // namespace detail

    using AssetCategory = ice::detail::AssetCategoryPicker::AssetCategory_Type;
    using AssetCategory_Arg = ice::detail::AssetCategoryPicker::AssetCategory_Arg;

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
