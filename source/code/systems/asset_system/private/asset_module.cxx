#include <ice/asset_module.hxx>
#include <ice/pod/array.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void load_asset_type_definitions(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetTypeArchive& asset_type_archive
    ) noexcept
    {
        using ice::detail::asset_system::v1::AssetTypeArchiveAPI;
        using ice::detail::asset_system::v1::Constant_APIName_AssetTypeArchive;

        ice::pod::Array<void*> api_ptrs{ ice::memory::default_scratch_allocator() };
        ice::pod::array::reserve(api_ptrs, 10);

        if (registry.find_module_apis(Constant_APIName_AssetTypeArchive, 1, api_ptrs))
        {
            for (void* api_ptr : api_ptrs)
            {
                AssetTypeArchiveAPI* module_api = reinterpret_cast<AssetTypeArchiveAPI*>(api_ptr);
                module_api->register_types_fn(asset_type_archive);
            }
        }
    }

} // namespace ice
