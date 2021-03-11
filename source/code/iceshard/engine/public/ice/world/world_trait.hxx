#pragma once

namespace ice
{

    class EngineFrame;

    class EngineRunner;

    class World;

    class WorldTrait
    {
    public:
        virtual ~WorldTrait() noexcept = default;

        virtual void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept = 0;
    };

} // namespace ice
