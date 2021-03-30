#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>

namespace ice::gfx
{

    class GfxPass;

    class GfxQueue
    {
    public:
        virtual ~GfxQueue() noexcept = default;

        virtual bool presenting() const noexcept = 0;
        virtual void set_presenting(bool is_presenting) noexcept = 0;

        virtual void execute_pass(
            ice::gfx::GfxPass* gfx_pass
        ) noexcept = 0;
    };

} // namespace ice::gfx
