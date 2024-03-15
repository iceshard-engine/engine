/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "trait_sprite_animator.hxx"
#include <ice/game_anim.hxx>
#include <ice/game_entity.hxx>

#include <ice/engine_frame.hxx>
#include <ice/ecs/ecs_entity_storage.hxx>

#include <ice/render/render_image.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>

#include <ice/string/static_string.hxx>
#include <ice/data_storage.hxx>
#include <ice/resource_meta.hxx>
#include <ice/task_utils.hxx>
#include <ice/asset_storage.hxx>
#include <ice/asset.hxx>

namespace ice
{

#if 0
    namespace detail
    {

        struct ReleaseOnExit
        {
            ice::Asset const& asset;
            ice::AssetStorage& storage;

            inline ~ReleaseOnExit() noexcept
            {
                ice::wait_for(storage.release(asset));
            }
        };

    } // namespace detail

    IceWorldTrait_SpriteAnimator::IceWorldTrait_SpriteAnimator(
        ice::Allocator& alloc
    ) noexcept
        : _anim_infos{ alloc }
    {
        ice::hashmap::reserve(_anim_infos, 100);
    }

    IceWorldTrait_SpriteAnimator::~IceWorldTrait_SpriteAnimator() noexcept
    {
    }

    void IceWorldTrait_SpriteAnimator::load_animations(
        ice::StringID_Arg animation_sprite
    ) noexcept
    {
    }

    void IceWorldTrait_SpriteAnimator::on_activate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        _assets = ice::addressof(engine.asset_storage());

        portal.storage().create_named_object<SpriteQuery::Query>("ice.query.animator_sprites"_sid, portal.entity_storage().create_query(portal.allocator(), SpriteQuery{}));
        portal.storage().create_named_object<AnimQuery::Query>("ice.query.animator"_sid, portal.entity_storage().create_query(portal.allocator(), AnimQuery{}));
    }

    void IceWorldTrait_SpriteAnimator::on_deactivate(
        ice::Engine& engine,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        portal.storage().destroy_named_object<SpriteQuery::Query>("ice.query.animator_sprites"_sid);
        portal.storage().destroy_named_object<AnimQuery::Query>("ice.query.animator"_sid);
    }

    void IceWorldTrait_SpriteAnimator::on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        SpriteQuery::Query const& sprite_query = *portal.storage().named_object<SpriteQuery::Query>("ice.query.animator_sprites"_sid);

        ice::ecs::query::for_each_entity(
            sprite_query,
            [&](ice::Animation const& anim, ice::Sprite const& sprite) noexcept
            {
                ice::Asset sprite_asset = _assets->bind(ice::render::AssetType_Texture2D, sprite.material);
                ice::wait_for(sprite_asset[AssetState::Baked]);
                if (!sprite_asset.available(AssetState::Baked))
                {
                    return;
                }

                ice::Metadata const& asset_meta = sprite_asset.metadata();
                ice::detail::ReleaseOnExit release_asset{ sprite_asset, *_assets };

                ice::Array<ice::String> anim_names{ portal.allocator() };
                if (meta_read_string_array(asset_meta, "animation.names"_sid, anim_names) == false)
                {
                    return;
                }

                for (ice::String const& name : anim_names)
                {
                    ice::StringID nameid = ice::stringid(name);

                    if (ice::hashmap::has(_anim_infos, ice::hash(nameid)) == true)
                    {
                        return;
                    }

                    ice::StaticString<64> meta_key{ "animation." };
                    ice::string::push_back(meta_key, name);
                    ice::string::push_back(meta_key, '.');
                    ice::u32 const base_key_size = ice::string::size(meta_key);

                    ice::i32 frame_count;
                    ice::i32 frame_step = 1;
                    ice::i32 frame_initial[2];

                    // Frame count
                    bool success = true;
                    ice::string::push_back(meta_key, "frame_count");
                    success &= ice::meta_read_int32(asset_meta, ice::stringid(meta_key), frame_count);

                    ice::string::resize(meta_key, base_key_size);
                    ice::string::push_back(meta_key, "frame_step");
                    ice::meta_read_int32(asset_meta, ice::stringid(meta_key), frame_step);

                    ice::string::resize(meta_key, base_key_size);
                    ice::string::push_back(meta_key, "frame_initial_x");
                    success &= ice::meta_read_int32(asset_meta, ice::stringid(meta_key), frame_initial[0]);

                    ice::string::resize(meta_key, base_key_size);
                    ice::string::push_back(meta_key, "frame_initial_y");
                    success &= ice::meta_read_int32(asset_meta, ice::stringid(meta_key), frame_initial[1]);

                    if (success == false)
                    {
                        return;
                    }

                    ice::TraitAnimatorAnimationInfo anim_info{
                        .name = ice::stringid_hash(nameid),
                        .initial_sprite_pos = { (ice::u32)frame_initial[0], (ice::u32)frame_initial[1] },
                        .frame_count = (ice::u32)frame_count,
                        .frame_step = frame_step
                    };

                    ice::hashmap::set(_anim_infos, ice::hash(anim_info.name), anim_info);
                }
            }
        );

        AnimQuery::Query& anim_query = *portal.storage().named_object<AnimQuery::Query>("ice.query.animator"_sid);

        ice::ecs::query::for_each_entity(
            anim_query,
            [&](ice::Animation const& anim, ice::AnimationState& state, ice::SpriteTile& sprite_tile) noexcept
            {
                Timer anim_timer = ice::timer::create_timer(runner.clock(), anim.speed, state.timestamp);
                if (ice::timer::update(anim_timer))
                {
                    static ice::TraitAnimatorAnimationInfo null_info{ .name = ice::StringID_Invalid };
                    ice::TraitAnimatorAnimationInfo const& anim_info = ice::hashmap::get(_anim_infos, ice::hash(anim.animation), null_info);

                    if (anim_info.name == ice::StringID_Invalid)
                    {
                        return;
                    }

                    state.timestamp = anim_timer.last_tick_timestamp;

                    if (anim.animation != state.current_animation)
                    {
                        state.timestamp = runner.clock().latest_timestamp;
                        state.current_animation = anim.animation;
                        state.frame = { 0, 0 };
                        sprite_tile.material_tile = anim_info.initial_sprite_pos;
                    }

                    state.frame.x += 1;
                    if (state.frame.x == anim_info.frame_count)
                    {
                        state.frame.x = 0;
                        sprite_tile.material_tile = anim_info.initial_sprite_pos;
                    }
                    else
                    {
                        sprite_tile.material_tile.x += anim_info.frame_step;
                    }
                }
            }
        );
    }

    auto create_trait_animator(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTrait>
    {
        return ice::make_unique<ice::WorldTrait, ice::IceWorldTrait_SpriteAnimator>(alloc, alloc);
    }
#endif

} // namespace ice
