#include <iceshard/renderer/vulkan/vulkan_system.hxx>
#include <iceshard/renderer/vulkan/vulkan_renderpass.hxx>

namespace iceshard::renderer::vulkan
{

    VulkanRenderSystem::VulkanRenderSystem(core::allocator& alloc, VkDevice device) noexcept
        : _allocator{ alloc }
        , _vk_device{ device }
    {
    }

    VulkanRenderSystem::~VulkanRenderSystem() noexcept
    {
        destroy_renderpass(_vk_device, _render_pass);
    }

    void VulkanRenderSystem::prepare(VkFormat format, RenderPassFeatures renderpass_features) noexcept
    {
        _render_pass = create_renderpass(_vk_device, format, renderpass_features);
    }

    auto VulkanRenderSystem::renderpass([[maybe_unused]] RenderPassStage stage) noexcept -> RenderPass
    {
        return _render_pass;
    }

    auto create_render_system(core::allocator& alloc, VkDevice device) noexcept -> VulkanRenderSystem*
    {
        return alloc.make<VulkanRenderSystem>(alloc, device);
    }

    void destroy_render_system(core::allocator& alloc, VulkanRenderSystem* system) noexcept
    {
        alloc.destroy(system);
    }

} // namespace iceshard::renderer::vulkan
