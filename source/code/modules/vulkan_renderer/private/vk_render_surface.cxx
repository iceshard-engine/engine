/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_render_surface.hxx"

namespace ice::render::vk
{

    VulkanRenderSurface::VulkanRenderSurface(
        VkInstance vk_instance,
        VkSurfaceKHR vk_surface
    ) noexcept
        : _vk_instance{ vk_instance }
        , _vk_surface{ vk_surface }
    {
    }

    VulkanRenderSurface::~VulkanRenderSurface() noexcept
    {
        vkDestroySurfaceKHR(_vk_instance, _vk_surface, nullptr);
    }

    auto VulkanRenderSurface::handle() const noexcept -> VkSurfaceKHR
    {
        return _vk_surface;
    }

} // namespace ice::render::vk
