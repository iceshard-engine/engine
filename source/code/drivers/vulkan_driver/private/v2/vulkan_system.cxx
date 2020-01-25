#include <iceshard/renderer/vulkan/vulkan_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>

namespace iceshard::renderer::vulkan
{

    VulkanRenderSystem::VulkanRenderSystem(core::allocator& alloc, VkInstance instance) noexcept
        : _allocator{ alloc }
        , _vk_instance{ instance }
    {
        _surface = create_surface(_allocator, _vk_instance, { 1280, 720 });
        create_devices(_vk_instance, _devices);
    }

    VulkanRenderSystem::~VulkanRenderSystem() noexcept
    {
        destroy_devices(_vk_instance, _devices);
        destroy_surface(_allocator, _surface);
    }

    void VulkanRenderSystem::prepare(VkExtent2D, VkFormat renderpass_format, RenderPassFeatures renderpass_features) noexcept
    {
        create_renderpass(_devices.graphics_device, renderpass_format, renderpass_features, _renderpass);
    }

    auto VulkanRenderSystem::renderpass([[maybe_unused]] RenderPassStage stage) noexcept -> RenderPass
    {
        return RenderPass{ reinterpret_cast<uintptr_t>(_renderpass.renderpass) };
    }

    auto VulkanRenderSystem::renderpass_native([[maybe_unused]] RenderPassStage stage) noexcept -> VkRenderPass
    {
        return _renderpass.renderpass;
    }

    auto VulkanRenderSystem::v1_surface() noexcept -> VkSurfaceKHR
    {
        return native_handle(_surface);
    }

    auto VulkanRenderSystem::v1_physical_device() noexcept -> VkPhysicalDevice
    {
        return _devices.physical_device;
    }

    void VulkanRenderSystem::v1_destroy_renderpass() noexcept
    {
        destroy_renderpass(_renderpass);
    }

    void VulkanRenderSystem::v1_set_graphics_device(VkDevice device) noexcept
    {
        _devices.graphics_device = device;
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
