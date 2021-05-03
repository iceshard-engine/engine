#pragma once
#include <ice/world/world_trait.hxx>

namespace ice::phx
{

    class PhxWorldTrait : public ice::WorldTrait
    {
    public:
        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::World& world
        ) noexcept override;
    };

} // namespace ice::phx
