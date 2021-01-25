#pragma once
#include <ice/allocator.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    class ModuleRegister;

    class AssetSystem;
    class AssetPipeline;

    void load_asset_pipeline_modules(
        ice::Allocator& alloc,
        ice::ModuleRegister const& registry,
        ice::AssetSystem& asset_system
    ) noexcept;

    namespace detail::asset_system::v1
    {

        using CreateFn = auto(ice::Allocator&) noexcept -> ice::AssetPipeline*;
        using DestroyFn = void(ice::Allocator& alloc, ice::AssetPipeline*) noexcept;

        struct AssetModuleAPI
        {
            CreateFn* create_pipeline_fn;
            DestroyFn* destroy_pipeline_fn;
        };

    } // detail::engine::v1

} // namespace ice
