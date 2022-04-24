#include <ice/world/world_trait_module.hxx>
#include <ice/module_register.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    void load_trait_descriptions(
        ice::Allocator&,
        ice::ModuleRegister const& registry,
        ice::WorldTraitArchive& asset_type_archive
    ) noexcept
    {
        using ice::detail::world_traits::v1::TraitsModuleAPI;

        ice::pod::Array<void*> api_ptrs{ ice::memory::default_scratch_allocator() };
        ice::pod::array::reserve(api_ptrs, 10);

        if (registry.find_module_apis(Constant_APIName_WorldTraitsModule, 1, api_ptrs))
        {
            for (void* api_ptr : api_ptrs)
            {
                TraitsModuleAPI* module_api = reinterpret_cast<TraitsModuleAPI*>(api_ptr);
                module_api->register_traits_fn(asset_type_archive);
            }
        }
    }

} // namespace ice
