#pragma once
#include <ice/task.hxx>

namespace ice::gfx
{

    // TODO: Remove this as we don't want access to a frame from a context.
    //  However it's currently be easiest way to detangle a Gfx from Engine.
    class GfxFrame;

    class GfxContext
    {
    public:
        virtual ~GfxContext() noexcept = default;

        [[deprecated("TEMPORARY")]]
        virtual auto frame() noexcept -> ice::gfx::GfxFrame& = 0;

        [[deprecated("TEMPORARY")]]
        virtual void add_task(ice::Task<> task) noexcept = 0;
    };

} // namespace ice::gfx
