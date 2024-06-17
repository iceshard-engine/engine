/// Copyright 2024 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_tracker.hxx>
#include <ice/resource_meta.hxx>
#include <ice/stringid.hxx>
#include <ice/task.hxx>
#include <ice/uri.hxx>

namespace ice
{

    struct ResourceCompilerCtx
    {
        void* userdata;
    };

    struct ResourceCompilerResult
    {
        ice::Memory result;
    };

    namespace api::resource_compiler::v1
    {

        using FnSupportedResources = auto(*)() noexcept -> ice::Span<ice::String>;

        using FnPrepareContext = auto(*)(
            ice::Allocator& alloc,
            ice::ResourceCompilerCtx& ctx
        ) noexcept -> bool;

        using FnCleanupContext = auto(*)(
            ice::Allocator& alloc,
            ice::ResourceCompilerCtx& ctx
        ) noexcept -> bool;

        using FnCollectResourceSources = bool(*)(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::ResourceHandle*>& out_sources
        ) noexcept;

        using FnCollectResourceDependencies = bool(*)(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::URI>& out_dependencies
        ) noexcept;

        using FnValidateResourceSource = auto(*)(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker
        ) noexcept -> ice::Task<bool>;

        using FnCompileResourceSource = auto(*)(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Span<ice::ResourceHandle* const> sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Task<ice::ResourceCompilerResult>;

        using FnBuildResourceMetadata = auto(*)(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Span<ice::ResourceCompilerResult const> compiled_sources,
            ice::Span<ice::URI const> dependencies,
            ice::MutableMetadata& out_metadata
        ) noexcept -> ice::Task<bool>;

        using FnFinalizeResource = auto(*)(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::Span<ice::ResourceCompilerResult const> compiled_sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Memory;

        bool fn_collect_sources_default(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::ResourceHandle*>& out_sources
        ) noexcept;

        bool fn_collect_dependencies_default(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Array<ice::URI>& out_dependencies
        ) noexcept;

        auto fn_validate_source_default(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker
        ) noexcept -> ice::Task<bool>;

        auto fn_compile_source_default(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Span<ice::ResourceHandle* const> sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Task<ice::ResourceCompilerResult>;

        auto fn_build_metadata_default(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::ResourceTracker& resource_tracker,
            ice::Span<ice::ResourceCompilerResult const> compiled_sources,
            ice::Span<ice::URI const> dependencies,
            ice::MutableMetadata& out_metadata
        ) noexcept -> ice::Task<bool>;

        auto fn_finalize_default(
            ice::ResourceCompilerCtx& ctx,
            ice::ResourceHandle* resource_handle,
            ice::Span<ice::ResourceCompilerResult const> compiled_sources,
            ice::Span<ice::URI const> dependencies,
            ice::Allocator& result_alloc
        ) noexcept -> ice::Memory;

        struct ResourceCompilerAPI
        {
            static constexpr ice::StringID Constant_APIName = "ice.api.resource-compiler.v1"_sid;
            static constexpr ice::u32 Constant_APIVersion = 1;

            FnSupportedResources fn_supported_resources = nullptr;
            FnPrepareContext fn_prepare_context = nullptr;
            FnCleanupContext fn_cleanup_context = nullptr;
            FnCollectResourceSources fn_collect_sources = fn_collect_sources_default;
            FnCollectResourceDependencies fn_collect_dependencies = fn_collect_dependencies_default;
            FnValidateResourceSource fn_validate_source = fn_validate_source_default;
            FnCompileResourceSource fn_compile_source = fn_compile_source_default;
            FnBuildResourceMetadata fn_build_metadata = fn_build_metadata_default;
            FnFinalizeResource fn_finalize = fn_finalize_default;
        };

        struct ResourceCompilerCtxCleanup
        {
            ice::Allocator& alloc;
            ice::ResourceCompilerCtx& ctx;
            ice::api::resource_compiler::v1::FnCleanupContext ctx_cleanup;

            ~ResourceCompilerCtxCleanup() noexcept
            {
                if (ctx.userdata && ctx_cleanup)
                {
                    bool const cleanup_success = ctx_cleanup(alloc, ctx);
                    ICE_ASSERT_CORE(cleanup_success);
                }
            }
        };

    } // namespace api::resource_compiler::v1

} // namespace ice
