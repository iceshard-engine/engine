#include <iceshard/renderer/vulkan/vulkan_viewport.hxx>
#include "vulkan_surface.hxx"

namespace iceshard::renderer::vulkan
{

    VulkanViewport::VulkanViewport(ViewportExtents) noexcept
        : _surface{ create_surface() }
    {
    }

    VulkanViewport::~VulkanViewport() noexcept
    {
        destroy_surface(_surface);
    }

    auto VulkanViewport::extents() const noexcept -> ViewportExtents
    {
        return { };
    }

    bool VulkanViewport::update_viewport(ViewportExtents) const noexcept
    {
        return false;
    }

} // namespace iceshard::renderer::vulkan
