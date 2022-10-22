/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>

namespace ice::gfx
{

    class GfxContext
    {
    public:
        virtual ~GfxContext() noexcept = default;
    };

} // namespace ice::gfx
