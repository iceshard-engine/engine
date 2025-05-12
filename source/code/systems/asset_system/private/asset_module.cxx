/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset_module.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/container/array.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void load_asset_category_definitions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetCategoryArchive& asset_category_archive
    ) noexcept
    {
        ice::StackAllocator<ice::size_of<ice::detail::asset_system::v1::AssetArchiveAPI> * 10> static_alloc{};
        ice::Array<ice::detail::asset_system::v1::AssetArchiveAPI> api_ptrs{ static_alloc };
        registry.query_apis(api_ptrs);

        for (ice::detail::asset_system::v1::AssetArchiveAPI const& api : api_ptrs)
        {
            api.fn_register_categories(asset_category_archive, registry);
        }
    }

} // namespace ice
