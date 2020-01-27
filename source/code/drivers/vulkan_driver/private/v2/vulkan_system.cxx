#include <iceshard/renderer/vulkan/vulkan_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <core/pod/array.hxx>

namespace iceshard::renderer::vulkan
{

    VulkanRenderSystem::VulkanRenderSystem(core::allocator& alloc, VkInstance instance) noexcept
        : _allocator{ alloc }
        , _vk_instance{ instance }
        , _framebuffers{ _allocator }
    {
        _surface = create_surface(_allocator, _vk_instance, { 1280, 720 });
        create_devices(_vk_instance, native_handle(_surface), _devices);
    }

    VulkanRenderSystem::~VulkanRenderSystem() noexcept
    {
        destroy_devices(_vk_instance, _devices);
        destroy_surface(_allocator, _surface);
    }

    void VulkanRenderSystem::prepare(VkExtent2D, RenderPassFeatures renderpass_features) noexcept
    {
        VkSurfaceFormatKHR format;
        get_surface_format(_devices.physical.handle, _surface, format);

        create_renderpass(_devices.graphics.handle, format.format, renderpass_features, _renderpass);
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

    auto VulkanRenderSystem::v1_physical_device() noexcept -> VkPhysicalDevice
    {
        return _devices.physical.handle;
    }

    auto VulkanRenderSystem::v1_renderpass() noexcept -> VkRenderPass
    {
        return _renderpass.renderpass;
    }

    auto VulkanRenderSystem::v1_swapchain() noexcept -> VkSwapchainKHR
    {
        return native_handle(_swapchain);
    }

    auto VulkanRenderSystem::v1_device() noexcept -> VkDevice
    {
        return _devices.graphics.handle;
    }

    auto VulkanRenderSystem::v1_current_framebuffer() noexcept -> VkFramebuffer
    {
        return native_handle(_framebuffers[_current_framebuffer_index]);
    }

    auto VulkanRenderSystem::v1_framebuffer_semaphore() noexcept -> VkSemaphore const *
    {
        return &_framebuffer_semaphore;
    }

    void VulkanRenderSystem::v1_create_framebuffers() noexcept
    {
        core::pod::array::clear(_framebuffers);
        create_framebuffers(
            _allocator,
            render_area(),
            _devices,
            _renderpass,
            _swapchain,
            _framebuffers
        );
    }

    void VulkanRenderSystem::v1_destroy_framebuffers() noexcept
    {
        destroy_framebuffers(
            _allocator,
            _framebuffers
        );
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

    void VulkanRenderSystem::v1_create_swapchain() noexcept
    {
        _swapchain = create_swapchain(_allocator, _devices, _surface);
    }

    void VulkanRenderSystem::v1_destroy_swapchain() noexcept
    {
        destroy_swapchain(_allocator, _swapchain);
    }

    void VulkanRenderSystem::v1_destroy_semaphore() noexcept
    {
        vkDestroySemaphore(_devices.graphics.handle, _framebuffer_semaphore, nullptr);
    }

    void VulkanRenderSystem::v1_present(VkQueue queue) noexcept
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

        auto api_result = vkQueuePresentKHR(queue, &present);
        IS_ASSERT(api_result == VK_SUCCESS, "Failed to present framebuffer image!");
    }

    void VulkanRenderSystem::v1_destroy_renderpass() noexcept
    {
        destroy_renderpass(_renderpass);
    }

    void VulkanRenderSystem::v1_set_graphics_device(VkDevice device) noexcept
    {
        IS_ASSERT(_framebuffer_semaphore == vk_nullptr, "Semaphore object is not a nullptr!");

        _devices.graphics.handle = device;

        VkSemaphoreCreateInfo semaphore_info;
        semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        semaphore_info.pNext = nullptr;
        semaphore_info.flags = 0;
        vkCreateSemaphore(_devices.graphics.handle, &semaphore_info, nullptr, &_framebuffer_semaphore);
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
