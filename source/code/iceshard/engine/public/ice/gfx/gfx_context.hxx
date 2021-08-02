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
