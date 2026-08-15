/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
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
            VkSurfaceKHR vk_surface,
            ice::render::NativeSurface const* native_surface
        ) noexcept;
        ~VulkanRenderSurface() noexcept override;

        [[nodiscard]]
        auto handle() const noexcept -> VkSurfaceKHR;

        [[nodiscard]]
        auto dimensions() const noexcept -> ice::vec2u { return _native_surface->dimensions(); }

    private:
        VkInstance _vk_instance;
        VkSurfaceKHR _vk_surface;

        ice::render::NativeSurface const* _native_surface;
    };

} // namespace ice::render::vk
