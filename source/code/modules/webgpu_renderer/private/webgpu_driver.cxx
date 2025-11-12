/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_driver.hxx"
#include "webgpu_utils.hxx"
#include "webgpu_surface.hxx"
#include <ice/assert.hxx>

namespace ice::render::webgpu
{

    void wgpu_device_callback(WGPURequestDeviceStatus status, WGPUDevice device, WGPUStringView message, void* userdata, void*)
    {
        ICE_LOG_IF(
            status != WGPURequestDeviceStatus::WGPURequestDeviceStatus_Success,
            LogSeverity::Error, LogTag_WebGPU,
            "Failed creation of WebGPU device with error: {}",
            ice::String{ message.data, message.length }
        );

        WebGPUDriver* driver = reinterpret_cast<WebGPUDriver*>(userdata);
        if (status == WGPURequestDeviceStatus_Success)
        {
            driver->set_device(device);
        }
    }

    void wgpu_adapter_callback(WGPURequestAdapterStatus status, WGPUAdapter adapter, WGPUStringView message, void* userdata, void*)
    {
        ICE_LOG_IF(
            status != WGPURequestAdapterStatus_Success,
            LogSeverity::Error, LogTag_WebGPU,
            "Failed to create WebGPU Adapter with error: {}",
            ice::String{ message.data, message.length }
        );

        WebGPUDriver* driver = reinterpret_cast<WebGPUDriver*>(userdata);
        if (status == WGPURequestAdapterStatus_Success)
        {
            driver->set_adapter(adapter);

            //WGPURequiredLimits limits{};
            //limits.limits.maxBindGroups = 2;
            // limits.limits.minUniformBufferOffsetAlignment = 256;
            // limits.limits.minStorageBufferOffsetAlignment = 256;

            WGPUDeviceDescriptor descriptor = WGPU_DEVICE_DESCRIPTOR_INIT;
            descriptor.label = wgpu_string("IceShard-WebGPU");
            descriptor.defaultQueue.label = wgpu_string("Default Queue");

            WGPURequestDeviceCallbackInfo callback_info = WGPU_REQUEST_DEVICE_CALLBACK_INFO_INIT;
            callback_info.callback = wgpu_device_callback;
            callback_info.userdata1 = driver;
            callback_info.mode = WGPUCallbackMode_AllowSpontaneous;
            wgpuAdapterRequestDevice(adapter, &descriptor, callback_info);
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
        WGPURequestAdapterOptions adapter_options = WGPU_REQUEST_ADAPTER_OPTIONS_INIT;
        adapter_options.backendType = WGPUBackendType::WGPUBackendType_WebGPU;
        adapter_options.powerPreference = WGPUPowerPreference_HighPerformance;

        WGPURequestAdapterCallbackInfo adapter_callback = WGPU_REQUEST_ADAPTER_CALLBACK_INFO_INIT;
        adapter_callback.callback = wgpu_adapter_callback;
        adapter_callback.userdata1 = this;
        adapter_callback.mode = WGPUCallbackMode_AllowSpontaneous;
        wgpuInstanceRequestAdapter(_wgpu_instance, &adapter_options, adapter_callback);
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
        WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas = WGPU_EMSCRIPTEN_SURFACE_SOURCE_CANVAS_HTML_SELECTOR_INIT;
        canvas.selector = wgpu_string(surface_info.webgpu.selector);

        WGPUSurfaceDescriptor descriptor = WGPU_SURFACE_DESCRIPTOR_INIT;
        descriptor.label = wgpu_string("HTML Canvas Surface");
        descriptor.nextInChain = &canvas.chain;
        WGPUSurface const surface = wgpuInstanceCreateSurface(_wgpu_instance, &descriptor);

        WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
        wgpuSurfaceGetCapabilities(surface, _wgpu_adapter, &capabilities);
        ICE_ASSERT(capabilities.formatCount > 0, "Failed to fetch surface capabilities!");

        bool supportsMailbox = false;
        for (size_t i = 0; i < capabilities.presentModeCount; i++)
        {
            if (capabilities.presentModes[i] == WGPUPresentMode_Mailbox)
            {
                supportsMailbox = true;
            }
        }

        WGPUPresentMode const present_mode = supportsMailbox ? WGPUPresentMode_Mailbox : WGPUPresentMode_Fifo;
        WGPUTextureFormat const surface_format = capabilities.formats[0];
        wgpuSurfaceCapabilitiesFreeMembers(capabilities);

        ICE_ASSERT(surface_info.type == SurfaceType::HTML5_DOMCanvas, "Only 'HTML5 Canvas' surfaces are allowed!");
        return _allocator.create<WebGPURenderSurface>(surface, surface_format, present_mode, surface_info);
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
