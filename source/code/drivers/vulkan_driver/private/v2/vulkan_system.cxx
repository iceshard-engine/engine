#include <iceshard/renderer/vulkan/vulkan_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <core/pod/array.hxx>
#include <core/pod/algorithm.hxx>

#include "../vulkan_device_memory_manager.hxx"
#include "../vulkan_buffer.hxx"

namespace iceshard::renderer::vulkan
{

    VulkanRenderSystem::VulkanRenderSystem(core::allocator& alloc, VkInstance instance) noexcept
        : _allocator{ alloc }
        , _vk_instance{ instance }
        , _device_memory_manager{ nullptr, { alloc } }
        , _vulkan_buffers{ _allocator }
        , _framebuffers{ _allocator }
        , _command_buffers_secondary{ _allocator }
        , _command_buffers_submitted{ _allocator }
        , _command_buffers_subpass{ _allocator }
        , _resource_sets{ _allocator }
        , _pipelines{ _allocator }
    {
        _surface = create_surface(_allocator, _vk_instance, { 1280, 720 });
        create_devices(_vk_instance, native_handle(_surface), _devices);

        _device_memory_manager = core::memory::make_unique<VulkanDeviceMemoryManager>(
            _allocator,
            _allocator,
            _devices
        );

        allocate_command_buffers(_devices, _command_buffers, 6, _command_buffers_secondary);
        core::pod::array::resize(_command_buffers_submitted, core::pod::array::size(_command_buffers_secondary));

        VkSemaphoreCreateInfo semaphore_info;
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;
        semaphore_info.flags = 0;
        vkCreateSemaphore(_devices.graphics.handle, &semaphore_info, nullptr, &_framebuffer_semaphore);

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
        assert(res == VK_SUCCESS);

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

        vkDestroySemaphore(_devices.graphics.handle, _framebuffer_semaphore, nullptr);

        release_command_buffers(_devices, _command_buffers, _command_buffers_secondary);

        _vulkan_buffers.clear();
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
            auto cmds = _command_buffers.primary_buffers[0];

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

            vkCmdBeginRenderPass(cmds, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdNextSubpass(cmds, VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

            _subpass_stage = 1;
        }

        _submitted_command_buffer_count = 0;
        core::pod::array::clear(_command_buffers_submitted);
        core::pod::array::clear(_command_buffers_subpass);
    }

    void VulkanRenderSystem::end_frame() noexcept
    {
        auto cmds = _command_buffers.primary_buffers[0];

        struct SubmittInfo
        {
            VkCommandBuffer* begin;
            uint32_t count;
            uint32_t subpass;
        };

        core::pod::Array<SubmittInfo> submit_infos{ _allocator };
        core::pod::array::reserve(submit_infos, core::pod::array::size(_command_buffers_subpass));

        uint32_t idx = 0;
        uint32_t count = 0;
        uint32_t last_subpass = 1;
        VkCommandBuffer* const beg = core::pod::begin(_command_buffers_submitted);
        for (auto subpass : _command_buffers_subpass)
        {
            if (subpass != last_subpass)
            {
                if (count > 0)
                {
                    core::pod::array::push_back(submit_infos, SubmittInfo{ beg + (idx - count), count, last_subpass });
                    count = 0;
                }
                last_subpass = subpass;
            }
            idx += 1;
            count += 1;
        }
        core::pod::array::push_back(submit_infos, SubmittInfo{ beg + (idx - count), count, last_subpass });

        core::pod::sort(submit_infos, [](auto const& left, auto const& right) noexcept
            {
                return left.subpass < right.subpass;
            });

        last_subpass = 1;
        for (auto const& submit_info : submit_infos)
        {
            if (last_subpass != submit_info.subpass)
            {
                vkCmdNextSubpass(cmds, VkSubpassContents::VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
                last_subpass = submit_info.subpass;
            }
            vkCmdExecuteCommands(
                _command_buffers.primary_buffers[0],
                submit_info.count,
                submit_info.begin
            );
        }

        vkCmdEndRenderPass(cmds);
        vkEndCommandBuffer(cmds);
    }

    auto VulkanRenderSystem::acquire_command_buffer(RenderPassStage stage) noexcept -> CommandBuffer
    {
        auto cmd_buffer_index = _next_command_buffer.fetch_add(1);
        IS_ASSERT(cmd_buffer_index < core::pod::array::size(_command_buffers_secondary), "No more available command buffers!");

        ApiCommandBuffer cmd_buff{ };
        cmd_buff.native = _command_buffers_secondary[cmd_buffer_index];

        VkCommandBufferInheritanceInfo inheritance_info;
        inheritance_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
        inheritance_info.pNext = nullptr;
        inheritance_info.framebuffer = nullptr;
        inheritance_info.renderPass = _renderpass.renderpass;
        inheritance_info.queryFlags = 0;
        inheritance_info.occlusionQueryEnable = VK_FALSE;
        inheritance_info.pipelineStatistics = 0;
        inheritance_info.subpass = 1;

        VkCommandBufferBeginInfo cmd_buf_info = {};
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
        cmd_buf_info.pInheritanceInfo = &inheritance_info;

        auto api_result = vkBeginCommandBuffer(cmd_buff.native, &cmd_buf_info);
        IS_ASSERT(api_result == VkResult::VK_SUCCESS, "Couldn't begin command buffer.");

        // Setup scale and translation:
        // Our visible imgui space lies from draw_data->DisplayPps (top left) to draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for single viewport apps.
        if (stage == RenderPassStage::DebugUI)
        {
            auto draw_area_ext = render_area();

            float scale[2];
            scale[0] = 2.0f / draw_area_ext.width;
            scale[1] = 2.0f / draw_area_ext.height;
            float translate[2];
            translate[0] = -1.0f; // -1.0f - width * scale[0];
            translate[1] = -1.0f; //-1.0f - height * scale[1];
            vkCmdPushConstants(cmd_buff.native, _pipeline_layouts.debugui_layout.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
            vkCmdPushConstants(cmd_buff.native, _pipeline_layouts.debugui_layout.layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);

            inheritance_info.subpass = 2;
        }

        if (stage == RenderPassStage::PostProcess)
        {
            inheritance_info.subpass = 2;
        }

        core::pod::array::push_back(_command_buffers_subpass, inheritance_info.subpass);

        return cmd_buff.handle;
    }

    void VulkanRenderSystem::submit_command_buffer(CommandBuffer cmd_buffer) noexcept
    {
        ApiCommandBuffer cmd_buff{ cmd_buffer };
        vkEndCommandBuffer(cmd_buff.native);

        auto cmd_buffer_index = _submitted_command_buffer_count.fetch_add(1);
        IS_ASSERT(cmd_buffer_index < core::pod::array::size(_command_buffers_secondary), "No more available command buffers!");

        _command_buffers_submitted[cmd_buffer_index] = cmd_buff.native;
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
        core::pod::Array<RenderResource> const& resources
    ) noexcept -> ResourceSet
    {
        VulkanPipelineLayout pipeline_layout =
            layout == RenderPipelineLayout::DebugUI
            ? _pipeline_layouts.debugui_layout
            : _pipeline_layouts.default_layout;

        VulkanResourceSet* resource_set = _allocator.make<VulkanResourceSet>();
        vulkan::create_resource_set(
            _devices.graphics.handle,
            _framebuffers[0],
            _resource_pool,
            pipeline_layout,
            _resource_layouts,
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
                _framebuffers[_current_framebuffer_index],
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

    auto VulkanRenderSystem::v1_framebuffer_semaphore() noexcept -> VkSemaphore const *
    {
        return &_framebuffer_semaphore;
    }

    auto VulkanRenderSystem::v1_secondary_cmd_buffer() noexcept -> VkCommandBuffer
    {
        return core::pod::array::front(_command_buffers_secondary);
    }

    auto VulkanRenderSystem::v1_primary_cmd_buffer() noexcept -> VkCommandBuffer
    {
        return _command_buffers.primary_buffers[0];
    }

    auto VulkanRenderSystem::v1_transfer_cmd_buffer() noexcept -> VkCommandBuffer
    {
        return _command_buffers.primary_buffers[1];
    }

    void VulkanRenderSystem::v1_submit_command_buffers() noexcept
    {
        const VkCommandBuffer cmd_bufs[] =
        {
            _command_buffers.primary_buffers[0]
        };

        VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSubmitInfo submit_info[1] = {};
        submit_info[0].pNext = NULL;
        submit_info[0].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info[0].waitSemaphoreCount = 1;
        submit_info[0].pWaitSemaphores = &_framebuffer_semaphore;
        submit_info[0].pWaitDstStageMask = &pipe_stage_flags;
        submit_info[0].commandBufferCount = 1;
        submit_info[0].pCommandBuffers = cmd_bufs;
        submit_info[0].signalSemaphoreCount = 0;
        submit_info[0].pSignalSemaphores = NULL;

        auto res = vkQueueSubmit(_devices.graphics.queue, 1, submit_info, _draw_fence);
        assert(res == VK_SUCCESS);

        do
        {
            constexpr auto FENCE_TIMEOUT = 100'000'000; // in ns
            res = vkWaitForFences(_devices.graphics.handle, 1, &_draw_fence, VK_TRUE, FENCE_TIMEOUT);
        } while (res == VK_TIMEOUT);

        vkResetFences(_devices.graphics.handle, 1, &_draw_fence);
    }

    void VulkanRenderSystem::v1_execute_subpass_commands(VkCommandBuffer cmds) noexcept
    {
        vkCmdExecuteCommands(cmds, _submitted_command_buffer_count, core::pod::array::begin(_command_buffers_submitted));

        _submitted_command_buffer_count = 0;
        core::pod::array::clear(_command_buffers_submitted);
    }

    void VulkanRenderSystem::acquire_next_image() noexcept
    {
        // Get the index of the next available swapchain image:
        auto api_result = vkAcquireNextImageKHR(
            _devices.graphics.handle,
            native_handle(_swapchain),
            UINT64_MAX,
            _framebuffer_semaphore,
            VK_NULL_HANDLE,
            &_current_framebuffer_index
        );

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

        _next_command_buffer = 0;
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
