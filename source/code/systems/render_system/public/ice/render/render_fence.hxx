#pragma once
#include <ice/base.hxx>

namespace ice::render
{

    class RenderFence
    {
    protected:
        virtual ~RenderFence() noexcept = default;

    public:
        virtual bool wait(ice::u64 timeout_ns) noexcept = 0;
        virtual void reset() noexcept = 0;
    };

} // namespace ice::render
