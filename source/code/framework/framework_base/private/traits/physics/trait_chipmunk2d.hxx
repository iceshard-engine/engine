/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/game_entity.hxx>
#include <ice/game_physics.hxx>
#include <ice/game_actor.hxx>

#include <ice/world/world_trait.hxx>
#include <ice/ecs/ecs_query.hxx>

#include "trait_chipmunk2d.hxx"
#include "chipmunk2d.hxx"

namespace ice
{

    class DevUI_Chipmunk2D;

#if 0
    class IceWorldTrait_PhysicsBox2D : public ice::WorldTrait_Physics2D
    {
    public:
        auto create_static_body(
            ice::vec2f position,
            ice::PhysicsShape shape,
            ice::vec2f dimensions
        ) noexcept -> ice::PhysicsID override;

        auto create_static_body(
            ice::vec2f position,
            ice::u32 vertice_count,
            ice::vec2f const* vertices
        ) noexcept -> ice::PhysicsID override;

        void destroy_body(
            ice::PhysicsID physics_id
        ) noexcept override;

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
        using DynamicQuery = ice::ecs::QueryDefinition<ice::ecs::EntityHandle, ice::Transform2DDynamic&, ice::PhysicsBody&, ice::Actor const*>;
        using PhysicsQuery = ice::ecs::QueryDefinition<ice::ecs::EntityHandle, ice::PhysicsBody&, ice::PhysicsVelocity&>;

        ice::Engine* _engine = nullptr;

        cpSpace* _global_space = nullptr;

        ice::DevUI_Chipmunk2D* _devui;
    };
#endif

} // namespace ice
