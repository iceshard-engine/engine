/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/world/world_trait.hxx>
#include <ice/world/world_trait_module.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/module_register.hxx>
#include <ice/container/array.hxx>

namespace ice
{

    void load_trait_descriptions(
        ice::Allocator&,
        ice::ModuleRegister const& registry,
        ice::TraitArchive& asset_type_archive
    ) noexcept
    {
        using ice::detail::world_traits::TraitsModuleAPI;

        ice::StackAllocator<ice::size_of<void*> * 20> static_alloc{};
        ice::Array<void*> api_ptrs{ static_alloc };
        ice::array::reserve(api_ptrs, 20);

        if (registry.find_module_apis(Constant_APIName_WorldTraitsModule, 2, api_ptrs))
        {
            for (void* api_ptr : api_ptrs)
            {
                TraitsModuleAPI* module_api = reinterpret_cast<TraitsModuleAPI*>(api_ptr);
                module_api->register_traits_fn(asset_type_archive);
            }
        }
    }

} // namespace ice
