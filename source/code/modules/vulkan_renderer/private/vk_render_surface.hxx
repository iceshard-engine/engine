/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_surface.hxx>
#include "vk_include.hxx"

namespace ice::render::vk
{

    class VulkanRenderSurface final : public ice::render::RenderSurface
    {
    public:
        VulkanRenderSurface(
            VkInstance vk_instance,
            VkSurfaceKHR vk_surface
        ) noexcept;
        ~VulkanRenderSurface() noexcept override;

        auto handle() const noexcept -> VkSurfaceKHR;

    private:
        VkInstance _vk_instance;
        VkSurfaceKHR _vk_surface;
    };

} // namespace ice::render::vk
