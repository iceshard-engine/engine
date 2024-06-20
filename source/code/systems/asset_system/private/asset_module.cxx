/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset_module.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/container/array.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void load_asset_type_definitions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetTypeArchive& asset_type_archive
    ) noexcept
    {
        ice::StackAllocator<ice::size_of<ice::detail::asset_system::v1::AssetTypeArchiveAPI> * 10> static_alloc{};
        ice::Array<ice::detail::asset_system::v1::AssetTypeArchiveAPI> api_ptrs{ static_alloc };
        registry.query_apis(api_ptrs);

        for (ice::detail::asset_system::v1::AssetTypeArchiveAPI const& api : api_ptrs)
        {
            api.register_types_fn(asset_type_archive, registry);
        }
    }

} // namespace ice
