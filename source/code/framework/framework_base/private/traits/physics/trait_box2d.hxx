#pragma once
#include <ice/game_entity.hxx>
#include <ice/game_physics.hxx>
#include <ice/game_actor.hxx>

#include <ice/world/world_trait.hxx>
#include <ice/archetype/archetype_query.hxx>

#include "box2d.hxx"

namespace ice
{

    class IceWorldTrait_PhysicsBox2D : public ice::WorldTrait
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
        using DynamicQuery = ice::ComponentQuery<ice::Entity, ice::Transform2DDynamic&, ice::PhysicsBody&, ice::Actor const*>;

        ice::Engine* _engine = nullptr;
        b2World* _world = nullptr;
    };

} // namespace ice
