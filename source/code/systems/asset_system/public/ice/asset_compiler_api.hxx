#pragma once
#include <ice/stringid.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/uri.hxx>
#include <ice/task.hxx>

namespace ice
{

    struct AssetCompilerResult
    {
        ice::Memory result;
    };

    namespace api::asset_compiler::v1
    {

        using FnCollectAssetSources = bool(*)(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::ResourceHandle*>& out_sources
        ) noexcept;

        using FnCollectAssetDependencies = bool(*)(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::URI>& out_dependencies
        ) noexcept;

        using FnValidateAssetSource = auto(*)(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker
        ) noexcept -> ice::Task<bool>;

        using FnCompileAssetSource = auto(*)(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Span<ice::ResourceHandle* const> sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Task<ice::AssetCompilerResult>;

        using FnFinalizeAsset = auto(*)(
            ice::ResourceHandle* resource_handle,
            ice::Span<ice::AssetCompilerResult const> compiled_sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Memory;

        bool fn_collect_sources_default(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::ResourceHandle*>& out_sources
        ) noexcept;

        bool fn_collect_dependencies_default(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::URI>& out_dependencies
        ) noexcept;

        auto fn_validate_source_default(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker
        ) noexcept -> ice::Task<bool>;

        auto fn_compile_source_default(
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Span<ice::ResourceHandle* const> sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Task<ice::AssetCompilerResult>;

        auto fn_finalize_default(
            ice::ResourceHandle* resource_handle,
            ice::Span<ice::AssetCompilerResult const> compiled_sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Memory;

        struct AssetCompilerAPI
        {
            static constexpr ice::StringID Constant_APIName = "ice.api.asset-compiler.v1"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            FnCollectAssetSources fn_collect_sources = fn_collect_sources_default;
            FnCollectAssetDependencies fn_collect_dependencies = fn_collect_dependencies_default;
            FnValidateAssetSource fn_validate_source = fn_validate_source_default;
            FnCompileAssetSource fn_compile_source = fn_compile_source_default;
            FnFinalizeAsset fn_finalize = fn_finalize_default;
        };

    } // namespace api::asset_compiler::v1

} // namespace ice
