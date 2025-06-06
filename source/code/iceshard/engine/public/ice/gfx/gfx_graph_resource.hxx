/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice::gfx
{

    enum class GfxResourceType : ice::u8
    {
        Invalid = 0,
        RenderTarget,
        DepthStencil,
    };

    struct GfxResource
    {
        using TypeTag = ice::StrongValue;

        ice::u64 value;
    };

    constexpr auto gfx_resource_type_val(ice::gfx::GfxResource res) noexcept -> ice::u32
    {
        return ice::u32((res.value & 0x0000'00ff'0000'0000) >> 32);
    }

    constexpr auto gfx_resource_type(ice::gfx::GfxResource res) noexcept -> ice::gfx::GfxResourceType
    {
        return static_cast<GfxResourceType>(gfx_resource_type_val(res));
    }

} // namespace ice::gfx
