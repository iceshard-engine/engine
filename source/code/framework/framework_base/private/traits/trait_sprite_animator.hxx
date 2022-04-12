#pragma once
#include <ice/pod/hash.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/game_anim.hxx>
#include <ice/game_sprites.hxx>
#include <ice/ecs/ecs_query.hxx>

#include <ice/render/render_resource.hxx>
#include <ice/render/render_image.hxx>

namespace ice
{

    class AssetStorage;

    struct TraitAnimatorAnimationInfo
    {
        ice::StringID_Hash name;
        ice::vec2u initial_sprite_pos;
        ice::u32 frame_count;
        ice::i32 frame_step = 1;
    };

    class IceWorldTrait_SpriteAnimator : public ice::WorldTrait
    {
    public:
        IceWorldTrait_SpriteAnimator(
            ice::Allocator& alloc
        ) noexcept;

        ~IceWorldTrait_SpriteAnimator() noexcept;

        void load_animations(
            ice::StringID_Arg animation_sprite
        ) noexcept;

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
        using SpriteQuery = ice::ecs::QueryDefinition<ice::Animation const&, ice::Sprite const&>;
        using AnimQuery = ice::ecs::QueryDefinition<ice::Animation const&, ice::AnimationState&, ice::SpriteTile&>;

        ice::AssetStorage* _assets;
        ice::pod::Hash<ice::TraitAnimatorAnimationInfo> _anim_infos;
    };

} // namespace ice
