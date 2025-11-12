/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_swapchain.hxx"
#include <ice/render/render_image.hxx>

namespace ice::render::webgpu
{

    WebGPUSwapchain::WebGPUSwapchain(
        WGPUSurface wgpu_surface,
        WGPUTextureFormat wgpu_format,
        ice::vec2u extent
    ) noexcept
        : _wgpu_surface{ wgpu_surface }
        , _wgpu_format{ wgpu_format }
        , _extent{ extent }
        , _images{ { }, { } }
        , _current_index{ 1 }
    {
        wgpuSurfaceGetCurrentTexture(_wgpu_surface, &_texture);
        ICE_ASSERT_CORE(_texture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
            || _texture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal);
        // We aquire the first image so we don't need to handle a special case in 'aquire_image'
        _images[_current_index].wgpu_texture = _texture.texture;

        WGPUTextureViewDescriptor view_descriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
        view_descriptor.arrayLayerCount = 1;
        view_descriptor.baseArrayLayer = 0;
        view_descriptor.mipLevelCount = 1;
        view_descriptor.baseMipLevel = 0;
        view_descriptor.format = _wgpu_format;
        view_descriptor.aspect = WGPUTextureAspect_All;
        view_descriptor.usage = WGPUTextureUsage_RenderAttachment;
        _images[_current_index].wgpu_texture_view = wgpuTextureCreateView(_texture.texture, &view_descriptor);
    }

    WebGPUSwapchain::~WebGPUSwapchain() noexcept
    {
        wgpuSurfaceUnconfigure(_wgpu_surface);
    }

    auto WebGPUSwapchain::extent() const noexcept -> ice::vec2u
    {
        return _extent;
    }

    auto WebGPUSwapchain::image_format() const noexcept -> ice::render::ImageFormat
    {
        return api_format(_wgpu_format);
    }

    auto WebGPUSwapchain::image_count() const noexcept -> ice::u32
    {
        return 2;
    }

    auto WebGPUSwapchain::image(ice::u32 index) const noexcept -> ice::render::Image
    {
        return WebGPUImage::handle(_images + index);
    }

    auto WebGPUSwapchain::aquire_image() noexcept -> ice::u32
    {
        // We release the previous textures since it's no longer needed
        wgpuTextureRelease(_images[_current_index].wgpu_texture);
        wgpuTextureViewRelease(_images[_current_index].wgpu_texture_view);

        _current_index = (_current_index + 1) % image_count();
        // And allocate the new texture for the current frame
        wgpuSurfaceGetCurrentTexture(_wgpu_surface, &_texture);
        ICE_ASSERT_CORE(_texture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessOptimal
            || _texture.status == WGPUSurfaceGetCurrentTextureStatus_SuccessSuboptimal);
        // We aquire the first image so we don't need to handle a special case in 'aquire_image'
        _images[_current_index].wgpu_texture = _texture.texture;

        WGPUTextureViewDescriptor view_descriptor = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
        view_descriptor.arrayLayerCount = 1;
        view_descriptor.baseArrayLayer = 0;
        view_descriptor.mipLevelCount = 1;
        view_descriptor.baseMipLevel = 0;
        view_descriptor.format = _wgpu_format;
        view_descriptor.aspect = WGPUTextureAspect_All;
        view_descriptor.usage = WGPUTextureUsage_RenderAttachment;
        _images[_current_index].wgpu_texture_view = wgpuTextureCreateView(_texture.texture, &view_descriptor);

        return _current_index;
    }

    auto WebGPUSwapchain::current_image_index() const noexcept -> ice::u32
    {
        return _current_index;
    }

} // namespace ice::render::webgpu
