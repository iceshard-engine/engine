/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait_archive.hxx>

namespace ice
{

    class ModuleRegister;

    void load_trait_descriptions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::TraitArchive& trait_archive
    ) noexcept;

    namespace detail::world_traits
    {

        static constexpr ice::StringID Constant_APIName_WorldTraitsModule = "ice.world-traits-module"_sid;

        using RegisterTraitsFn_v2 = bool(
            ice::TraitArchive&
        ) noexcept;

        struct TraitsModuleAPI
        {
            RegisterTraitsFn_v2* register_traits_fn;
        };

    } // namespace detail::engine::v1

    using ice::detail::world_traits::Constant_APIName_WorldTraitsModule;

} // namespace ice
