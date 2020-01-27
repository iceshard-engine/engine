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
    {
        _surface = create_surface(_allocator, _vk_instance, { 1280, 720 });
        create_devices(_vk_instance, native_handle(_surface), _devices);

        allocate_command_buffers(_devices, _command_buffers, 6, _command_buffers_secondary);

        VkSemaphoreCreateInfo semaphore_info;
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;
        semaphore_info.flags = 0;
        vkCreateSemaphore(_devices.graphics.handle, &semaphore_info, nullptr, &_framebuffer_semaphore);

        VkSurfaceFormatKHR format;
        get_surface_format(_devices.physical.handle, _surface, format);
        create_renderpass(_devices.graphics.handle, format.format, RenderPassFeatures::None, _renderpass);

        prepare(render_area(), RenderPassFeatures::None);
        _initialized = true;
    }

    VulkanRenderSystem::~VulkanRenderSystem() noexcept
    {
        destroy_framebuffers(_allocator, _framebuffers);
        destroy_swapchain(_allocator, _swapchain);

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
