/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_context.hxx>

namespace ice::gfx
{

    class IceGfxWorld : public ice::gfx::GfxContext
    {
    public:
        IceGfxWorld() noexcept;
        ~IceGfxWorld() noexcept override;

        void activate() noexcept;
        void decativate() noexcept;
    };

} // namespace ice::gfx
