/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/input_action_definitions.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/asset_category.hxx>
#include <ice/asset_module.hxx>
#include <ice/module.hxx>

namespace ice
{

    static constexpr ice::AssetCategory AssetCategory_InputActionsScript = ice::make_asset_category("ice/input_actions/script");

    struct InputActionsModule : public ice::Module<InputActionsModule>
    {
        static void v1_archive_api(ice::detail::asset_system::v1::AssetArchiveAPI& api) noexcept;
        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto& negotiator) noexcept;

        ICE_WORKAROUND_MODULE_INITIALIZATION(InputActionsModule);
    };

} // namespace ice
