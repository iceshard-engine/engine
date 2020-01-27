#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>
#include <iceshard/renderer/vulkan/vulkan_surface.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <iceshard/renderer/vulkan/vulkan_swapchain.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>
#include <iceshard/renderer/vulkan/vulkan_framebuffer.hxx>
#include <iceshard/renderer/vulkan/vulkan_command_buffer.hxx>

namespace iceshard::renderer::vulkan
{

    class VulkanRenderSystem final : iceshard::renderer::RenderSystem
    {
    public:
        VulkanRenderSystem(core::allocator& alloc, VkInstance instance) noexcept;
        ~VulkanRenderSystem() noexcept override;

        void prepare(
            VkExtent2D surface_extent,
            RenderPassFeatures renderpass_features
        ) noexcept;

        auto devices() noexcept -> VulkanDevices const& { return _devices; }

        auto renderpass(RenderPassStage stage = RenderPassStage::Geometry) noexcept -> RenderPass;

        auto swapchain() noexcept -> VulkanSwapchain;

        auto render_area() noexcept -> VkExtent2D;

    public:
        auto v1_surface() noexcept -> VkSurfaceKHR;
        auto v1_physical_device() noexcept -> VkPhysicalDevice;
        auto v1_graphics_device() noexcept -> VkDevice;
        auto v1_graphics_queue() noexcept -> VkQueue;
        auto v1_renderpass() noexcept -> VkRenderPass;
        auto v1_swapchain() noexcept -> VkSwapchainKHR;
        auto v1_device() noexcept -> VkDevice;
        auto v1_current_framebuffer() noexcept -> VkFramebuffer;
        auto v1_framebuffer_semaphore() noexcept -> VkSemaphore const*;

        auto v1_graphics_cmd_buffer() noexcept -> VkCommandBuffer;
        auto v1_transfer_cmd_buffer() noexcept -> VkCommandBuffer;

    public:
        void v1_acquire_next_image() noexcept;
        void v1_present(VkQueue queue) noexcept;

    private:
        core::allocator& _allocator;
        VkInstance const _vk_instance;

        VulkanSurface _surface;
        VulkanDevices _devices;
        VulkanSwapchain _swapchain;
        VulkanRenderPass _renderpass;
        VulkanCommandBuffers _command_buffers;

        uint32_t _current_framebuffer_index = 0;
        core::pod::Array<VulkanFramebuffer> _framebuffers;
        VkSemaphore _framebuffer_semaphore = vk_nullptr;

        bool _initialized = false;
    };

    auto create_render_system(core::allocator& alloc, VkInstance device) noexcept -> VulkanRenderSystem*;

    void destroy_render_system(core::allocator& alloc, VulkanRenderSystem* system) noexcept;

} // namespace iceshard::renderer::vulkan
