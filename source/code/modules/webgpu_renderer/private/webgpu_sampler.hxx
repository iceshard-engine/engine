/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "webgpu_utils.hxx"
#include <ice/render/render_image.hxx>

namespace ice::render::webgpu
{

    auto native_filter(ice::render::SamplerFilter filter) noexcept -> WGPUFilterMode
    {
        switch (filter)
        {
        case SamplerFilter::Nearest: return WGPUFilterMode_Nearest;
        case SamplerFilter::Linear: return WGPUFilterMode_Linear;
        case SamplerFilter::CubicImg: // [[fallthrough]]
        case SamplerFilter::CubicExt: return WGPUFilterMode_Undefined;
        }
    }

    auto native_mipmap_mode(ice::render::SamplerMipMapMode mode) noexcept -> WGPUMipmapFilterMode
    {
        switch (mode)
        {
        case SamplerMipMapMode::Nearest: return WGPUMipmapFilterMode_Nearest;
        case SamplerMipMapMode::Linear: return WGPUMipmapFilterMode_Linear;
        case SamplerMipMapMode::None: return WGPUMipmapFilterMode_Undefined;
        }
    }

    auto native_address_mode(ice::render::SamplerAddressMode mode) noexcept -> WGPUAddressMode
    {
        switch (mode)
        {
        case SamplerAddressMode::ClampToBorder: // [[fallthrough]]
        case SamplerAddressMode::ClampToEdgeMirrored: // [[fallthrough]]
        case SamplerAddressMode::ClampToEdge: return WGPUAddressMode_ClampToEdge;
        case SamplerAddressMode::RepeatMirrored: return WGPUAddressMode_MirrorRepeat;
        case SamplerAddressMode::Repeat: return WGPUAddressMode_Repeat;
        }
    }

} // namespace ice::render::webgpu
