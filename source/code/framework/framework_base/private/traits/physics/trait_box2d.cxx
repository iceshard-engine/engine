#include "trait_box2d.hxx"
#include <ice/game_anim.hxx>

#include <ice/engine_frame.hxx>
#include <ice/engine_runner.hxx>
#include <ice/world/world_portal.hxx>

#include <ice/input/input_device.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>

#include <ice/data_storage.hxx>
#include <ice/clock.hxx>

namespace ice
{

    void IceWorldTrait_PhysicsBox2D::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        b2Vec2 gravity{ 0.f, -10.f };
        _world = portal.allocator().make<b2World>(gravity, static_cast<ice::Allocator*>(ice::addressof(portal.allocator())));



        b2BodyDef body_def{ };
        body_def.position.Set(0, -0.5f);

        b2Body* body = _world->CreateBody(&body_def);

        b2PolygonShape tile_shape;
        tile_shape.SetAsBox(50.f, 0.5, { 0.5f, 0.5f }, 0.f);

        body->CreateFixture(&tile_shape, 0.f);

        portal.storage().create_named_object<DynamicQuery>("ice.query.physics_bodies"_sid, portal.allocator(), portal.entity_storage().archetype_index());
    }

    void IceWorldTrait_PhysicsBox2D::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        DynamicQuery& query = *portal.storage().named_object<DynamicQuery>("ice.query.physics_bodies"_sid);
        DynamicQuery::ResultByEntity result = query.result_by_entity(portal.allocator(), portal.entity_storage());

        result.for_each(
            [&](ice::Transform2DDynamic const& dyn_xform, ice::PhysicsBody& phx_body, ice::Actor const*) noexcept
            {
                phx_body.trait_data = nullptr;
            }
        );

        portal.allocator().destroy(_world);
        _world = nullptr;

        portal.storage().destroy_named_object<DynamicQuery>("ice.query.physics_bodies"_sid);
    }

    void IceWorldTrait_PhysicsBox2D::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        //if (ice::pod::hash::has(_entity_bodies, ice::hash(e)) == false)
        //{
        //    b2BodyDef body_def{ };
        //    body_def.type = b2_dynamicBody;
        //    body_def.position.Set(xform.position.x / Constant_TileSize, xform.position.y / Constant_TileSize);

        //    b2Body* body = _world.CreateBody(&body_def);
        //    ice::pod::hash::set(_entity_bodies, ice::hash(e), body);

        //    b2PolygonShape tile_shape;
        //    tile_shape.SetAsBox(0.25, 0.25, { 0.25f, 0.25f }, 0.f);

        //    b2FixtureDef fixture_def;
        //    fixture_def.shape = &tile_shape;
        //    fixture_def.density = 1.0f;
        //    fixture_def.friction = 0.3f;

        //    body->CreateFixture(&fixture_def);
        //}
        //else
        //{
        //    b2Body* body = ice::pod::hash::get(_entity_bodies, ice::hash(e), nullptr);
        //    b2Vec2 body_pos = body->GetPosition();
        //    xform.position = ice::vec3f{ body_pos.x * Constant_TileSize, body_pos.y * Constant_TileSize, 1.f };
        //}
        _world->Step(1.f / 120.f, 6, 2);

        DynamicQuery& query = *portal.storage().named_object<DynamicQuery>("ice.query.physics_bodies"_sid);
        DynamicQuery::ResultByEntity result = query.result_by_entity(frame.allocator(), portal.entity_storage());

        result.for_each(
            [&](ice::Transform2DDynamic& dyn_xform, ice::PhysicsBody& phx_body, ice::Actor const* actor) noexcept
            {
                if (phx_body.trait_data == nullptr)
                {
                    b2BodyDef body_def{ };
                    body_def.type = b2_dynamicBody;
                    body_def.position.Set(dyn_xform.position.x / Constant_PixelsInMeter, dyn_xform.position.y / Constant_PixelsInMeter);

                    b2Body* body = _world->CreateBody(&body_def);
                    phx_body.trait_data = body;

                    b2PolygonShape tile_shape;
                    tile_shape.SetAsBox(0.25, 0.25, { 0.25f, 0.25f }, 0.f);
                    //ice::pod::hash::set(_entity_bodies, ice::hash(e), body);

                    b2FixtureDef fixture_def;
                    fixture_def.shape = &tile_shape;
                    fixture_def.density = 1.0f;
                    fixture_def.friction = 0.0f;

                    body->CreateFixture(&fixture_def);
                }
                else
                {
                    b2Body* body = reinterpret_cast<b2Body*>(phx_body.trait_data);
                    b2Vec2 body_pos = body->GetPosition();

                    //if (actor != nullptr)
                    //{
                    //    b2Vec2 vec{ dyn_xform.position.x / Constant_PixelsInMeter, dyn_xform.position.y / Constant_PixelsInMeter };
                    //    body->SetTransform(vec, body->GetAngle());
                    //}

                    dyn_xform.position = ice::vec3f{ body_pos.x * Constant_PixelsInMeter, body_pos.y * Constant_PixelsInMeter, 1.f };
                }
            }
        );
    }

    auto create_trait_physics(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>
    {
        return ice::make_unique<ice::WorldTrait, ice::IceWorldTrait_PhysicsBox2D>(alloc);
    }

} // namespace ice
