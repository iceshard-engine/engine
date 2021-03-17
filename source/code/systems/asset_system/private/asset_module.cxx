#include <ice/asset_module.hxx>
#include <ice/asset_system.hxx>
#include <ice/pod/array.hxx>
#include <ice/memory/memory_globals.hxx>
#include <ice/module_register.hxx>

namespace ice
{

    void load_asset_pipeline_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetSystem& asset_system
    ) noexcept
    {
        using ice::detail::asset_system::v1::AssetModuleAPI;

        ice::pod::Array<void*> api_ptrs{ ice::memory::default_scratch_allocator() };
        ice::pod::array::reserve(api_ptrs, 10);

        if (registry.find_module_apis("ice.asset_module"_sid, 1, api_ptrs))
        {
            for (void* api_ptr : api_ptrs)
            {
                AssetModuleAPI* module_api = reinterpret_cast<AssetModuleAPI*>(api_ptr);
                AssetPipeline* pipeline = module_api->create_pipeline_fn(alloc);

                if (pipeline != nullptr)
                {
                    asset_system.add_pipeline(
                        module_api->name_fn(),
                        ice::UniquePtr<ice::AssetPipeline>{ pipeline, { alloc, module_api->destroy_pipeline_fn } }
                    );
                }
            }
        }
    }

} // namespace ice
