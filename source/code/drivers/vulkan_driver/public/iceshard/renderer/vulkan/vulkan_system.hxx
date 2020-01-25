#pragma once
#include <core/allocator.hxx>
#include <iceshard/renderer/render_system.hxx>
#include <iceshard/renderer/render_pass.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>
#include <iceshard/renderer/vulkan/vulkan_surface.hxx>
#include <iceshard/renderer/vulkan/vulkan_devices.hxx>
#include <iceshard/renderer/vulkan/vulkan_swapchain.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>

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

        auto renderpass(RenderPassStage stage = RenderPassStage::Geometry) noexcept -> RenderPass;

        auto swapchain() noexcept -> VulkanSwapchain;

        auto render_area() noexcept -> VkExtent2D;

    public:
        auto v1_surface() noexcept -> VkSurfaceKHR;
        auto v1_physical_device() noexcept -> VkPhysicalDevice;
        auto v1_renderpass() noexcept -> VkRenderPass;
        auto v1_swapchain() noexcept -> VkSwapchainKHR;
        auto v1_device() noexcept -> VkDevice;

    public:
        void v1_create_swapchain() noexcept;
        void v1_destroy_swapchain() noexcept;

        void v1_destroy_renderpass() noexcept;
        void v1_set_graphics_device(VkDevice device) noexcept;

    private:
        core::allocator& _allocator;
        VkInstance const _vk_instance;

        VulkanSurface _surface;
        VulkanDevices _devices;
        VulkanSwapchain _swapchain;
        VulkanRenderPass _renderpass;
    };

    auto create_render_system(core::allocator& alloc, VkInstance device) noexcept -> VulkanRenderSystem*;

    void destroy_render_system(core::allocator& alloc, VulkanRenderSystem* system) noexcept;

} // namespace iceshard::renderer::vulkan
