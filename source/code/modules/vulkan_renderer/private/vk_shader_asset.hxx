/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os.hxx>
#include <ice/module.hxx>
#include <ice/module_query.hxx>
#include <ice/resource_compiler.hxx>
#include <ice/asset_module.hxx>

namespace ice::render::vk
{

    struct VkShaderAssetModule : ice::Module<VkShaderAssetModule>
    {
        static void v1_archive_api(ice::detail::asset_system::v1::AssetArchiveAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            return negotiator.register_api(v1_archive_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(VkShaderAssetModule);
    };

} // namespace ice::render::vk
