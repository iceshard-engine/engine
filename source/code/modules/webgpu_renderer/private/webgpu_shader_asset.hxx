/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/os.hxx>
#include <ice/module.hxx>
#include <ice/resource_compiler.hxx>
#include <ice/asset_module.hxx>

namespace ice::render::vk
{

    struct WebGPUShaderAssetModule : ice::Module<WebGPUShaderAssetModule>
    {
        static ice::ResourceCompiler API_ShaderCompiler;

        static void v1_archive_api(ice::detail::asset_system::v1::AssetTypeArchiveAPI& api) noexcept;

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            return negotiator.register_api(v1_archive_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(WebGPUShaderAssetModule);
    };

} // namespace ice::render::vk
