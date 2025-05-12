/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_chipmunk2d.hxx"
#include "devui_chipmunk2d.hxx"
#include <ice/game_anim.hxx>

#include <ice/devui_context.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_shards.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>

#include <ice/input/input_device.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>

#include <ice/data_storage.hxx>
#include <ice/clock.hxx>

namespace ice
{

#if 0
    namespace detail
    {

        inline auto body_to_physics_id(cpBody* body) noexcept
        {
            return static_cast<ice::PhysicsID>(reinterpret_cast<ice::uptr>(body));
        }

        inline auto body_from_physics_id(ice::PhysicsID physics_id) noexcept
        {
            return reinterpret_cast<cpBody*>(static_cast<ice::uptr>(physics_id));
        }

    } // namespace detail

    auto IceWorldTrait_PhysicsBox2D::create_static_body(
        ice::vec2f position,
        ice::PhysicsShape shape,
        ice::vec2f dimensions
    ) noexcept -> ice::PhysicsID
    {
        cpBody* body = cpBodyNewStatic();
        cpBodySetPosition(body, cpVect{ position.x, position.y });
        cpBodySetUserData(body, (void*) static_cast<std::uintptr_t>(ice::ecs::EntityHandle::Invalid));
        cpSpaceAddBody(_global_space, body);

        if (shape == PhysicsShape::Box)
        {
            cpShape* shp = cpBoxShapeNew2(body, { 0, 0, dimensions.x, dimensions.y }, dimensions.x);
            cpShapeSetFriction(shp, 1.0);
        }

        return detail::body_to_physics_id(body);
    }

    auto IceWorldTrait_PhysicsBox2D::create_static_body(
        ice::vec2f position,
        ice::u32 vertice_count,
        ice::vec2f const* vertices
    ) noexcept -> ice::PhysicsID
    {
        cpBody* body = cpBodyNewStatic();
        cpBodySetPosition(body, cpVect{ position.x, position.y });
        cpBodySetUserData(body, (void*) static_cast<std::uintptr_t>(ice::ecs::EntityHandle::Invalid));
        cpSpaceAddBody(_global_space, body);

        cpShape* shape = cpPolyShapeNew(body, vertice_count, reinterpret_cast<cpVect const*>(vertices), cpTransformIdentity, 0.f);
        cpShapeSetFriction(shape, 1.0);

        return detail::body_to_physics_id(body);
    }

    void delete_shapes(cpBody* body, cpShape* shape, void* ud)
    {
        cpShapeDestroy(shape);
    }

    void IceWorldTrait_PhysicsBox2D::destroy_body(
        ice::PhysicsID physics_id
    ) noexcept
    {
        cpBody* body = detail::body_from_physics_id(physics_id);
        cpSpaceRemoveBody(_global_space, body);

        cpBodyEachShape(body, delete_shapes, nullptr);
        cpBodyDestroy(body);
    }

    void IceWorldTrait_PhysicsBox2D::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _engine = ice::addressof(engine);
        _global_space = cpSpaceNew();

        //_global_space = portal.allocator().create<cpSpace>();
        //cpSpaceInit(_global_space);

        cpVect gravity{ 0.f, -10.f };

        cpBody* body = cpBodyNewStatic();
        cpBodySetPosition(body, cpVect{ 0.0f, -0.5f });
        cpBodySetUserData(body, (void*) static_cast<std::uintptr_t>(ice::ecs::EntityHandle::Invalid));
        cpSpaceAddBody(_global_space, body);

        cpShape* shape = cpBoxShapeNew2(body, { 0, 0, 50.f, 0.5 }, 0.0f);
        cpShapeSetFriction(shape, 1.0);

        portal.storage().create_named_object<DynamicQuery::Query>("ice.query.physics_bodies"_sid, portal.entity_storage().create_query(portal.allocator(), DynamicQuery{}));
        portal.storage().create_named_object<PhysicsQuery::Query>("ice.query.physics_data"_sid, portal.entity_storage().create_query(portal.allocator(), PhysicsQuery{}));

        _devui = portal.allocator().create<ice::DevUI_Chipmunk2D>(*_global_space);
        engine.developer_ui().register_widget(_devui);
    }

    void IceWorldTrait_PhysicsBox2D::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        cpSpaceDestroy(_global_space);

        engine.developer_ui().unregister_widget(_devui);
        portal.allocator().destroy(_devui);
        _devui = nullptr;

        _engine = nullptr;

        DynamicQuery::Query& query = *portal.storage().named_object<DynamicQuery::Query>("ice.query.physics_bodies"_sid);

        ice::ecs::query::for_each_entity(
            query,
            [&](ice::ecs::EntityHandle, ice::Transform2DDynamic const& dyn_xform, ice::PhysicsBody& phx_body, ice::Actor const*) noexcept
            {
                phx_body.trait_data = nullptr;
            }
        );

        //portal.allocator().destroy(_world);
        //_world = nullptr;

        portal.storage().destroy_named_object<DynamicQuery::Query>("ice.query.physics_bodies"_sid);
        portal.storage().destroy_named_object<PhysicsQuery::Query>("ice.query.physics_data"_sid);
    }

    void IceWorldTrait_PhysicsBox2D::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        ice::shards::inspect_each<ice::ecs::EntityHandle>(
            frame.shards(),
            ice::ecs::Shard_EntityDestroyed,
            [this](ice::ecs::EntityHandle entity) noexcept
            {
                struct It
                {
                    cpSpace* space;
                    ice::ecs::EntityHandle entity;
                } it{ _global_space, entity };

                auto const body_iterator = [](cpBody* body, void* ud)
                {
                    auto const& userdata = cpBodyGetUserData(body);

                    if (static_cast<ice::ecs::EntityHandle>((uintptr_t)userdata) == reinterpret_cast<It*>(ud)->entity)
                    {
                        cpSpaceRemoveBody(reinterpret_cast<It*>(ud)->space, body);
                        cpBodyDestroy(body);
                    }
                };

                cpSpaceEachBody(_global_space, body_iterator, nullptr);
            }
        );

        PhysicsQuery::Query const& phx_query = *portal.storage().named_object<PhysicsQuery::Query>("ice.query.physics_data"_sid);

        ice::ecs::query::for_each_entity(
            phx_query,
            [](ice::ecs::EntityHandle e, ice::PhysicsBody& phx_body, ice::PhysicsVelocity& vel) noexcept
            {
                if (phx_body.trait_data != nullptr)
                {
                    cpBody* body = reinterpret_cast<cpBody*>(phx_body.trait_data);
                    cpVect velocity = cpBodyGetVelocity(body);

                    if (vel.velocity.y >= 0.01f || vel.velocity.y <= -0.01f)
                    {
                        velocity.y = vel.velocity.y;
                    }

                    if (vel.velocity.x >= 0.01f || vel.velocity.x <= -0.01f)
                    {
                        velocity.x = vel.velocity.x;
                    }

                    cpBodySetVelocity(body, velocity);
                }
            }
        );

        cpSpaceStep(_global_space, 1.f / 120.f);

        DynamicQuery::Query& query = *portal.storage().named_object<DynamicQuery::Query>("ice.query.physics_bodies"_sid);

        ice::ecs::query::for_each_entity(
            query,
            [&](ice::ecs::EntityHandle e, ice::Transform2DDynamic& dyn_xform, ice::PhysicsBody& phx_body, ice::Actor const* actor) noexcept
            {
                if (phx_body.trait_data == nullptr)
                {
                    ice::vec2f const half = (phx_body.dimensions / Constant_PixelsInMeter) / 2.f;
                    ice::vec2f const workaround_recenter = (ice::vec2f{ 48.f / 2, 0.f } / Constant_PixelsInMeter) - ice::vec2f{ half.x, 0.f };

                    cpBody* body = cpBodyNewKinematic();
                    cpBodySetPosition(body, { dyn_xform.position.x / Constant_PixelsInMeter, dyn_xform.position.y / Constant_PixelsInMeter });

                    cpBodySetUserData(body, (void*)static_cast<std::uintptr_t>(e));
                    phx_body.trait_data = body;

                    if (phx_body.shape == PhysicsShape::Box)
                    {
                        cpShape* shape = cpBoxShapeNew2(body, { half.x, half.y, half.x + workaround_recenter.x, half.y }, 0.0f);
                        cpShapeSetFriction(shape, 1.0f);
                        cpShapeSetDensity(shape, 1.0f);
                    }
                    else if (phx_body.shape == PhysicsShape::Capsule)
                    {
                        cpShape* shape = cpBoxShapeNew2(body, { half.x, half.y, half.x + workaround_recenter.x, half.y }, 0.0f);
                        cpShapeSetFriction(shape, 1.0f);
                        cpShapeSetDensity(shape, 1.0f);

                        //ice::vec2f const half_half = half / 2.f;

                        //b2CircleShape shape{ };
                        //shape.m_p = { half.x + workaround_recenter.x, half_half.y };
                        //shape.m_radius = half.x;

                        //b2PolygonShape tile_shape;
                        //tile_shape.SetAsBox(
                        //    half.x,
                        //    half_half.y,
                        //    { half.x + workaround_recenter.x, half.y },
                        //    0.f
                        //);

                        //b2FixtureDef fixture_def;
                        //fixture_def.shape = &shape;
                        //fixture_def.density = 1.0f;
                        //fixture_def.friction = 1.0f;
                        //body->CreateFixture(&fixture_def);

                        //shape.m_p.y += half.y;
                        //body->CreateFixture(&fixture_def);

                        //fixture_def.shape = &tile_shape;
                        //body->CreateFixture(&fixture_def);
                    }

                    //body->SetFixedRotation(true);
                }
                else
                {
                    cpBody* body = reinterpret_cast<cpBody*>(phx_body.trait_data);
                    cpVect body_pos = cpBodyGetPosition(body);

                    dyn_xform.position = ice::vec3f{ (float) body_pos.x * Constant_PixelsInMeter, (float)body_pos.y * Constant_PixelsInMeter, dyn_xform.position.z };
                }
            }
        );

        _devui->on_frame(frame);
    }

    auto create_trait_physics(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::WorldTrait_Physics2D>
    {
        return ice::make_unique<ice::IceWorldTrait_PhysicsBox2D>(alloc);
    }
#endif

} // namespace ice
