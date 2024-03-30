/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_gfx_image_storage_trait.hxx"
#include "iceshard_gfx_shader_storage_trait.hxx"
#include <ice/world/world_trait_module.hxx>
#include <ice/module.hxx>

namespace ice::gfx
{

    template<typename TraitType>
    static auto gfx_trait_factory(ice::Allocator& alloc, void*) noexcept -> UniquePtr<ice::Trait>
    {
        return ice::make_unique<TraitType>(alloc, alloc);
    }

    struct IceshardModule_GfxTraits : ice::Module<IceshardModule_GfxTraits>
    {
        static bool v1_register_traits(ice::TraitArchive& archive) noexcept
        {
            archive.register_trait({ .name = ice::TraitID_GfxShaderStorage, .fn_factory = gfx_trait_factory<Trait_GfxShaderStorage> });
            archive.register_trait({ .name = ice::TraitID_GfxImageStorage, .fn_factory = gfx_trait_factory<Trait_GfxImageStorage> });
            return true;
        }

        static void v1_api(ice::detail::world_traits::TraitsModuleAPI& api) noexcept
        {
            api.register_traits_fn = v1_register_traits;
        }

        static bool on_load(ice::Allocator&, ice::ModuleNegotiator auto const& negotiator) noexcept
        {
            return negotiator.register_api(v1_api);
        }

        IS_WORKAROUND_MODULE_INITIALIZATION(IceshardModule_GfxTraits);
    };

} // namespace ice::gfx
