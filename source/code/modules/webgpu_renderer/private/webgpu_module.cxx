/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/render/render_module.hxx>
#include <ice/module.hxx>
#include <ice/log_module.hxx>
#include <ice/log_tag.hxx>
#include <ice/assert_core.hxx>

#include <atomic>
#include "webgpu_driver.hxx"
#include "webgpu_utils.hxx"


namespace ice::render::webgpu
{

    struct WGPUAdapterRequest
    {
        WGPUAdapter adapter;
        std::atomic_flag finished = ATOMIC_FLAG_INIT;
    };

    auto create_webgpu_driver(ice::Allocator& alloc) noexcept -> ice::render::RenderDriver*
    {
        // WGPUDescriptor is not implemented in EMScripten
        WGPUInstance wgpu_instance = wgpuCreateInstance(nullptr);
        if (wgpu_instance == nullptr)
        {
            return nullptr;
        }

        return alloc.create<WebGPUDriver>(alloc, wgpu_instance);
    }

    void destroy_webgpu_driver(ice::render::RenderDriver* render_driver) noexcept
    {
        static_cast<ice::render::webgpu::WebGPUDriver*>(render_driver)->destroy();
    }

    struct WebGpuModule : ice::Module<WebGpuModule>
    {
        static void v1_driver_api(ice::render::detail::v1::RenderAPI& api) noexcept
        {
            api.create_driver_fn = create_webgpu_driver;
            api.destroy_driver_fn = destroy_webgpu_driver;
        }

        static bool on_load(ice::Allocator& alloc, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            ice::LogModule::init(alloc, negotiator);
            ice::log_tag_register(LogTag_WebGPU);
            return negotiator.register_api(v1_driver_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(WebGpuModule);
    };

} // namespace ice::renderer::webgpu
