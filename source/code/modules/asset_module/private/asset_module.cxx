#include <core/allocator.hxx>
#include <asset_system/asset_system.hxx>

#include "mesh_assets/mesh_resolvers.hxx"
#include "mesh_assets/mesh_compilers.hxx"
#include "mesh_assets/mesh_loaders.hxx"
#include "texture_assets/texture_resolvers.hxx"
#include "texture_assets/texture_compiler.hxx"
#include "texture_assets/texture_loaders.hxx"

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
            , _loader_handles{ _allocator }
            , _compiler_handles{ _allocator }
            , _resolver_handles{ _allocator }
        {
            register_operations();
        }

        ~IceshardAssetModule() noexcept
        {
            unregister_operations();
        }

    private:
        void register_operations() noexcept
        {
            // Asset resolvers
            core::pod::array::reserve(_resolver_handles, 2);
            core::pod::array::push_back(
                _resolver_handles,
                _asset_system.add_resolver(
                    core::memory::make_unique<asset::AssetResolver, iceshard::AssimpMeshResolver>(_allocator)
                )
            );
            core::pod::array::push_back(
                _resolver_handles,
                _asset_system.add_resolver(
                    core::memory::make_unique<asset::AssetResolver, iceshard::StbTextureResolver>(_allocator)
                )
            );

            // Asset compilers
            core::pod::array::reserve(_compiler_handles, 2);
            core::pod::array::push_back(
                _compiler_handles,
                _asset_system.add_compiler(
                    core::memory::make_unique<asset::AssetCompiler, iceshard::AssimpMeshCompiler>(_allocator)
                )
            );
            core::pod::array::push_back(
                _compiler_handles,
                _asset_system.add_compiler(
                    core::memory::make_unique<asset::AssetCompiler, iceshard::StbTextureCompiler>(_allocator)
                )
            );

            // Asset loaders
            core::pod::array::reserve(_loader_handles, 2);
            core::pod::array::push_back(
                _loader_handles,
                _asset_system.add_loader(
                    core::memory::make_unique<asset::AssetLoader, iceshard::AssimpMeshLoader>(_allocator, _allocator)
                )
            );
            core::pod::array::push_back(
                _loader_handles,
                _asset_system.add_loader(
                    core::memory::make_unique<asset::AssetLoader, iceshard::StbTextureLoader>(_allocator, _allocator)
                )
            );
        }

        void unregister_operations() noexcept
        {
            for (auto handle : _loader_handles)
            {
                _asset_system.remove_loader(handle);
            }
            for (auto handle : _compiler_handles)
            {
                _asset_system.remove_compiler(handle);
            }
            for (auto handle : _resolver_handles)
            {
                _asset_system.remove_resolver(handle);
            }
        }

    private:
        core::allocator& _allocator;
        asset::AssetSystem& _asset_system;

        core::pod::Array<asset::AssetLoaderHandle> _loader_handles;
        core::pod::Array<asset::AssetCompilerHandle> _compiler_handles;
        core::pod::Array<asset::AssetResolverHandle> _resolver_handles;
    };

} // namespace iceshard

extern "C"
{

    __declspec(dllexport) void* create_asset_module(core::allocator& alloc, asset::AssetSystem& asset_system)
    {
        return alloc.make<iceshard::IceshardAssetModule>(alloc, asset_system);
    }

    __declspec(dllexport) void destroy_asset_module(core::allocator& alloc, iceshard::IceshardAssetModule* asset_module)
    {
        alloc.destroy(asset_module);
    }

} // extern "C"
