/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_driver.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/mem_unique_ptr.hxx>
#include "webgpu_utils.hxx"
#include "webgpu_device.hxx"

namespace ice::render::webgpu
{

    class WebGPUDriver : public ice::render::RenderDriver
    {
    public:
        WebGPUDriver(
            ice::Allocator& alloc,
            WGPUInstance wgpu_instance
        ) noexcept;

        ~WebGPUDriver() noexcept override;

        void destroy() noexcept;

        auto state() const noexcept -> ice::render::DriverState override { return _state; }
        auto render_api() const noexcept -> ice::render::RenderDriverAPI override { return RenderDriverAPI::WebGPU; }

        void set_adapter(WGPUAdapter adapter) noexcept;
        void set_device(WGPUDevice device) noexcept;

        auto create_surface(ice::render::SurfaceInfo const& surface_info) noexcept -> ice::render::RenderSurface* override;
        void destroy_surface(ice::render::RenderSurface* surface) noexcept override;

        void query_queue_infos(ice::Array<ice::render::QueueFamilyInfo>& queue_info) noexcept override;

        auto create_device(ice::Span<ice::render::QueueInfo const> queue_info) noexcept -> ice::render::RenderDevice* override;
        void destroy_device(ice::render::RenderDevice* device) noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::render::DriverState _state;

        WGPUInstance _wgpu_instance;
        WGPUAdapter _wgpu_adapter;
        WGPUDevice _wgpu_device;
    };

} // namespace ice::render::webgpu
