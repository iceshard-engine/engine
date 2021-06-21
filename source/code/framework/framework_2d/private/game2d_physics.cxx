#include <ice/game2d_physics.hxx>
#include <ice/game2d_object.hxx>
#include <ice/game2d_trait.hxx>

#include <ice/engine_frame.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/entity/entity.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_query.hxx>

#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>

#include <ice/clock.hxx>

#include "box2d.hxx"

namespace ice
{

    namespace detail
    {

        using TileMapQuery = ice::ComponentQuery<ice::TileMapComponent const&>;
        using ObjectQuery = ice::ComponentQuery<ice::Entity, ice::Obj2dShape const&, ice::Obj2dTransform&>;

        class IceRoomPhysicsWorld
        {
        public:
            IceRoomPhysicsWorld(
                ice::Allocator& alloc,
                ice::TileRoom const& room,
                ice::Span<ice::TileMaterial const> rigid_materials
            ) noexcept
                : _allocator{ alloc }
                , _gravity{ 0.f, -10.f }
                , _world{ _gravity, &_allocator }
                , _entity_bodies{ _allocator }
            {
                for (ice::u32 idx = 0; idx < room.tiles_count; ++idx)
                {
                    ice::vec2f position = room.tiles_position[idx] / Constant_TileSize;
                    ice::TileMaterial material = room.tiles_material[idx];

                    bool is_rigid_material = false;
                    for (ice::TileMaterial const& mat : rigid_materials)
                    {
                        is_rigid_material |= mat.material_id == material.material_id;
                    }

                    if (is_rigid_material)
                    {
                        b2BodyDef body_def{ };
                        body_def.position.Set(position.x, position.y);

                        b2Body* body = _world.CreateBody(&body_def);

                        b2PolygonShape tile_shape;
                        tile_shape.SetAsBox(0.5, 0.5, { 0.5f, 0.5f }, 0.f);

                        body->CreateFixture(&tile_shape, 0.f);
                    }
                    //else
                    //{
                    //    b2BodyDef body_def{ };
                    //    body_def.type = b2_dynamicBody;
                    //    body_def.position.Set(position.x, position.y);

                    //    b2Body* body = _world.CreateBody(&body_def);
                    //    ice::pod::array::push_back(_tile_bodies, body);

                    //    b2PolygonShape tile_shape;
                    //    tile_shape.SetAsBox(0.5, 0.5, { 0.5f, 0.5f }, 0.f);

                    //    b2FixtureDef fixture_def;
                    //    fixture_def.shape = &tile_shape;
                    //    fixture_def.density = 1.0f;
                    //    fixture_def.friction = 0.3f;

                    //    body->CreateFixture(&fixture_def);
                    //}
                }
            }

            ~IceRoomPhysicsWorld() noexcept
            {
            }

            void update(ice::f32 timestep, ObjectQuery::ResultByEntity& query_result) noexcept
            {
                _world.Step(timestep, 6, 2);

                query_result.for_each(
                    [this](ice::Entity e, ice::Obj2dShape const& shape, ice::Obj2dTransform& xform) noexcept
                    {
                        if (ice::pod::hash::has(_entity_bodies, ice::hash(e)) == false)
                        {
                            b2BodyDef body_def{ };
                            body_def.type = b2_dynamicBody;
                            body_def.position.Set(xform.position.x / Constant_TileSize, xform.position.y / Constant_TileSize);

                            b2Body* body = _world.CreateBody(&body_def);
                            ice::pod::hash::set(_entity_bodies, ice::hash(e), body);

                            b2PolygonShape tile_shape;
                            tile_shape.SetAsBox(0.25, 0.25, { 0.25f, 0.25f }, 0.f);

                            b2FixtureDef fixture_def;
                            fixture_def.shape = &tile_shape;
                            fixture_def.density = 1.0f;
                            fixture_def.friction = 0.3f;

                            body->CreateFixture(&fixture_def);
                        }
                        else
                        {
                            b2Body* body = ice::pod::hash::get(_entity_bodies, ice::hash(e), nullptr);
                            b2Vec2 body_pos = body->GetPosition();
                            xform.position = ice::vec3f{ body_pos.x * Constant_TileSize, body_pos.y * Constant_TileSize, 1.f };
                        }
                    }
                );
            }

        private:
            ice::Allocator& _allocator;
            //ice::pod::Array<b2Body*> _tile_bodies;
            ice::pod::Hash<b2Body*> _entity_bodies;

            b2Vec2 const _gravity;
            b2World _world;
        };

    } // namespace detail

    class IcePhysics2DTrait : public ice::Physics2DTrait
    {
    public:
        IcePhysics2DTrait(
            ice::Allocator& alloc,
            ice::Clock const& clock
        ) noexcept;
        ~IcePhysics2DTrait() noexcept;

        void load_tilemap_room(
            ice::TileRoom const& room,
            ice::Span<ice::TileMaterial const> rigid_materials
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
        ice::Allocator& _allocator;
        ice::detail::IceRoomPhysicsWorld* _current_physics_world;
        ice::Timer _timer;
    };

    IcePhysics2DTrait::IcePhysics2DTrait(
        ice::Allocator& alloc,
        ice::Clock const& clock
    ) noexcept
        : _allocator{ alloc }
        , _current_physics_world{ nullptr }
        , _timer{ ice::timer::create_timer(clock, 1.f / 60.f) }
    {
    }

    IcePhysics2DTrait::~IcePhysics2DTrait() noexcept
    {
        _allocator.destroy(_current_physics_world);
    }

    void IcePhysics2DTrait::load_tilemap_room(
        ice::TileRoom const& room,
        ice::Span<ice::TileMaterial const> rigid_materials
    ) noexcept
    {
        _allocator.destroy(_current_physics_world);
        _current_physics_world = _allocator.make<ice::detail::IceRoomPhysicsWorld>(_allocator, room, rigid_materials);
    }

    void IcePhysics2DTrait::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().create_named_object<detail::TileMapQuery>(
            "ice.tilemap.physics.query"_sid,
            _allocator,
            portal.entity_storage().archetype_index()
        );
        portal.storage().create_named_object<detail::ObjectQuery>(
            "ice.entities.physics.query"_sid,
            _allocator,
            portal.entity_storage().archetype_index()
        );
    }

    void IcePhysics2DTrait::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().destroy_named_object<detail::TileMapQuery>(
            "ice.tilemap.physics.query"_sid
        );
        portal.storage().destroy_named_object<detail::ObjectQuery>(
            "ice.entities.physics.query"_sid
        );
    }

    void IcePhysics2DTrait::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        detail::TileMapQuery const* const query = portal.storage().named_object<detail::TileMapQuery>("ice.tilemap.physics.query"_sid);
        detail::TileMapQuery::ResultByEntity query_result = query->result_by_entity(_allocator, portal.entity_storage());

        detail::ObjectQuery const* const obj_query = portal.storage().named_object<detail::ObjectQuery>("ice.entities.physics.query"_sid);

        bool step = false;
        for (ice::input::InputEvent const& event : frame.input_events())
        {
            switch (event.identifier)
            {
            case ice::input::input_identifier(ice::input::DeviceType::Keyboard, ice::input::KeyboardKey::KeyT, ice::input::key_identifier_base_value):
                step = true;
            }
        }

        if (step && _current_physics_world != nullptr && ice::timer::update_by_step(_timer))
        {
            detail::ObjectQuery::ResultByEntity obj_query_result = obj_query->result_by_entity(_allocator, portal.entity_storage());
            _current_physics_world->update(1.f / 60.f, obj_query_result);
        }
    }

    auto create_physics2d_trait(
        ice::Allocator& alloc,
        ice::Clock const& clock
    ) noexcept -> ice::UniquePtr<ice::Physics2DTrait>
    {
        return ice::make_unique<ice::Physics2DTrait, ice::IcePhysics2DTrait>(alloc, alloc, clock);
    }

} // namespace ice
