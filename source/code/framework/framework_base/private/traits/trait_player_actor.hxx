#pragma once
#include <ice/game_actor.hxx>
#include <ice/game_entity.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/archetype/archetype_query.hxx>

namespace ice
{

    class IceWorldTrait_PlayerActor : public ice::WorldTrait
    {
    public:
        void on_activate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_deactivate(
            ice::Engine& engine,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

        void on_update(
            ice::EngineFrame& frame,
            ice::EngineRunner& runner,
            ice::WorldPortal& portal
        ) noexcept override;

    private:
        using Query = ice::ComponentQuery<ice::Actor const&, ice::Transform2DStatic&>;
    };

} // namespace ice
