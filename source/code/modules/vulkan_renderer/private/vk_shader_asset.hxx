/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
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
        static ice::api::resource_compiler::v1::ResourceCompilerAPI compiler_api;

        static void v1_archive_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            negotiator.query_api(compiler_api);

            return negotiator.register_api(v1_archive_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(VkShaderAssetModule);
    };

} // namespace ice::render::vk
