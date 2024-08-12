/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_device.hxx"
#include "webgpu_buffer.hxx"
#include "webgpu_queue.hxx"
#include "webgpu_swapchain.hxx"
#include "webgpu_fence.hxx"
#include "webgpu_surface.hxx"
#include "webgpu_framebuffer.hxx"
#include "webgpu_image.hxx"
#include "webgpu_pipeline.hxx"
#include "webgpu_resources.hxx"
#include "webgpu_sampler.hxx"
#include "webgpu_shader.hxx"
#include "webgpu_renderpass.hxx"

#include <ice/render/render_fence.hxx>
#include <ice/assert.hxx>

namespace ice::render::webgpu
{

    WebGPUDevice::WebGPUDevice(
        ice::Allocator& alloc,
        WGPUDevice device
    ) noexcept
        : _allocator{ alloc }
        , _wgpu_device{ device }
        , _wgpu_queue{ wgpuDeviceGetQueue(_wgpu_device) }
        , _commands{ }
    {
    }

    WebGPUDevice::~WebGPUDevice() noexcept
    {
        wgpuQueueRelease(_wgpu_queue);
    }

    auto WebGPUDevice::create_swapchain(
        ice::render::RenderSurface* surface
    ) noexcept -> ice::render::RenderSwapchain*
    {
        WebGPURenderSurface* webgpu_surface = static_cast<WebGPURenderSurface*>(surface);
        ice::platform::RenderSurface* render_surface = reinterpret_cast<ice::platform::RenderSurface*>(webgpu_surface->_surface_info.webgpu.internal);
        ice::vec2u surface_dimensions = render_surface->get_dimensions();

        WGPUSwapChainDescriptor descriptor{};
        descriptor.label = "Default Swapchain";
        descriptor.presentMode = WGPUPresentMode_Fifo;
        descriptor.usage = WGPUTextureUsage_RenderAttachment;
        descriptor.width = surface_dimensions.x;
        descriptor.height = surface_dimensions.y;
        descriptor.format = webgpu_surface->_wgpu_surface_format;
        WGPUSwapChain swapchain = wgpuDeviceCreateSwapChain(_wgpu_device, webgpu_surface->_wgpu_surface, &descriptor);

        ICE_ASSERT(surface != nullptr, "Invalid render surface object!");
        return _allocator.create<WebGPUSwapchain>(swapchain, webgpu_surface->_wgpu_surface_format, surface_dimensions);
    }

    void WebGPUDevice::destroy_swapchain(
        ice::render::RenderSwapchain* swapchain
    ) noexcept
    {
        _allocator.destroy(static_cast<WebGPUSwapchain*>(swapchain));
    }

    auto WebGPUDevice::create_renderpass(
        ice::render::RenderpassInfo const& info
    ) noexcept -> ice::render::Renderpass
    {
        return WebGPURenderPass::handle(_allocator.create<WebGPURenderPass>(_allocator, info));
    }

    void WebGPUDevice::destroy_renderpass(
        ice::render::Renderpass render_pass
    ) noexcept
    {
        _allocator.destroy(WebGPURenderPass::native(render_pass));
    }

    auto WebGPUDevice::create_resourceset_layout(
        ice::Span<ice::render::ResourceSetLayoutBinding const> bindings
    ) noexcept -> ice::render::ResourceSetLayout
    {
        WGPUBindGroupLayoutEntry entries[16]{};

        WGPUBindGroupLayoutDescriptor descriptor{};
        descriptor.label = "Resource Set Layout";
        descriptor.entryCount = ice::count(bindings);
        descriptor.entries = entries;
        ICE_ASSERT_CORE(descriptor.entryCount <= 16);

        ice::u32 idx = 0;
        for (ResourceSetLayoutBinding const& binding : bindings)
        {
            ICE_ASSERT_CORE(binding.resource_count == 1);
            ICE_ASSERT(binding.binding_details != nullptr, "WebGPU renderer requires binding details for layout bindings to be filled.");
            ResourceSetLayoutBindingDetails const& details = *binding.binding_details;

            WGPUBindGroupLayoutEntry& entry = entries[idx];
            entry.binding = binding.binding_index;
            entry.visibility = native_shader_stages(binding.shader_stage_flags);

            if (binding.resource_type == ResourceType::SampledImage)
            {
                ICE_ASSERT_CORE(details.image.type == ImageType::Image2D);
                entry.texture.multisampled = false;
                entry.texture.sampleType = WGPUTextureSampleType_Float;
                entry.texture.viewDimension = WGPUTextureViewDimension_2D;
            }
            else if (binding.resource_type == ResourceType::Sampler)
            {
                entry.sampler.type = WGPUSamplerBindingType::WGPUSamplerBindingType_Force32;
            }
            else if (binding.resource_type == ResourceType::UniformBuffer)
            {
                entry.buffer.minBindingSize = details.buffer.min_size;
                entry.buffer.hasDynamicOffset = false;
                entry.buffer.type = WGPUBufferBindingType_Uniform;
            }

            idx += 1;
        }

        WGPUBindGroupLayout group_layout = wgpuDeviceCreateBindGroupLayout(_wgpu_device, &descriptor);
        return WebGPUResourceSet::handle(group_layout);
    }

    void WebGPUDevice::destroy_resourceset_layout(
        ice::render::ResourceSetLayout resourceset_layout
    ) noexcept
    {
        wgpuBindGroupLayoutRelease(WebGPUResourceSet::native(resourceset_layout));
    }

    bool WebGPUDevice::create_resourcesets(
        ice::Span<ice::render::ResourceSetLayout const> resource_set_layouts,
        ice::Span<ice::render::ResourceSet> resource_sets_out
    ) noexcept
    {
        ice::u32 const count = ice::count(resource_set_layouts);
        ICE_ASSERT_CORE(count == ice::count(resource_sets_out));

        for (ice::u32 idx = 0; idx < count; ++idx)
        {
            WebGPUResourceSet* resource_set = _allocator.create<WebGPUResourceSet>();
            resource_set->_wgpu_group_layout = WebGPUResourceSet::native(resource_set_layouts[idx]);
            resource_set->_wgpu_group = nullptr;
            resource_sets_out[idx] = WebGPUResourceSet::handle(resource_set);
        }
        return true;
    }

    void WebGPUDevice::update_resourceset(
        ice::Span<ice::render::ResourceSetUpdateInfo const> update_infos
    ) noexcept
    {
        WGPUBindGroupEntry entries[16]{};


        ice::u32 idx = 0;
        ice::render::ResourceSet current_set = update_infos[0].resource_set;
        for (ice::render::ResourceSetUpdateInfo const& set_info : update_infos)
        {
            if (current_set != set_info.resource_set)
            {
                WebGPUResourceSet* native = WebGPUResourceSet::native(current_set);
                WGPUBindGroupDescriptor descriptor{};
                descriptor.label = "Resource Set";
                descriptor.layout = native->_wgpu_group_layout;
                descriptor.entries = entries;
                descriptor.entryCount = idx;

                if (native->_wgpu_group != nullptr)
                {
                    wgpuBindGroupRelease(native->_wgpu_group);
                }
                native->_wgpu_group = wgpuDeviceCreateBindGroup(_wgpu_device, &descriptor);
                idx = 0;
            }

            current_set = set_info.resource_set;

            ICE_ASSERT_CORE(set_info.resources._count == 1);
            for (ResourceUpdateInfo const& info : set_info.resources)
            {
                WGPUBindGroupEntry& entry = entries[idx];
                entry = WGPUBindGroupEntry{};
                entry.binding = set_info.binding_index;

                switch(set_info.resource_type)
                {
                case ResourceType::Sampler:
                    entry.sampler = reinterpret_cast<WGPUSampler>(static_cast<uintptr_t>(info.sampler));
                    break;
                case ResourceType::UniformBuffer:
                {
                    WebGPUBuffer* native = WebGPUBuffer::native(info.uniform_buffer.buffer);
                    entry.buffer = native->wgpu_buffer;
                    entry.offset = info.uniform_buffer.offset;
                    entry.size = info.uniform_buffer.size;
                    // native->size = entry.size;
                    break;
                }
                case ResourceType::SampledImage:
                    entry.textureView = WebGPUImage::native(info.image)->wgpu_texture_view;
                    break;
                default: ICE_ASSERT_CORE(false);
                }

                idx += 1;
            }
        }

        if (idx > 0)
        {
            WebGPUResourceSet* native = WebGPUResourceSet::native(current_set);
            WGPUBindGroupDescriptor descriptor{};
            descriptor.label = "Resource Set";
            descriptor.layout = native->_wgpu_group_layout;
            descriptor.entries = entries;
            descriptor.entryCount = idx;

            if (native->_wgpu_group != nullptr)
            {
                wgpuBindGroupRelease(native->_wgpu_group);
            }
            native->_wgpu_group = wgpuDeviceCreateBindGroup(_wgpu_device, &descriptor);
        }
    }

    void WebGPUDevice::destroy_resourcesets(
        ice::Span<ice::render::ResourceSet const> resource_sets
    ) noexcept
    {
        for (ice::render::ResourceSet resource_set : resource_sets)
        {
            WebGPUResourceSet* native = WebGPUResourceSet::native(resource_set);
            if (native->_wgpu_group != nullptr)
            {
                wgpuBindGroupRelease(native->_wgpu_group);
            }
            _allocator.destroy(native);
        }
    }

    auto WebGPUDevice::create_pipeline_layout(
        ice::render::PipelineLayoutInfo const& info
    ) noexcept -> ice::render::PipelineLayout
    {
        if (ice::span::any(info.push_constants))
        {
            ICE_LOG_WGPU(LogSeverity::Warning, "WebGPU Pipelines do not support push constants.");
        }

        ice::ucount count = 0;
        WGPUBindGroupLayout layouts[8]{};
        for (ResourceSetLayout resource_layout : info.resource_layouts)
        {
            layouts[count] = WebGPUResourceSet::native(resource_layout);
            count += 1;
        }

        WGPUPipelineLayoutDescriptor descriptor{};
        descriptor.label = "Pipeline Layout";
        descriptor.bindGroupLayoutCount = count;
        descriptor.bindGroupLayouts = layouts;

        WGPUPipelineLayout pipeline_layout = wgpuDeviceCreatePipelineLayout(_wgpu_device, &descriptor);
        return WebGPUPipeline::handle(pipeline_layout);
    }

    void WebGPUDevice::destroy_pipeline_layout(
        ice::render::PipelineLayout pipeline_layout
    ) noexcept
    {
        wgpuPipelineLayoutRelease(WebGPUPipeline::native(pipeline_layout));
    }

    auto WebGPUDevice::create_shader(
        ice::render::ShaderInfo const& shader_info
    ) noexcept -> ice::render::Shader
    {
        char const* code = reinterpret_cast<char const*>(shader_info.shader_data.location);
        ICE_ASSERT_CORE(code[shader_info.shader_data.size.value - 1] == '\0');

        WGPUShaderModuleWGSLDescriptor wgsl_descriptor{};
        wgsl_descriptor.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
        wgsl_descriptor.chain.next = nullptr;
        wgsl_descriptor.code = code;

        WGPUShaderModuleDescriptor descriptor{};
        descriptor.label = "Shader";
        descriptor.nextInChain = &wgsl_descriptor.chain;

        WGPUShaderModule shader = wgpuDeviceCreateShaderModule(_wgpu_device, &descriptor);
        ICE_ASSERT_CORE(shader != nullptr);
        return WebGPUShader::handle(_allocator.create<WebGPUShader>(shader));
    }

    void WebGPUDevice::destroy_shader(
        ice::render::Shader shader
    ) noexcept
    {
        _allocator.destroy(WebGPUShader::native(shader));
    }

    auto WebGPUDevice::create_pipeline(
        ice::render::PipelineInfo const& info
    ) noexcept -> ice::render::Pipeline
    {
        WebGPURenderPass* webgpu_rp = WebGPURenderPass::native(info.renderpass);

        WGPUVertexBufferLayout bindings[6]{};
        WGPUVertexAttribute attributes[16]{};

        ice::u32 binding_count = 0;
        ice::u32 attribute_offset = 0;
        for (ShaderInputBinding const& binding : info.vertex_bindings)
        {
            ice::u32 attribute_count = 0;
            for (ShaderInputAttribute const& attrib : binding.attributes)
            {
                WGPUVertexAttribute& native = attributes[attribute_offset + attribute_count];
                native.shaderLocation = attrib.location;
                native.offset = attrib.offset;
                native.format = native_attribute_type(attrib.type);
                attribute_count += 1;
            }

            WGPUVertexBufferLayout& native = bindings[binding_count];
            native.arrayStride = binding.stride;
            native.attributeCount = attribute_count;
            native.attributes = attributes + attribute_offset;
            native.stepMode = binding.instanced ? WGPUVertexStepMode_Instance :  WGPUVertexStepMode_Vertex;

            attribute_offset += attribute_count;
            binding_count += 1;
        }

        WGPUVertexState vertex{};
        vertex.entryPoint = "main";
        vertex.bufferCount = binding_count;
        vertex.buffers = bindings;

        WGPUBlendState blend{};
        blend.color.srcFactor = WGPUBlendFactor_SrcAlpha;
        blend.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
        blend.color.operation = WGPUBlendOperation_Add;
        blend.alpha.srcFactor = WGPUBlendFactor_Zero;
        blend.alpha.dstFactor = WGPUBlendFactor_One;
        blend.alpha.operation = WGPUBlendOperation_Add;

        WGPUColorTargetState targets[4]{};
        WGPUFragmentState fragment{};
        fragment.entryPoint = "main";
        fragment.targetCount = 0;
        fragment.targets = targets;

        RenderSubPass const& subpass = webgpu_rp->subpasses[info.subpass_index];
        for (AttachmentReference const& ref : subpass.color_attachments)
        {
            WGPUColorTargetState& target = targets[fragment.targetCount];
            target.writeMask = WGPUColorWriteMask_All;
            target.format = native_format(webgpu_rp->attachments[ref.attachment_index].format);
            target.blend = &blend;

            fragment.targetCount += 1;
        }

        for (PipelineProgramInfo const& program : info.shaders)
        {
            switch (program.stage)
            {
            case ShaderStageFlags::VertexStage:
                vertex.module = WebGPUShader::native(program.shader)->_wgpu_shader;
                vertex.entryPoint = ice::string::begin(program.entry_point);
                break;
            case ShaderStageFlags::FragmentStage:
                fragment.module = WebGPUShader::native(program.shader)->_wgpu_shader;
                fragment.entryPoint = ice::string::begin(program.entry_point);
                break;
            default:
                ICE_ASSERT_CORE(false);
                break;
            }
        }

        // vertex.module = WebGPUShader::native(info.shaders[0])->_wgpu_shader;


        // depthstencil.depthTestEnable = info.depth_test ? VK_TRUE : VK_FALSE;
        // depthstencil.depthWriteEnable = VK_TRUE;
        // depthstencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        // depthstencil.depthBoundsTestEnable = VK_FALSE;
        // depthstencil.minDepthBounds = 0;
        // depthstencil.maxDepthBounds = 0;
        // depthstencil.stencilTestEnable = VK_FALSE;
        // depthstencil.back.failOp = VK_STENCIL_OP_KEEP;
        // depthstencil.back.passOp = VK_STENCIL_OP_KEEP;
        // depthstencil.back.compareOp = VK_COMPARE_OP_ALWAYS;
        // depthstencil.back.compareMask = 0;
        // depthstencil.back.reference = 0;
        // depthstencil.back.depthFailOp = VK_STENCIL_OP_KEEP;
        // depthstencil.back.writeMask = 0;
        // depthstencil.front = depthstencil.back;

        WGPURenderPipelineDescriptor descriptor{};
        descriptor.label = "Render Pipeline";
        descriptor.layout = WebGPUPipeline::native(info.layout);
        descriptor.multisample.alphaToCoverageEnabled = false;
        descriptor.multisample.count = 1;
        descriptor.multisample.mask = ~0u;
        // descriptor.depthStencil = &depth_stencil;
        descriptor.primitive.topology = primitive_topology(info.primitive_topology);
        descriptor.primitive.cullMode = info.cull_mode == CullMode::BackFace ? WGPUCullMode_Back : (info.cull_mode == CullMode::Disabled ? WGPUCullMode_None : WGPUCullMode_Front);
        descriptor.primitive.frontFace = info.front_face == FrontFace::ClockWise ? WGPUFrontFace_CW : WGPUFrontFace_CCW;
        descriptor.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
        descriptor.vertex = vertex;
        descriptor.fragment = &fragment;

        return WebGPUPipeline::handle(wgpuDeviceCreateRenderPipeline(_wgpu_device, &descriptor));
    }

    void WebGPUDevice::destroy_pipeline(
        ice::render::Pipeline pipeline
    ) noexcept
    {
        wgpuRenderPipelineRelease(WebGPUPipeline::native(pipeline));
    }

    auto WebGPUDevice::create_buffer(
        ice::render::BufferType buffer_type,
        ice::u32 buffer_size
    ) noexcept -> ice::render::Buffer
    {
        WGPUBufferUsageFlags flags = WGPUBufferUsage_CopyDst;
        switch(buffer_type)
        {
        case BufferType::Index:
            flags |= WGPUBufferUsage_Index;
            break;
        case BufferType::Vertex:
            flags |= WGPUBufferUsage_Vertex;
            break;
        case BufferType::Uniform:
            flags |= WGPUBufferUsage_Uniform;
            break;
        case BufferType::Transfer:
            flags |= WGPUBufferUsage_CopySrc;
            break;
        }

        WGPUBufferDescriptor descriptor{};
        descriptor.label = "Buffer";
        descriptor.mappedAtCreation = false;
        descriptor.size = buffer_size;
        descriptor.usage = flags;

        WGPUBuffer wgpu_buffer = wgpuDeviceCreateBuffer(_wgpu_device, &descriptor);
        ICE_ASSERT_CORE(wgpu_buffer != nullptr);

        WebGPUBuffer* native = _allocator.create<WebGPUBuffer>();
        native->wgpu_buffer = wgpu_buffer;
        native->size = buffer_size;
        return WebGPUBuffer::handle(native);
    }

    void WebGPUDevice::destroy_buffer(
        ice::render::Buffer buffer
    ) noexcept
    {
        WebGPUBuffer* native = WebGPUBuffer::native(buffer);
        wgpuBufferRelease(native->wgpu_buffer);
        _allocator.destroy(native);
    }

    void WebGPUDevice::update_buffers(
        ice::Span<ice::render::BufferUpdateInfo const> update_infos
    ) noexcept
    {
        for (BufferUpdateInfo const& info : update_infos)
        {
            wgpuQueueWriteBuffer(_wgpu_queue, WebGPUBuffer::native(info.buffer)->wgpu_buffer, info.offset, info.data.location, info.data.size.value);
        }
    }

    auto WebGPUDevice::create_framebuffer(
        ice::vec2u extent,
        ice::render::Renderpass renderpass,
        ice::Span<ice::render::Image const> images
    ) noexcept -> ice::render::Framebuffer
    {
        ice::Array<WebGPUImage const*> native_images{ _allocator };
        for (ice::render::Image image : images)
        {
            ice::array::push_back(native_images, WebGPUImage::native(image));
        }

        return WebGPUFrameBuffer::handle(_allocator.create<WebGPUFrameBuffer>(ice::move(native_images)));
    }

    void WebGPUDevice::destroy_framebuffer(
        ice::render::Framebuffer framebuffer
    ) noexcept
    {
        _allocator.destroy(WebGPUFrameBuffer::native(framebuffer));
    }

    auto WebGPUDevice::create_image(
        ice::render::ImageInfo const& image_info,
        ice::Data data
    ) noexcept -> ice::render::Image
    {
        ICE_ASSERT_CORE(image_info.type == ImageType::Image2D);

        WGPUTextureDescriptor descriptor{};
        descriptor.label = "Texture";
        descriptor.dimension = WGPUTextureDimension_2D;
        descriptor.usage = native_usage(image_info.usage);
        descriptor.format = native_format(image_info.format);
        descriptor.size.depthOrArrayLayers = 1;
        descriptor.size.width = image_info.width;
        descriptor.size.height = image_info.height;
        descriptor.mipLevelCount = 1;
        descriptor.sampleCount = 1;

        WGPUTexture texture = wgpuDeviceCreateTexture(_wgpu_device, &descriptor);

        if (data.location != nullptr && data.size > 0_B)
        {
            WGPUImageCopyTexture copy_info{};
            copy_info.mipLevel = 0;
            copy_info.texture = texture;
            copy_info.origin = { 0, 0, 0 };
            copy_info.aspect = WGPUTextureAspect_All;

            WGPUTextureDataLayout layout{};
            layout.offset = 0;
            layout.bytesPerRow = image_info.width * 4;
            layout.rowsPerImage = image_info.height;

            wgpuQueueWriteTexture(_wgpu_queue, &copy_info, data.location, data.size.value, &layout, &descriptor.size);
        }

        WGPUTextureViewDescriptor view_descriptor{};
        view_descriptor.label = "";
        view_descriptor.dimension = WGPUTextureViewDimension_2D;
        view_descriptor.format = descriptor.format;
        view_descriptor.mipLevelCount = descriptor.mipLevelCount;
        view_descriptor.baseMipLevel = 0;
        view_descriptor.baseArrayLayer = 0;
        view_descriptor.arrayLayerCount = 1;
        view_descriptor.aspect = WGPUTextureAspect_Undefined;
        if (ice::has_any(image_info.usage, ImageUsageFlags::DepthStencilAttachment))
        {
            view_descriptor.aspect = WGPUTextureAspect_All;
        }

        WGPUTextureView texture_view = wgpuTextureCreateView(texture, &view_descriptor);
        return WebGPUImage::handle(_allocator.create<WebGPUImage>(WebGPUImage{ texture, texture_view }));
    }

    void WebGPUDevice::destroy_image(
        ice::render::Image image
    ) noexcept
    {
        _allocator.destroy(reinterpret_cast<WebGPUImage*>(static_cast<uintptr_t>(image)));
    }

    auto WebGPUDevice::create_sampler(
        ice::render::SamplerInfo const& sampler_info
    ) noexcept -> ice::render::Sampler
    {
        WGPUSamplerDescriptor descriptor{};
        descriptor.addressModeU = native_address_mode(sampler_info.address_mode.u);
        descriptor.addressModeV = native_address_mode(sampler_info.address_mode.v);
        descriptor.addressModeW = native_address_mode(sampler_info.address_mode.w);
        descriptor.label = "Sampler";
        descriptor.magFilter = native_filter(sampler_info.mag_filter);
        descriptor.minFilter = native_filter(sampler_info.min_filter);
        descriptor.mipmapFilter = native_mipmap_mode(sampler_info.mip_map_mode);
        descriptor.lodMinClamp = 0.0;
        descriptor.lodMaxClamp = 1.0;
        descriptor.compare = WGPUCompareFunction_Undefined;
        descriptor.maxAnisotropy = 1;

        WGPUSampler sampler = wgpuDeviceCreateSampler(_wgpu_device, &descriptor);
        return static_cast<Sampler>(reinterpret_cast<uintptr_t>(sampler));
    }

    void WebGPUDevice::destroy_sampler(
        ice::render::Sampler sampler
    ) noexcept
    {
        wgpuSamplerRelease(reinterpret_cast<WGPUSampler>(static_cast<uintptr_t>(sampler)));
    }

    auto WebGPUDevice::create_queue(
        ice::render::QueueID queue_id,
        ice::render::QueueFlags queue_flags,
        ice::u32 queue_index,
        ice::u32 command_pools
    ) const noexcept -> ice::render::RenderQueue*
    {
        WGPUQueue queue = wgpuDeviceGetQueue(_wgpu_device);
        wgpuQueueReference(queue);
        return _allocator.create<WebGPUQueue>(_allocator, _wgpu_device, queue);
    }

    void WebGPUDevice::destroy_queue(
        ice::render::RenderQueue* queue
    ) const noexcept
    {
        _allocator.destroy(static_cast<WebGPUQueue*>(queue));
    }

    auto WebGPUDevice::create_fence() noexcept -> ice::render::RenderFence*
    {
        return _allocator.create<WebGPUCallbackFence>();
    }

    void WebGPUDevice::destroy_fence(
        ice::render::RenderFence* fence
    ) noexcept
    {
        _allocator.destroy(static_cast<WebGPUCallbackFence*>(fence));
    }

} // namespace ice::render::webgpu
