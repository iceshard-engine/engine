/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/module_query.hxx>
#include <ice/asset_types.hxx>

namespace ice
{

    void load_asset_category_definitions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetCategoryArchive& category_archive
    ) noexcept;

    namespace detail::asset_system::v1
    {

        using RegisterTypesFn = void (ice::AssetCategoryArchive&, ice::ModuleQuery const&) noexcept;

        struct AssetArchiveAPI
        {
            static constexpr ice::StringID Constant_APIName = "ice.asset-category-archive"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            RegisterTypesFn* fn_register_categories;
        };

    } // detail::engine::v1

} // namespace ice
