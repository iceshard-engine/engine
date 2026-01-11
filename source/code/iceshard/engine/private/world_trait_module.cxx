/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/world/world_trait.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/module_register.hxx>
#include <ice/array.hxx>

namespace ice
{

    void load_trait_descriptions(
        ice::Allocator&,
        ice::ModuleRegister const& registry,
        ice::TraitArchive& asset_category_archive
    ) noexcept
    {
        using ice::detail::world_traits::TraitsModuleAPI;

        ice::StackAllocator<ice::size_of<TraitsModuleAPI> * 20> static_alloc{};
        ice::Array<TraitsModuleAPI> api_ptrs{ static_alloc };
        if (registry.query_apis(api_ptrs))
        {
            for (TraitsModuleAPI const& api : api_ptrs)
            {
                api.register_traits_fn(asset_category_archive);
            }
        }
    }

} // namespace ice
