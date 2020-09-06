#include <iceshard/renderer/vulkan/vulkan_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <core/pod/array.hxx>
#include <core/pod/algorithm.hxx>
#include <core/debug/assert.hxx>

#include "../vulkan_device_memory_manager.hxx"
#include "../vulkan_buffer.hxx"

namespace iceshard::renderer::vulkan
{

    VulkanRenderSystem::VulkanRenderSystem(core::allocator& alloc, VkInstance instance) noexcept
        : _allocator{ alloc }
        , _vk_instance{ instance }
        , _device_memory_manager{ nullptr, { alloc } }
        , _textures{ nullptr, { alloc } }
        , _command_buffer_pool{ nullptr, { alloc } }
        , _vulkan_buffers{ _allocator }
        , _framebuffers{ _allocator }
        , _command_buffers_primary{ _allocator }
        , _resource_sets{ _allocator }
        , _pipelines{ _allocator }
        , _command_semaphores{ _allocator }
    {
        _surface = create_surface(_allocator, _vk_instance, { 1280, 720 });
        create_devices(_vk_instance, native_handle(_surface), _devices);

        _device_memory_manager = core::memory::make_unique<VulkanDeviceMemoryManager>(
            _allocator,
            _allocator,
            _devices
        );

        _textures = core::memory::make_unique<VulkanTextureStorage>(
            _allocator,
            _allocator,
            *_device_memory_manager
        );

        _command_buffer_pool = core::memory::make_unique<VulkanCommandBufferPool>(
            _allocator,
            _devices
        );

        core::pod::array::resize(_command_buffers_primary, 2);

        _command_buffer_pool->allocate_buffers(
            api::CommandBufferType::Primary,
            _command_buffers_primary
        );

        {
            VkSemaphoreCreateInfo semaphore_info;
            semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
            semaphore_info.pNext = nullptr;
            semaphore_info.flags = 0;

            core::pod::array::resize(_command_semaphores, 4);
            for (uint32_t i = 0; i < core::pod::array::size(_command_semaphores); ++i)
            {
                vkCreateSemaphore(
                    _devices.graphics.handle,
                    &semaphore_info,
                    nullptr,
                    std::addressof(_command_semaphores[i])
                );
            }
        }

        VkSurfaceFormatKHR format;
        get_surface_format(_devices.physical.handle, _surface, format);
        create_renderpass(_devices.graphics.handle, format.format, RenderPassFeatures::PostProcess, _renderpass);

        create_resource_pool(_devices.graphics.handle, _resource_pool);
        create_resource_layouts(_devices.graphics.handle, _resource_layouts);
        create_pipeline_layouts(_devices, _resource_layouts, _pipeline_layouts);

        VkFenceCreateInfo fence_info;
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fence_info.pNext = nullptr;
        fence_info.flags = 0;

        auto res = vkCreateFence(_devices.graphics.handle, &fence_info, nullptr, &_draw_fence);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't create fence!");

        begin_frame();
        _initialized = true;
    }

    VulkanRenderSystem::~VulkanRenderSystem() noexcept
    {
        vkDestroyFence(_devices.graphics.handle, _draw_fence, nullptr);

        destroy_framebuffers(_allocator, _framebuffers);
        destroy_swapchain(_allocator, _swapchain);

        destroy_pipeline_layouts(_devices, _pipeline_layouts);
        destroy_resource_layouts(_devices.graphics.handle, _resource_layouts);
        destroy_resource_pool(_devices.graphics.handle, _resource_pool);

        destroy_renderpass(_renderpass);

        for (auto cmd_semaphore : _command_semaphores)
        {
            vkDestroySemaphore(_devices.graphics.handle, cmd_semaphore, nullptr);
        }

        _vulkan_buffers.clear();

        _command_buffer_pool = nullptr;
        _textures = nullptr;
        _device_memory_manager = nullptr;

        destroy_devices(_vk_instance, _devices);
        destroy_surface(_allocator, _surface);
    }

    void VulkanRenderSystem::begin_frame() noexcept
    {
        VkSurfaceCapabilitiesKHR capabilities;
        get_surface_capabilities(_devices.physical.handle, _surface, capabilities);

        auto new_area = capabilities.currentExtent;
        if (_render_area.width != new_area.width || _render_area.height != new_area.height)
        {
            vkDeviceWaitIdle(_devices.graphics.handle);

            _render_area = new_area;
            if (_initialized)
            {
                destroy_framebuffers(_allocator, _framebuffers);
                destroy_swapchain(_allocator, _swapchain);
            }

            _swapchain = create_swapchain(_allocator, _devices, _surface);
            create_framebuffers(_allocator, _render_area, _devices, _renderpass, _swapchain, _framebuffers);
        }

        acquire_next_image();

        {
            auto cmds = _command_buffers_primary[0];

            VkCommandBufferBeginInfo cmd_buf_info = {};
            cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            cmd_buf_info.pNext = nullptr;
            cmd_buf_info.flags = 0;
            cmd_buf_info.pInheritanceInfo = nullptr;

            auto api_result = vkBeginCommandBuffer(cmds, &cmd_buf_info);
            IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");

            VkClearValue clear_values[3];
            clear_values[0].color.float32[0] = 0.2f;
            clear_values[0].color.float32[1] = 0.2f;
            clear_values[0].color.float32[2] = 0.2f;
            clear_values[0].color.float32[3] = 0.2f;
            clear_values[1] = clear_values[0];
            clear_values[2].depthStencil.depth = 1.0f;
            clear_values[2].depthStencil.stencil = 0;

            VkRenderPassBeginInfo rp_begin;
            rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            rp_begin.pNext = NULL;
            rp_begin.renderPass = _renderpass.renderpass;
            rp_begin.framebuffer = native_handle(_framebuffers[_current_framebuffer_index]);
            rp_begin.renderArea.offset.x = 0;
            rp_begin.renderArea.offset.y = 0;
            rp_begin.renderArea.extent = _render_area;
            rp_begin.clearValueCount = 3;
            rp_begin.pClearValues = clear_values;

            vkCmdBeginRenderPass(cmds, &rp_begin, VkSubpassContents::VK_SUBPASS_CONTENTS_INLINE);
            vkCmdNextSubpass(cmds, VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        }

    }

    void VulkanRenderSystem::end_frame() noexcept
    {
        auto cmds = _command_buffers_primary[0];

        vkCmdEndRenderPass(cmds);
        vkEndCommandBuffer(cmds);

        v1_submit_command_buffers();

        v1_present();
    }

    //auto VulkanRenderSystem::acquire_command_buffer(RenderPassStage stage) noexcept -> CommandBuffer
    //{
    //    auto cmd_buffer_index = _next_command_buffer.fetch_add(1);
    //    IS_ASSERT(cmd_buffer_index < core::pod::array::size(_command_buffers_secondary), "No more available command buffers!");

    //    VulkanCommandBuffer cmd_buff{ };
    //    cmd_buff.native = _command_buffers_secondary[cmd_buffer_index];

    //    VkCommandBufferInheritanceInfo inheritance_info;
    //    inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    //    inheritance_info.pNext = nullptr;
    //    inheritance_info.framebuffer = nullptr;
    //    inheritance_info.renderPass = _renderpass.renderpass;
    //    inheritance_info.queryFlags = 0;
    //    inheritance_info.occlusionQueryEnable = VK_FALSE;
    //    inheritance_info.pipelineStatistics = 0;
    //    inheritance_info.subpass = 1;

    //    VkCommandBufferBeginInfo cmd_buf_info = {};
    //    cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    //    cmd_buf_info.pNext = nullptr;
    //    cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
    //    cmd_buf_info.pInheritanceInfo = &inheritance_info;

    //    auto api_result = vkBeginCommandBuffer(cmd_buff.native, &cmd_buf_info);
    //    IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");

    //    // Setup scale and translation:
    //    // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
    //    if (stage == RenderPassStage::DebugUI)
    //    {
    //        auto draw_area_ext = render_area();

    //        float scale[2];
    //        scale[0] = 2.0f / draw_area_ext.width;
    //        scale[1] = 2.0f / draw_area_ext.height;
    //        float translate[2];
    //        translate[0] = -1.0f; // -1.0f - width * scale[0];
    //        translate[1] = -1.0f; //-1.0f - height * scale[1];
    //        vkCmdPushConstants(cmd_buff.native, _pipeline_layouts.debugui_layout.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
    //        vkCmdPushConstants(cmd_buff.native, _pipeline_layouts.debugui_layout.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);

    //        inheritance_info.subpass = 2;
    //    }

    //    if (stage == RenderPassStage::PostProcess)
    //    {
    //        inheritance_info.subpass = 2;
    //    }

    //    core::pod::array::push_back(_command_buffers_subpass, inheritance_info.subpass);

    //    return cmd_buff.handle;
    //}

    //void VulkanRenderSystem::submit_command_buffer(CommandBuffer cmd_buffer) noexcept
    //{
    //    VulkanCommandBuffer cmd_buff{ cmd_buffer };
    //    vkEndCommandBuffer(cmd_buff.native);

    //    auto cmd_buffer_index = _submitted_command_buffer_count.fetch_add(1);
    //    IS_ASSERT(cmd_buffer_index < core::pod::array::size(_command_buffers_secondary), "No more available command buffers!");

    //    _command_buffers_submitted[cmd_buffer_index] = cmd_buff.native;
    //}

    void VulkanRenderSystem::submit_command_buffer_v2(CommandBuffer buffer) noexcept
    {
        const VkCommandBuffer cmd_bufs[] =
        {
            VulkanCommandBuffer{ buffer }.native
        };

        VkSubmitInfo submit_info[1] = {};
        submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info[0].commandBufferCount = 1;
        submit_info[0].pCommandBuffers = cmd_bufs;
        submit_info[0].signalSemaphoreCount = 1;
        submit_info[0].pSignalSemaphores = &_command_semaphores[_next_command_semaphore];
        submit_info[0].pWaitDstStageMask = nullptr;
        _next_command_semaphore += 1;

        auto res = vkQueueSubmit(_devices.graphics.queue, 1, submit_info, vk_nullptr);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't submit queue!");
    }

    auto VulkanRenderSystem::swapchain() noexcept -> VulkanSwapchain
    {
        return _swapchain;
    }

    auto VulkanRenderSystem::render_area() noexcept -> VkExtent2D
    {
        return _render_area;
    }

    auto VulkanRenderSystem::resource_layouts() noexcept -> VulkanResourceLayouts
    {
        return _resource_layouts;
    }

    auto VulkanRenderSystem::get_resource_set(core::stringid_arg_type name) noexcept -> ResourceSet
    {
        VulkanResourceSet* resource_set = core::pod::hash::get(
            _resource_sets,
            core::hash(name),
            nullptr
        );

        return ResourceSet{ (uintptr_t)resource_set };
    }

    auto VulkanRenderSystem::create_resource_set(
        core::stringid_arg_type name,
        iceshard::renderer::RenderPipelineLayout layout,
        RenderResourceSetInfo resource_set_info,
        core::pod::Array<RenderResource> const& resources
    ) noexcept -> ResourceSet
    {
        core::memory::stack_allocator<32> resource_layouts_alloc;
        core::pod::Array<VkDescriptorSetLayout> descriptor_layouts{ resource_layouts_alloc };
        core::pod::array::reserve(descriptor_layouts, 32 / sizeof(VkDescriptorSetLayout));

        if (core::has_flag(resource_set_info.usage, RenderResourceSetUsage::MaterialData))
        {
            core::pod::array::push_back(descriptor_layouts, _resource_layouts.descriptor_set_samplers);
            core::pod::array::push_back(descriptor_layouts, _resource_layouts.descriptor_set_textures);
        }
        if (core::has_flag(resource_set_info.usage, RenderResourceSetUsage::ViewProjectionData))
        {
            core::pod::array::push_back(descriptor_layouts, _resource_layouts.descriptor_set_uniforms[0]);
        }
        if (core::has_flag(resource_set_info.usage, RenderResourceSetUsage::LightsData))
        {
            core::pod::array::push_back(descriptor_layouts, _resource_layouts.descriptor_set_uniforms[1]);
        }

        IS_ASSERT(core::pod::array::empty(descriptor_layouts) == false, "No resource set usage selected!");

        VulkanPipelineLayout pipeline_layout{ };
        if (layout == RenderPipelineLayout::DebugUI)
        {
            pipeline_layout = _pipeline_layouts.debugui_layout;
        }
        else if (layout == RenderPipelineLayout::PostProcess)
        {
            pipeline_layout = _pipeline_layouts.postprocess_layout;
        }
        else if (layout == RenderPipelineLayout::Textured)
        {
            pipeline_layout = _pipeline_layouts.textured_layout;
        }
        else // if (layout == RenderPipelineLayout::Default)
        {
            pipeline_layout = _pipeline_layouts.default_layout;
        }

        VulkanResourceSet* resource_set = _allocator.make<VulkanResourceSet>();
        resource_set->resource_set_usage = resource_set_info.usage;
        vulkan::create_resource_set(
            _devices.graphics.handle,
            _framebuffers[0],
            _resource_pool,
            pipeline_layout,
            descriptor_layouts,
            name,
            resources,
            *resource_set
        );
        core::pod::hash::set(
            _resource_sets,
            core::hash(name),
            resource_set
        );
        return ResourceSet{ (uintptr_t) resource_set };
    }

    void VulkanRenderSystem::update_resource_set(
        core::stringid_arg_type name,
        core::pod::Array<RenderResource> const& resources
    ) noexcept
    {
        VulkanResourceSet* resource_set = core::pod::hash::get(
            _resource_sets,
            core::hash(name),
            nullptr
        );

        if (resource_set != nullptr)
        {
            vulkan::update_resource_set(
                _devices.graphics.handle,
                _framebuffers[0],
                *resource_set,
                resources
            );
        }
    }

    void VulkanRenderSystem::destroy_resource_set(
        core::stringid_arg_type name
    ) noexcept
    {
        auto* const resource_set = core::pod::hash::get<VulkanResourceSet*>(_resource_sets, core::hash(name), nullptr);
        if (resource_set != nullptr)
        {
            vulkan::destroy_resource_set(
                _devices.graphics.handle,
                _resource_pool,
                *resource_set
            );
            _allocator.destroy(resource_set);
            core::pod::hash::remove(_resource_sets, core::hash(name));
        }
    }

    auto VulkanRenderSystem::create_pipeline(
        core::stringid_arg_type name,
        RenderPipelineLayout layout,
        core::pod::Array<asset::AssetData> const& shader_assets
    ) noexcept -> Pipeline
    {
        VulkanPipelineLayout selected_layout{ };
        if (layout == RenderPipelineLayout::DebugUI)
        {
            selected_layout = _pipeline_layouts.debugui_layout;
        }
        else if (layout == RenderPipelineLayout::PostProcess)
        {
            selected_layout = _pipeline_layouts.postprocess_layout;
        }
        else if (layout == RenderPipelineLayout::Textured)
        {
            selected_layout = _pipeline_layouts.textured_layout;
        }
        else // if (layout == RenderPipelineLayout::Default)
        {
            selected_layout = _pipeline_layouts.default_layout;
        }

        Pipeline result = Pipeline::Invalid;

        VulkanPipelineModules pipeline_modules;
        build_pipeline_shaders(_devices, shader_assets, pipeline_modules);
        {
            auto* vulkan_pipeline = _allocator.make<VulkanPipeline>();
            create_graphics_pipeline(_devices, _renderpass, selected_layout, pipeline_modules, *vulkan_pipeline);

            core::pod::hash::set(
                _pipelines,
                core::hash(name),
                vulkan_pipeline
            );

            result = Pipeline{ (uintptr_t)vulkan_pipeline };
        }
        release_pipeline_shaders(_devices, pipeline_modules);

        return result;
    }

    void VulkanRenderSystem::destroy_pipeline(core::stringid_arg_type name) noexcept
    {
        auto* const pipeline = core::pod::hash::get<VulkanPipeline*>(_pipelines, core::hash(name), nullptr);
        if (pipeline != nullptr)
        {
            destroy_graphics_pipeline(_devices, *pipeline);
            _allocator.destroy(pipeline);
            core::pod::hash::remove(_pipelines, core::hash(name));
        }
    }

    auto VulkanRenderSystem::create_data_buffer(
        iceshard::renderer::api::BufferType type,
        uint32_t size
    ) noexcept -> iceshard::renderer::api::Buffer
    {
        auto vulkan_buffer = iceshard::renderer::vulkan::create_buffer(
            _allocator,
            type,
            size,
            *_device_memory_manager
        );

        auto result = iceshard::renderer::api::v1_1::Buffer{ reinterpret_cast<uintptr_t>(vulkan_buffer.get()) };
        _vulkan_buffers.emplace_back(std::move(vulkan_buffer));

        return result;
    }

    auto VulkanRenderSystem::renderpass(RenderPassFeatures features) noexcept -> RenderPass
    {
        return _renderpass.handle;
    }

    auto VulkanRenderSystem::renderpass_command_buffer(
        RenderPassFeatures features
    ) noexcept -> CommandBuffer
    {
        return VulkanCommandBuffer{ .native = _command_buffers_primary[0] }.handle;
    }

    auto VulkanRenderSystem::current_framebuffer() noexcept -> iceshard::renderer::api::Framebuffer
    {
        return iceshard::renderer::api::Framebuffer{
            _framebuffers[_current_framebuffer_index]
        };
    }

    auto VulkanRenderSystem::v1_surface() noexcept -> VkSurfaceKHR
    {
        return native_handle(_surface);
    }

    auto VulkanRenderSystem::v1_graphics_device() noexcept -> VkDevice
    {
        return _devices.graphics.handle;
    }

    auto VulkanRenderSystem::v1_graphics_queue() noexcept -> VkQueue
    {
        return _devices.graphics.queue;
    }

    auto VulkanRenderSystem::v1_renderpass() noexcept -> VkRenderPass
    {
        return _renderpass.renderpass;
    }

    auto VulkanRenderSystem::v1_current_framebuffer() noexcept -> VkFramebuffer
    {
        return native_handle(_framebuffers[_current_framebuffer_index]);
    }

    void VulkanRenderSystem::v1_submit_command_buffers() noexcept
    {
        const VkCommandBuffer cmd_bufs[] =
        {
            _command_buffers_primary[0]
        };

        VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info[1] = {};
        submit_info[0].pNext = NULL;
        submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info[0].waitSemaphoreCount = _next_command_semaphore;
        submit_info[0].pWaitSemaphores = &_command_semaphores[0];
        submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
        submit_info[0].commandBufferCount = 1;
        submit_info[0].pCommandBuffers = cmd_bufs;
        submit_info[0].signalSemaphoreCount = 0;
        submit_info[0].pSignalSemaphores = NULL;

        _next_command_semaphore = 0;

        auto res = vkQueueSubmit(_devices.graphics.queue, 1, submit_info, _draw_fence);
        IS_ASSERT(res == VK_SUCCESS, "Couldn't submit queue!");

        do
        {
            constexpr auto FENCE_TIMEOUT = 100'000'000; // in ns
            res = vkWaitForFences(_devices.graphics.handle, 1, &_draw_fence, VK_TRUE, FENCE_TIMEOUT);
        } while (res == VK_TIMEOUT);

        vkResetFences(_devices.graphics.handle, 1, &_draw_fence);
    }

    void VulkanRenderSystem::acquire_next_image() noexcept
    {
        // Get the index of the next available swapchain image:
        auto api_result = vkAcquireNextImageKHR(
            _devices.graphics.handle,
            native_handle(_swapchain),
            UINT64_MAX,
            _command_semaphores[_next_command_semaphore],
            VK_NULL_HANDLE,
            &_current_framebuffer_index
        );

        _next_command_semaphore += 1;

        // TODO: Deal with the VK_SUBOPTIMAL_KHR and VK_ERROR_OUT_OF_DATE_KHR
        // return codes
        IS_ASSERT(api_result == VK_SUCCESS, "Couldn't get next framebuffer image!");
    }

    void VulkanRenderSystem::present_image() noexcept
    {
        VkSwapchainKHR swapchains[1]{ native_handle(_swapchain) };

        VkPresentInfoKHR present;
        present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        present.pNext = NULL;
        present.swapchainCount = 1;
        present.pSwapchains = swapchains;
        present.pImageIndices = &_current_framebuffer_index;
        present.pWaitSemaphores = NULL;
        present.waitSemaphoreCount = 0;
        present.pResults = NULL;

        auto api_result = vkQueuePresentKHR(_devices.presenting_queue, &present);
        IS_ASSERT(api_result == VK_SUCCESS, "Failed to present framebuffer image!");
    }

    void VulkanRenderSystem::v1_present() noexcept
    {
        present_image();
    }

    auto create_render_system(core::allocator& alloc, VkInstance instance) noexcept -> VulkanRenderSystem*
    {
        return alloc.make<VulkanRenderSystem>(alloc, instance);
    }

    void destroy_render_system(core::allocator& alloc, VulkanRenderSystem* system) noexcept
    {
        alloc.destroy(system);
    }

} // namespace iceshard::renderer::vulkan
