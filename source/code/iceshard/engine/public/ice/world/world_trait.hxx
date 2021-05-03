#pragma once

namespace ice
{

    class Engine;
    class EngineFrame;
    class EngineRunner;

    class World;

    class WorldTrait
    {
    public:
        virtual ~WorldTrait() noexcept = default;

        virtual void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept { }

        virtual void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept { }

        virtual void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept { }
    };

} // namespace ice
