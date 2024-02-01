/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/game_module.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/framework_module.hxx>

#include "traits/trait_tilemap.hxx"
#include "traits/trait_camera.hxx"
#include "traits/trait_sprite_animator.hxx"
#include "traits/trait_player_actor.hxx"
#include "traits/physics/trait_chipmunk2d.hxx"

#include "traits/ui/game_ui_trait.hxx"
#include "traits/ui/render_ui_trait.hxx"

#include "traits/render/trait_render_gfx.hxx"
#include "traits/render/trait_render_postprocess.hxx"

#include "traits/render/trait_render_sprites.hxx"
#include "traits/render/trait_render_texture_loader.hxx"
#include "traits/render/trait_render_debug.hxx"

#include "traits/render/trait_render_tilemap.hxx"
#include "traits/render/trait_render_glyphs.hxx"

namespace ice::framework
{

#if 0
    bool game_register_traits(ice::WorldTraitArchive& archive) noexcept
    {
        register_trait_render_gfx(archive);
        register_trait_render_clear(archive);
        register_trait_render_postprocess(archive);
        register_trait_render_finish(archive);
        register_trait_render_sprites(archive);
        register_trait_render_texture_loader(archive);
        register_trait_render_ui(archive);
        register_trait_render_glyphs(archive);
        register_trait_render_debug(archive);
        register_trait_render_tilemap(archive);

        register_trait_default<IceWorldTrait_PlayerActor>(archive, ice::Constant_TraitName_Actor);
        register_trait_default<IceWorldTrait_PhysicsBox2D>(archive, ice::Constant_TraitName_PhysicsBox2D);
        register_trait_default<IceWorldTrait_RenderCamera>(archive, ice::Constant_TraitName_RenderCamera);
        register_trait_default<IceWorldTrait_SpriteAnimator>(archive, ice::Constant_TraitName_SpriteAnimator);
        register_trait_gameui(archive);
        register_trait_tilemap(archive);
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
#endif

    bool FrameworkModule::on_load(
        ice::Allocator& alloc,
        ice::ModuleNegotiator const& negotiator
    ) noexcept
    {
        return true;
    }

    void FrameworkModule::on_unload(
        ice::Allocator& alloc
    ) noexcept
    {
    }

} // namespace ice::framework
