/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_driver.hxx"
#include "webgpu_utils.hxx"
#include "webgpu_surface.hxx"
#include <ice/assert.hxx>

namespace ice::render::webgpu
{

    void wgpu_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* userdata)
    {
        WebGPUDriver* driver = reinterpret_cast<WebGPUDriver*>(userdata);
        if (status == WGPURequestDeviceStatus_Success)
        {
            driver->set_device(device);
        }
    }

    void wgpu_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* userdata)
    {
        WebGPUDriver* driver = reinterpret_cast<WebGPUDriver*>(userdata);
        if (status == WGPURequestAdapterStatus_Success)
        {
            driver->set_adapter(adapter);

            //WGPURequiredLimits limits{};
            //limits.limits.maxBindGroups = 2;
            // limits.limits.minUniformBufferOffsetAlignment = 256;
            // limits.limits.minStorageBufferOffsetAlignment = 256;

            WGPUDeviceDescriptor descriptor{};
            descriptor.label = "IceShard-WebGPU";
            descriptor.defaultQueue.label = "Default Queue";
            descriptor.deviceLostCallback = nullptr;
            descriptor.deviceLostUserdata = nullptr;
            descriptor.requiredFeatureCount = 0;
            descriptor.requiredFeatures = nullptr;
            descriptor.requiredLimits = nullptr; //&limits;
            wgpuAdapterRequestDevice(adapter, &descriptor, wgpu_device_callback, driver);
        }
    }

    WebGPUDriver::WebGPUDriver(
        ice::Allocator& alloc,
        WGPUInstance wgpu_instance
    ) noexcept
        : _allocator{ alloc, "WebGPU-Renderer" }
        , _state{ DriverState::Pending }
        , _wgpu_instance{ wgpu_instance }
        , _wgpu_adapter{ nullptr }
        , _wgpu_device{ nullptr }
    {

    }

    WebGPUDriver::~WebGPUDriver() noexcept
    {
        wgpuAdapterRelease(_wgpu_adapter);
        wgpuInstanceRelease(_wgpu_instance);
    }

    void WebGPUDriver::set_adapter(WGPUAdapter adapter) noexcept
    {
        if (adapter == nullptr)
        {
            _state = DriverState::Failed;
        }
        else
        {
            _wgpu_adapter = adapter;
        }
    }

    void WebGPUDriver::set_device(WGPUDevice device) noexcept
    {
        if (device == nullptr)
        {
            _state = DriverState::Failed;
        }
        else
        {
            _wgpu_device = device;
            _state = DriverState::Ready;
        }
    }

    auto WebGPUDriver::create_surface(
        ice::render::SurfaceInfo const& surface_info
    ) noexcept -> ice::render::RenderSurface*
    {
        WGPUSurfaceDescriptorFromCanvasHTMLSelector canvas{
            .chain = { .sType = WGPUSType_SurfaceDescriptorFromCanvasHTMLSelector },
            .selector = surface_info.webgpu.selector
        };
        WGPUSurfaceDescriptor descriptor{};
        descriptor.label = "Default Surface";
        descriptor.nextInChain = &canvas.chain;
        WGPUSurface surface = wgpuInstanceCreateSurface(_wgpu_instance, &descriptor);
        WGPUTextureFormat surface_format = wgpuSurfaceGetPreferredFormat(surface, _wgpu_adapter);

        WGPURequestAdapterOptions adapter_options{};
        adapter_options.backendType = WGPUBackendType::WGPUBackendType_WebGPU;
        adapter_options.powerPreference = WGPUPowerPreference_HighPerformance;
        adapter_options.compatibleSurface = surface;
        wgpuInstanceRequestAdapter(_wgpu_instance, &adapter_options, wgpu_adapter_callback, this);

        ICE_ASSERT(surface_info.type == SurfaceType::HTML5_DOMCanvas, "Only 'HTML5 Canvas' surfaces are allowed!");
        return _allocator.create<WebGPURenderSurface>(surface, surface_format, surface_info);
    }

    void WebGPUDriver::destroy_surface(
        ice::render::RenderSurface* surface
    ) noexcept
    {
        _allocator.destroy(static_cast<WebGPURenderSurface*>(surface));
    }

    void WebGPUDriver::query_queue_infos(
        ice::Array<ice::render::QueueFamilyInfo>& queue_info
    ) noexcept
    {
        ice::array::push_back(
            queue_info,
            QueueFamilyInfo {
                .id = QueueID{ 1 },
                .flags = QueueFlags::Transfer | QueueFlags::Graphics | QueueFlags::Present,
                .count = 2,
            }
        );
    }

    auto WebGPUDriver::create_device(
        ice::Span<ice::render::QueueInfo const> queue_info
    ) noexcept -> ice::render::RenderDevice*
    {
        // WGPUQuerySetDescriptor descriptor{};
        // descriptor.type = WGPUQueryType::WGPUQueryType_Occlusion;
        // descriptor.count = queue_info[0].count;
        // descriptor.label = "Default Queue Set";

        // wgpuDeviceCreateQuerySet(_wgpu_device, &descriptor);

        return _allocator.create<WebGPUDevice>(_allocator, _wgpu_device);
    }

    void WebGPUDriver::destroy_device(
        ice::render::RenderDevice* device
    ) noexcept
    {
        _allocator.destroy(static_cast<WebGPUDevice*>(device));
    }

    void WebGPUDriver::destroy() noexcept
    {
        ice::Allocator& alloc = _allocator;
        alloc.destroy(this);
    }

} // namespace ice::render::webgpu
