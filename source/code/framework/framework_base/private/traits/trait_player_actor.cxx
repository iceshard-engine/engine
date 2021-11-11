#include "trait_player_actor.hxx"
#include <ice/game_anim.hxx>

#include <ice/engine_frame.hxx>
#include <ice/world/world_portal.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

#include <ice/input/input_device.hxx>
#include <ice/input/input_event.hxx>
#include <ice/input/input_keyboard.hxx>

#include <ice/data_storage.hxx>

namespace ice
{

    void IceWorldTrait_PlayerActor::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().create_named_object<Query::Query>("ice.query.player_actors"_sid, portal.entity_storage().create_query(portal.allocator(), Query{}));
    }

    void IceWorldTrait_PlayerActor::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().destroy_named_object<Query::Query>("ice.query.player_actors"_sid);
    }

    void IceWorldTrait_PlayerActor::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        using ice::input::DeviceType;
        using ice::input::KeyboardKey;
        using ice::input::input_identifier;

        Query::Query& query = *portal.storage().named_object<Query::Query>("ice.query.player_actors"_sid);

        ice::vec2f movement;
        for (ice::input::InputEvent const& input : frame.input_events())
        {
            switch (input.identifier)
            {
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyW):
                movement.y += 2.f;
                break;
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyS):
                movement.y -= 2.f;
                break;
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyA):
                movement.x -= 2.f;
                break;
            case input_identifier(DeviceType::Keyboard, KeyboardKey::KeyD):
                movement.x += 2.f;
                break;
            default:
                break;
            }
        }

        ice::ecs::query::for_each_entity(
            query,
            [&](ice::Actor const& actor, ice::PhysicsVelocity& vel, ice::Animation& anim, ice::Transform2DDynamic& xform) noexcept
            {
                if (actor.type == ActorType::Player)
                {
                    vel.velocity = movement;
                }

                if (movement.x != 0.f)
                {
                    anim.speed = 1.f / 15.f;
                    if (movement.x > 0)
                    {
                        anim.animation = "cotm_run_right"_sid_hash;
                    }
                    else
                    {
                        anim.animation = "cotm_run_left"_sid_hash;
                    }
                }
                else
                {
                    anim.animation = "cotm_idle"_sid_hash;
                    anim.speed = 1.f / 15.f;
                }
            }
        );
    }

    auto create_trait_actor(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>
    {
        return ice::make_unique<ice::WorldTrait, ice::IceWorldTrait_PlayerActor>(alloc);
    }

} // namespace ice
