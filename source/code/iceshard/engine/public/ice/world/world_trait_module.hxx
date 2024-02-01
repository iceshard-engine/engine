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

        using RegisterTraitsFn = bool(
            ice::TraitArchive&
        ) noexcept;

        struct TraitsModuleAPI
        {
            static constexpr ice::StringID Constant_APIName = "ice.world-traits-module"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            RegisterTraitsFn* register_traits_fn;
        };

    } // namespace detail::engine::v1

} // namespace ice
