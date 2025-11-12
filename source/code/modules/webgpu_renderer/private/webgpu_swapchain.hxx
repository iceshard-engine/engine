/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_swapchain.hxx>
#include "webgpu_image.hxx"
#include "webgpu_utils.hxx"

namespace ice::render::webgpu
{

    class WebGPUSwapchain : public ice::render::RenderSwapchain
    {
    public:
        WebGPUSwapchain(
            WGPUSurface wgpu_surface,
            WGPUTextureFormat wgpu_format,
            ice::vec2u extent
        ) noexcept;
        ~WebGPUSwapchain() noexcept override;

        auto extent() const noexcept -> ice::vec2u override;

        auto image_format() const noexcept -> ice::render::ImageFormat override;

        auto image_count() const noexcept -> ice::u32 override;

        auto image(ice::u32 index) const noexcept -> ice::render::Image override;

        auto aquire_image() noexcept -> ice::u32 override;

        auto current_image_index() const noexcept -> ice::u32 override;

        WGPUSurface const _wgpu_surface;
    private:
        WGPUTextureFormat const _wgpu_format;
        ice::vec2u const _extent;
        WGPUSurfaceTexture _texture;
        WebGPUImage _images[2];
        uint32_t _current_index;
    };

} // namespace ice::render::webgpu
