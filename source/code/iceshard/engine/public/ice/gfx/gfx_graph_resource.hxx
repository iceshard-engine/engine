/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice::gfx
{

    enum class GfxResourceType : uint8_t
    {
        Invalid = 0,
        RenderTarget,
        DepthStencil,
    };

    struct GfxResource
    {
        using TypeTag = ice::StrongValue;

        ice::uptr value;
    };

    inline auto gfx_resource_type_val(GfxResource res) noexcept
    {
        return ice::u32((res.value & 0x0000'00ff'0000'0000) >> 32);
    }

    inline auto gfx_resource_type(GfxResource res) noexcept
    {
        return static_cast<GfxResourceType>(gfx_resource_type_val(res));
    }

} // namespace ice::gfx
