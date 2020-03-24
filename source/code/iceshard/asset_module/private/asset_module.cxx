#include <core/allocator.hxx>
#include <asset_system/asset_system.hxx>

#include "mesh_assets/mesh_resolvers.hxx"

namespace iceshard
{
    class IceshardAssetModule
    {
    public:
        IceshardAssetModule(
            core::allocator& alloc,
            asset::AssetSystem& asset_system
        ) noexcept
            : _allocator{ alloc }
            , _asset_system{ asset_system }
        {
            register_resolvers();
            register_loaders();
        }

    private:
        void register_resolvers() noexcept
        {
        }

        void register_loaders() noexcept
        {
        }

    private:
        core::allocator& _allocator;
        asset::AssetSystem& _asset_system;
    };
}

extern "C"
{

    void* create_asset_module(core::allocator& alloc, asset::AssetSystem& asset_system)
    {
        return alloc.make<iceshard::IceshardAssetModule>(alloc, asset_system);
    }

    void destroy_asset_module(core::allocator& alloc, iceshard::IceshardAssetModule* asset_module)
    {
        alloc.destroy(asset_module);
    }

} // extern "C"
