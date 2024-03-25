/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_swapchain.hxx"
#include <ice/render/render_image.hxx>

namespace ice::render::webgpu
{

    WebGPUSwapchain::WebGPUSwapchain(
        WGPUSwapChain wgpu_swapchain,
        WGPUTextureFormat wgpu_format,
        ice::vec2u extent
    ) noexcept
        : _wgpu_swapchain{ wgpu_swapchain }
        , _wgpu_format{ wgpu_format }
        , _extent{ extent }
        , _images{ { }, { } }
        , _current_index{ 1 }
    {
        // We aquire the first image so we don't need to handle a special case in 'aquire_image'
        _images[_current_index].wgpu_texture_view = wgpuSwapChainGetCurrentTextureView(_wgpu_swapchain);
    }

    WebGPUSwapchain::~WebGPUSwapchain() noexcept
    {
        wgpuTextureRelease(_images[_current_index].wgpu_texture);
        wgpuSwapChainRelease(_wgpu_swapchain);
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
        wgpuTextureViewRelease(_images[_current_index].wgpu_texture_view);
        _current_index = (_current_index + 1) % image_count();
        // And allocate the new texture for the current frame
        _images[_current_index].wgpu_texture_view = wgpuSwapChainGetCurrentTextureView(_wgpu_swapchain);
        return _current_index;
    }

    auto WebGPUSwapchain::current_image_index() const noexcept -> ice::u32
    {
        return _current_index;
    }

} // namespace ice::render::webgpu
