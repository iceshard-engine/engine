#pragma once
#include <ice/clock.hxx>
#include <ice/gfx/gfx_types.hxx>

namespace ice
{

    class World;

    class EngineFrame;

    class EngineRunner
    {
    public:
        virtual ~EngineRunner() noexcept = default;

        virtual auto graphics_device() noexcept -> ice::gfx::GfxDevice& = 0;

        virtual auto graphics_frame() noexcept -> ice::gfx::GfxFrame& = 0;

        virtual auto clock() const noexcept -> ice::Clock const& = 0;

        virtual auto previous_frame() const noexcept -> EngineFrame const& = 0;

        virtual auto current_frame() const noexcept -> EngineFrame const& = 0;

        virtual auto current_frame() noexcept -> EngineFrame& = 0;

        virtual void next_frame() noexcept = 0;

        virtual void update_world(
            ice::World* world
        ) noexcept = 0;
    };

} // namespace ice
