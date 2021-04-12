#pragma once
#include <ice/span.hxx>

namespace ice
{

    struct EngineRequest;

    class EngineFrame
    {
    public:
        virtual ~EngineFrame() noexcept = default;

        virtual auto memory_consumption() noexcept -> ice::u32 = 0;

        virtual void push_requests(
            ice::Span<EngineRequest const> requests
        ) noexcept = 0;
    };

} // namespace ice
