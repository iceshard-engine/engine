#pragma once
#include <iceshard/renderer/render_viewport.hxx>
#include <iceshard/renderer/vulkan/vulkan_sdk.hxx>

namespace iceshard::renderer::vulkan
{

    enum class VulkanSurface : uintptr_t;

    class VulkanViewport : public iceshard::renderer::RenderViewport
    {
    public:
        VulkanViewport(ViewportExtents extents) noexcept;
        ~VulkanViewport() noexcept override;

        auto extents() const noexcept -> ViewportExtents override;

        bool update_viewport(ViewportExtents extents) const noexcept override;

    private:
        VulkanSurface _surface;
        VkViewport _viewport;
    };

} // namespace iceshard::renderer::vulkan

