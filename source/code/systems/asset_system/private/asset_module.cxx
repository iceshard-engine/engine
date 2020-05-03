#include <asset_system/asset_module.hxx>
#include <core/platform/windows.hxx>

#include <filesystem>

namespace iceshard::detail
{

    using AssetModuleCreateFunc = void*(core::allocator&, asset::AssetSystem&);
    using AssetModuleDestroyFunc = void(core::allocator&, void*);

    class SimpleAssetModule final : public iceshard::AssetModule
    {
    public:
        SimpleAssetModule(
            core::allocator& alloc,
            HMODULE native_handle,
            void* module_object,
            AssetModuleDestroyFunc* destroy_module_func
        ) noexcept
            : _allocator{ alloc }
            , _handle{ native_handle }
            , _object{ module_object }
            , _destroy_func{ destroy_module_func }
        {
        }

        ~SimpleAssetModule() noexcept
        {
            _destroy_func(_allocator, _object);
            FreeLibrary(_handle);
        }

    private:
        core::allocator& _allocator;
        HMODULE const _handle;
        void* const _object;
        AssetModuleDestroyFunc* const _destroy_func;
    };

} // namespace iceshard::detail

auto iceshard::load_asset_module(
    core::allocator& alloc,
    core::StringView path,
    asset::AssetSystem& asset_system
) noexcept -> core::memory::unique_pointer<AssetModule>
{
    auto module_path = std::filesystem::canonical(path);

    // The result object
    core::memory::unique_pointer<AssetModule> result{ nullptr, { alloc } };

    // Try to load the module.
    HMODULE module_handle = LoadLibraryExW(module_path.c_str(), NULL, NULL);
    if (module_handle != nullptr)
    {
        void* create_driver_addr = GetProcAddress(module_handle, "create_asset_module");
        void* destroy_driver_addr = GetProcAddress(module_handle, "destroy_asset_module");

        // Check both functions.
        if (create_driver_addr && destroy_driver_addr)
        {
            auto create_module_func = reinterpret_cast<detail::AssetModuleCreateFunc*>(create_driver_addr);
            auto release_module_func = reinterpret_cast<detail::AssetModuleDestroyFunc*>(destroy_driver_addr);

            result = { alloc.make<detail::SimpleAssetModule>(alloc, module_handle, create_module_func(alloc, asset_system), release_module_func), alloc };
        }
    }

    return result;
}
