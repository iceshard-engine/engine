#include <iceshard/renderer/vulkan/vulkan_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    VulkanRenderSystem::VulkanRenderSystem(core::allocator& alloc, VkInstance instance) noexcept
        : _allocator{ alloc }
        , _vk_instance{ instance }
        , _framebuffers{ _allocator }
        , _command_buffers_secondary{ _allocator }
        , _command_buffers_submitted{ _allocator }
        , _resource_sets { _allocator }
    {
        _surface = create_surface(_allocator, _vk_instance, { 1280, 720 });
        create_devices(_vk_instance, native_handle(_surface), _devices);

        allocate_command_buffers(_devices, _command_buffers, 6, _command_buffers_secondary);
        core::pod::array::resize(_command_buffers_submitted, core::pod::array::size(_command_buffers_secondary));

        VkSemaphoreCreateInfo semaphore_info;
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;
        semaphore_info.flags = 0;
        vkCreateSemaphore(_devices.graphics.handle, &semaphore_info, nullptr, &_framebuffer_semaphore);

        VkSurfaceFormatKHR format;
        get_surface_format(_devices.physical.handle, _surface, format);
        create_renderpass(_devices.graphics.handle, format.format, RenderPassFeatures::None, _renderpass);

        create_resource_pool(_devices.graphics.handle, _resource_pool);
        create_resource_layouts(_devices.graphics.handle, _resource_layouts);

        prepare(render_area(), RenderPassFeatures::None);
        _initialized = true;
    }

    VulkanRenderSystem::~VulkanRenderSystem() noexcept
    {
        destroy_framebuffers(_allocator, _framebuffers);
        destroy_swapchain(_allocator, _swapchain);

        destroy_resource_layouts(_devices.graphics.handle, _resource_layouts);
        destroy_resource_pool(_devices.graphics.handle, _resource_pool);

        destroy_renderpass(_renderpass);

        vkDestroySemaphore(_devices.graphics.handle, _framebuffer_semaphore, nullptr);

        release_command_buffers(_devices, _command_buffers, _command_buffers_secondary);
        destroy_devices(_vk_instance, _devices);
        destroy_surface(_allocator, _surface);
    }

    void VulkanRenderSystem::prepare(VkExtent2D area, RenderPassFeatures) noexcept
    {
        if (_initialized)
        {
            destroy_framebuffers(_allocator, _framebuffers);
            destroy_swapchain(_allocator, _swapchain);
        }

        _swapchain = create_swapchain(_allocator, _devices, _surface);
        create_framebuffers(_allocator, area, _devices, _renderpass, _swapchain, _framebuffers);
    }

    auto VulkanRenderSystem::renderpass([[maybe_unused]] RenderPassStage stage) noexcept -> RenderPass
    {
        return RenderPass{ reinterpret_cast<uintptr_t>(_renderpass.renderpass) };
    }

    auto VulkanRenderSystem::acquire_command_buffer(RenderPassStage) noexcept -> CommandBuffer
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
        {
            auto draw_area_ext = render_area();

            float scale[2];
            scale[0] = 2.0f / draw_area_ext.width;
            scale[1] = 2.0f / draw_area_ext.height;
            float translate[2];
            translate[0] = -1.0f; // -1.0f - width * scale[0];
            translate[1] = -1.0f; //-1.0f - height * scale[1];
            vkCmdPushConstants(cmd_buff.native, _resource_layouts.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 0, sizeof(float) * 2, scale);
            vkCmdPushConstants(cmd_buff.native, _resource_layouts.pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, sizeof(float) * 2, sizeof(float) * 2, translate);
        }

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
        VkSurfaceCapabilitiesKHR capabilities;
        get_surface_capabilities(_devices.physical.handle, _surface, capabilities);
        return capabilities.currentExtent;
    }

    auto VulkanRenderSystem::resource_layouts() noexcept -> VulkanResourceLayouts
    {
        return _resource_layouts;
    }

    auto VulkanRenderSystem::create_resource_set(
        core::stringid_arg_type name,
        core::pod::Array<RenderResource> const& resources
    ) noexcept -> ResourceSet
    {
        VulkanResourceSet* resource_set = _allocator.make<VulkanResourceSet>();
        vulkan::create_resource_set(
            _devices.graphics.handle,
            _resource_pool,
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
        [[maybe_unused]] core::stringid_arg_type name,
        [[maybe_unused]] core::pod::Array<RenderResource> const& resources
    ) noexcept
    {
        IS_ASSERT(false, "Currently not implemented!");
    }

    void VulkanRenderSystem::destroy_resource_set(
        core::stringid_arg_type name
    ) noexcept
    {
        auto* const resource_set = core::pod::hash::get<VulkanResourceSet*>(_resource_sets, core::hash(name), nullptr);
        if (resource_set != nullptr)
        {
            _allocator.destroy(resource_set);
        }
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

    void VulkanRenderSystem::v1_execute_subpass_commands(VkCommandBuffer cmds) noexcept
    {
        vkCmdExecuteCommands(cmds, _submitted_command_buffer_count, core::pod::array::begin(_command_buffers_submitted));

        _submitted_command_buffer_count = 0;
        core::pod::array::clear(_command_buffers_submitted);
    }

    void VulkanRenderSystem::v1_acquire_next_image() noexcept
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

    void VulkanRenderSystem::v1_present() noexcept
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

    auto create_render_system(core::allocator& alloc, VkInstance instance) noexcept -> VulkanRenderSystem*
    {
        return alloc.make<VulkanRenderSystem>(alloc, instance);
    }

    void destroy_render_system(core::allocator& alloc, VulkanRenderSystem* system) noexcept
    {
        alloc.destroy(system);
    }

} // namespace iceshard::renderer::vulkan
