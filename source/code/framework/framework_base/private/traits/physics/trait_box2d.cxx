#include "trait_box2d.hxx"
#include <ice/game_anim.hxx>

#include <ice/devui/devui_system.hxx>

#include <ice/engine.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_shards.hxx>
#include <ice/entity/entity_index.hxx>
#include <ice/world/world_portal.hxx>

#include <ice/input/input_device.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>

#include <ice/data_storage.hxx>
#include <ice/span_filter.hxx>
#include <ice/clock.hxx>

namespace ice
{

    namespace detail
    {

        inline auto body_to_physics_id(b2Body* body) noexcept
        {
            return static_cast<ice::PhysicsID>(reinterpret_cast<ice::uptr>(body));
        }

        inline auto body_from_physics_id(ice::PhysicsID physics_id) noexcept
        {
            return reinterpret_cast<b2Body*>(static_cast<ice::uptr>(physics_id));
        }

    } // namespace detail

    auto IceWorldTrait_PhysicsBox2D::create_static_body(
        ice::vec2f position,
        ice::PhysicsShape shape,
        ice::vec2f dimensions
    ) noexcept -> ice::PhysicsID
    {
        b2BodyDef body_def{ };
        body_def.type = b2BodyType::b2_staticBody;
        body_def.position.Set(position.x, position.y);

        b2Body* body = _world->CreateBody(&body_def);
        body->GetUserData().entity = ice::Entity{ };

        b2PolygonShape tile_shape{ };
        ice::vec2f const center = dimensions / 2.f;
        tile_shape.SetAsBox(center.x, center.y, { center.x, center.y }, 0.f);
        body->CreateFixture(&tile_shape, 0.f);

        return detail::body_to_physics_id(body);
    }

    void IceWorldTrait_PhysicsBox2D::destroy_body(
        ice::PhysicsID physics_id
    ) noexcept
    {
        b2Body* body = detail::body_from_physics_id(physics_id);
        _world->DestroyBody(body);
    }

    void IceWorldTrait_PhysicsBox2D::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _engine = ice::addressof(engine);

        b2Vec2 gravity{ 0.f, -10.f };
        _world = portal.allocator().make<b2World>(gravity, static_cast<ice::Allocator*>(ice::addressof(portal.allocator())));

        b2BodyDef body_def{ };
        body_def.position.Set(0, -0.5f);

        b2Body* body = _world->CreateBody(&body_def);
        body->GetUserData().entity = ice::Entity{ };

        b2PolygonShape tile_shape;
        tile_shape.SetAsBox(50.f, 0.5, { 0.5f, 0.5f }, 0.f);

        body->CreateFixture(&tile_shape, 0.f);

        portal.storage().create_named_object<DynamicQuery>("ice.query.physics_bodies"_sid, portal.allocator(), portal.entity_storage().archetype_index());
        portal.storage().create_named_object<PhysicsQuery>("ice.query.physics_data"_sid, portal.allocator(), portal.entity_storage().archetype_index());

        _devui = portal.allocator().make<ice::DevUI_Box2D>(*_world);
        engine.developer_ui().register_widget(_devui);
    }

    void IceWorldTrait_PhysicsBox2D::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        engine.developer_ui().unregister_widget(_devui);
        portal.allocator().destroy(_devui);
        _devui = nullptr;

        _engine = nullptr;

        DynamicQuery& query = *portal.storage().named_object<DynamicQuery>("ice.query.physics_bodies"_sid);
        DynamicQuery::ResultByEntity result = query.result_by_entity(portal.allocator(), portal.entity_storage());

        result.for_each(
            [&](ice::Entity, ice::Transform2DDynamic const& dyn_xform, ice::PhysicsBody& phx_body, ice::Actor const*) noexcept
            {
                phx_body.trait_data = nullptr;
            }
        );

        portal.allocator().destroy(_world);
        _world = nullptr;

        portal.storage().destroy_named_object<DynamicQuery>("ice.query.physics_bodies"_sid);
        portal.storage().destroy_named_object<DynamicQuery>("ice.query.physics_data"_sid);
    }

    void IceWorldTrait_PhysicsBox2D::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        for (ice::Shard const& shard : ice::filter_span(frame.shards(), ice::any_of<Shard_EntityDestroyed>))
        {
            ice::Entity entity;
            if (ice::shard_inspect(shard, entity))
            {
                b2Body* body = _world->GetBodyList();
                while (body != nullptr)
                {
                    b2Body* next = body->GetNext();
                    auto const& userdata = body->GetUserData();

                    if (userdata.entity == entity)
                    {
                        _world->DestroyBody(body);
                    }

                    body = next;
                }
            }
        }

        PhysicsQuery& phx_query = *portal.storage().named_object<PhysicsQuery>("ice.query.physics_data"_sid);
        PhysicsQuery::ResultByEntity phx_result = phx_query.result_by_entity(frame.allocator(), portal.entity_storage());

        phx_result.for_each([](ice::Entity e, ice::PhysicsBody& phx_body, ice::PhysicsVelocity& vel) noexcept
            {
                if (phx_body.trait_data != nullptr)
                {
                    b2Body* body = reinterpret_cast<b2Body*>(phx_body.trait_data);
                    b2Vec2 velocity = body->GetLinearVelocity();

                    if (vel.velocity.y >= 0.01f || vel.velocity.y <= -0.01f)
                    {
                        velocity.y = vel.velocity.y;
                    }

                    if (vel.velocity.x >= 0.01f || vel.velocity.x <= -0.01f)
                    {
                        velocity.x = vel.velocity.x;
                    }

                    body->SetLinearVelocity(velocity);
                }
            }
        );

        _world->Step(1.f / 120.f, 6, 2);

        DynamicQuery& query = *portal.storage().named_object<DynamicQuery>("ice.query.physics_bodies"_sid);
        DynamicQuery::ResultByEntity result = query.result_by_entity(frame.allocator(), portal.entity_storage());

        result.for_each(
            [&](ice::Entity e, ice::Transform2DDynamic& dyn_xform, ice::PhysicsBody& phx_body, ice::Actor const* actor) noexcept
            {
                if (phx_body.trait_data == nullptr)
                {
                    b2BodyDef body_def{ };
                    body_def.type = b2_dynamicBody;
                    body_def.position.Set(dyn_xform.position.x / Constant_PixelsInMeter, dyn_xform.position.y / Constant_PixelsInMeter);
                    body_def.angularDamping = 1.f;

                    b2Body* body = _world->CreateBody(&body_def);
                    body->GetUserData().entity = e;
                    phx_body.trait_data = body;

                    b2PolygonShape tile_shape;
                    tile_shape.SetAsBox(0.25, 0.25, { 0.25f, 0.25f }, 0.f);
                    //ice::pod::hash::set(_entity_bodies, ice::hash(e), body);

                    b2FixtureDef fixture_def;
                    fixture_def.shape = &tile_shape;
                    fixture_def.density = 1.0f;
                    fixture_def.friction = 1.0f;

                    body->SetFixedRotation(true);
                    body->CreateFixture(&fixture_def);
                }
                else
                {
                    b2Body* body = reinterpret_cast<b2Body*>(phx_body.trait_data);
                    b2Vec2 body_pos = body->GetWorldPoint(body->GetLocalCenter());

                    body_pos.y -= 0.25;
                    body_pos.x -= 0.25;

                    dyn_xform.position = ice::vec3f{ body_pos.x * Constant_PixelsInMeter, body_pos.y * Constant_PixelsInMeter, dyn_xform.position.z };
                }
            }
        );

        _devui->on_frame(frame);
    }

    auto create_trait_physics(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait_Physics2D>
    {
        return ice::make_unique<ice::WorldTrait_Physics2D, ice::IceWorldTrait_PhysicsBox2D>(alloc);
    }

} // namespace ice
