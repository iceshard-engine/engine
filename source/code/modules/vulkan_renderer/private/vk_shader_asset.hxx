/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os.hxx>
#if ISP_WINDOWS
#include <ice/module.hxx>
#include <ice/resource_compiler.hxx>
#include <ice/asset_module.hxx>

namespace ice::render::vk
{

    struct VkShaderCompilerModule : ice::Module<VkShaderCompilerModule>
    {
        static void v1_resource_compiler_api(ice::api::resource_compiler::v1::ResourceCompilerAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& module_negotiator) noexcept
        {
            return module_negotiator.register_api(v1_resource_compiler_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(VkShaderCompilerModule);
    };

    struct VkShaderAssetModule : ice::Module<VkShaderAssetModule>
    {
        static void v1_archive_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator const& negotiator) noexcept
        {
            return negotiator.register_api(v1_archive_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(VkShaderAssetModule);
    };

} // namespace ice::render::vk
#endif
