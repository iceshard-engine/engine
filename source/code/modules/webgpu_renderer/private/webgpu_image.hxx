/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_image.hxx>
#include "webgpu_utils.hxx"

namespace ice::render::webgpu
{

    struct WebGPUImage
    {
        WGPUTexture wgpu_texture;
        WGPUTextureView wgpu_texture_view;

        static auto handle(WebGPUImage const* native) noexcept
        {
            return static_cast<ice::render::Image>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::Image handle) noexcept
        {
            return reinterpret_cast<WebGPUImage const*>(static_cast<uintptr_t>(handle));
        }
    };

    inline auto native_usage(ice::render::ImageUsageFlags flags) noexcept -> WGPUTextureUsage
    {
        using enum ImageUsageFlags;

        WGPUTextureUsage result = WGPUTextureUsage_None;
        if (ice::has_any(flags, InputAttachment | ColorAttachment | DepthStencilAttachment))
        {
            result = WGPUTextureUsage(result | WGPUTextureUsage_RenderAttachment);
        }
        if (ice::has_any(flags, Sampled))
        {
            result = WGPUTextureUsage(result | WGPUTextureUsage_TextureBinding);
        }
        if (ice::has_any(flags, TransferDst))
        {
            result = WGPUTextureUsage(result | WGPUTextureUsage_CopyDst);
        }
        return result;
    }

    inline auto native_format(ice::render::ImageFormat format) noexcept -> WGPUTextureFormat
    {
        switch(format)
        {
            using enum ImageFormat;
        case I32_RGBA: return WGPUTextureFormat_R32Sint;
        case SRGB_RGBA: return WGPUTextureFormat_RGBA8UnormSrgb;
        case SRGB_BGRA: return WGPUTextureFormat_BGRA8UnormSrgb;
        case UNORM_RGBA: return WGPUTextureFormat_RGBA8Unorm;
        case UNORM_BGRA: return WGPUTextureFormat_BGRA8Unorm;
        case UNORM_D24_UINT_S8: return WGPUTextureFormat_Depth24PlusStencil8;
        case SFLOAT_D32: return WGPUTextureFormat_Depth32Float;
        case SFLOAT_D32_UINT_S8: return WGPUTextureFormat_Depth32FloatStencil8;
        case UNORM_RGB: // [[fallthrough]]
        case UNORM_ARGB: // [[fallthrough]]
        default:
            return WGPUTextureFormat_Undefined;
        }
    }

    inline auto api_format(WGPUTextureFormat format) noexcept -> ice::render::ImageFormat
    {
        switch (format)
        {
            using enum ImageFormat;
        case WGPUTextureFormat_R32Sint: return I32_RGBA;
        case WGPUTextureFormat_RGBA8UnormSrgb: return SRGB_RGBA;
        case WGPUTextureFormat_BGRA8UnormSrgb: return SRGB_BGRA;
        case WGPUTextureFormat_RGBA8Unorm: return UNORM_RGBA;
        case WGPUTextureFormat_BGRA8Unorm: return UNORM_BGRA;
        case WGPUTextureFormat_Depth24PlusStencil8: return UNORM_D24_UINT_S8;
        case WGPUTextureFormat_Depth32Float: return SFLOAT_D32;
        case WGPUTextureFormat_Depth32FloatStencil8: return SFLOAT_D32_UINT_S8;
        default:
            return ImageFormat::Invalid;
        }
    }

} // namespace ice::render::webgpu
