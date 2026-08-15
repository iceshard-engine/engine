/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "vk_render_surface.hxx"

namespace ice::render::vk
{

    VulkanRenderSurface::VulkanRenderSurface(
        VkInstance vk_instance,
        VkSurfaceKHR vk_surface,
        ice::render::NativeSurface const* native_surface
    ) noexcept
        : _vk_instance{ vk_instance }
        , _vk_surface{ vk_surface }
        , _native_surface{ native_surface }
    {
        ICE_ASSERT_CORE(_native_surface != nullptr);
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
