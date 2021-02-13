#pragma once
#include <ice/clock.hxx>

namespace ice
{

    class EngineFrame;

    class EngineRunner
    {
    public:
        virtual ~EngineRunner() noexcept = default;

        virtual auto clock() const noexcept -> ice::Clock const& = 0;

        virtual auto previous_frame() const noexcept -> EngineFrame const& = 0;

        virtual auto current_frame() const noexcept -> EngineFrame const& = 0;

        virtual auto current_frame() noexcept -> EngineFrame& = 0;

        virtual void next_frame() noexcept = 0;
    };

} // namespace ice
