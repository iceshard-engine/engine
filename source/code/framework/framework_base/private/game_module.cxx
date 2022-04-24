#include <ice/game_module.hxx>
#include <ice/world/world_trait_module.hxx>

#include "traits/trait_tilemap.hxx"
#include "traits/trait_camera.hxx"
#include "traits/trait_sprite_animator.hxx"
#include "traits/trait_player_actor.hxx"
#include "traits/physics/trait_box2d.hxx"

namespace ice
{

    static constexpr ice::StringID_Hash Constant_WorldTraitsAPI = ice::stringid_hash(ice::Constant_APIName_WorldTraitsModule);

    static constexpr ice::StringID Constant_TraitName_PlayerActor = "ice.base-framework.trait-player-actor"_sid;
    static constexpr ice::StringID Constant_TraitName_PhysicsBox2D = "ice.base-framework.trait-physics-box2d"_sid;
    static constexpr ice::StringID Constant_TraitName_RenderCamera = "ice.base-framework.trait-camera"_sid;
    static constexpr ice::StringID Constant_TraitName_SpriteAnimator = "ice.base-framework.trait-sprite-animator"_sid;
    static constexpr ice::StringID Constant_TraitName_TileMap = "ice.base-framework.trait-tilemap"_sid;

    template<typename TraitType>
    void game_register_trait(ice::WorldTraitArchive& archive, ice::StringID_Arg name) noexcept
    {
        archive.register_trait(
            name,
            ice::WorldTraitDescription{ .factory = &ice::detail::generic_trait_factory<TraitType> }
        );
    }

    void game_register_trait(ice::WorldTraitArchive& archive, ice::StringID_Arg name, ice::WorldTraitFactory* factory) noexcept
    {
        archive.register_trait(
            name,
            ice::WorldTraitDescription{ .factory = factory }
        );
    }

    bool game_register_traits(ice::WorldTraitArchive& archive) noexcept
    {
        game_register_trait<IceWorldTrait_PlayerActor>(archive, ice::Constant_TraitName_PlayerActor);
        game_register_trait<IceWorldTrait_PhysicsBox2D>(archive, ice::Constant_TraitName_PhysicsBox2D);
        game_register_trait<IceWorldTrait_RenderCamera>(archive, ice::Constant_TraitName_RenderCamera);
        game_register_trait<IceWorldTrait_SpriteAnimator>(archive, ice::Constant_TraitName_SpriteAnimator);
        game_register_trait(archive, ice::Constant_TraitName_TileMap, ice::trait_factory_tilemap);
        return true;
    }

    bool get_module_api(ice::StringID_Hash api, ice::u32 ver, void** ptr) noexcept
    {
        if (api == Constant_WorldTraitsAPI && ver == 1)
        {
            static ice::detail::world_traits::v1::TraitsModuleAPI module_api{
                .register_traits_fn = game_register_traits
            };

            *ptr = &module_api;
            return true;
        }

        return false;
    }

    void load_game_module(
        ice::Allocator* alloc,
        ice::ModuleNegotiatorContext* ctx,
        ice::ModuleNegotiator* api
    ) noexcept
    {
        api->fn_register_module(ctx, Constant_WorldTraitsAPI, &get_module_api);
    }

    void unload_game_module(
        ice::Allocator* alloc
    ) noexcept
    {

    }

} // namespace ice
