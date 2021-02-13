#pragma once
#include <ice/base.hxx>

namespace ice
{

    class EngineFrame
    {
    public:
        virtual ~EngineFrame() noexcept = default;

        virtual auto memory_consumption() noexcept -> ice::u32 = 0;
    };

} // namespace ice
