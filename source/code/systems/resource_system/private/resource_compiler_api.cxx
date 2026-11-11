/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/resource_compiler_api.hxx>
#include <ice/config.hxx>

namespace ice::api::resource_compiler::v1
{

    bool fn_collect_sources_default(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Array<ice::ResourceHandle>& out_sources
    ) noexcept
    {
        return true; // No additional sources are needed.
    }

    bool fn_collect_dependencies_default(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Array<ice::URI>& out_dependencies
    ) noexcept
    {
        return true; // No additional dependencies are needed.
    }

    auto fn_validate_source_default(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::ResourceTracker& resource_tracker
    ) noexcept -> ice::Task<bool>
    {
        co_return true; // The resource is already loaded and we don't checke the data.
    }

    auto fn_compile_source_default(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Span<ice::ResourceHandle const> sources,
        ice::Span<ice::URI const> dependencies,
        ice::Allocator& result_alloc
    ) noexcept -> ice::Task<ice::ResourceCompilerResult>
    {
        ice::ResourceResult const res = co_await resource_tracker.load_resource(resource_handle);
        ice::Memory result = result_alloc.allocate(res.data.size);

        // We copy the data as-is in the default implementation.
        ice::memcpy(result, res.data);

        co_return ResourceCompilerResult{ .result = result };
    }

    auto fn_build_metadata_default(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Span<ice::ResourceCompilerResult const> compiled_sources,
        ice::Span<ice::URI const> dependencies,
        ice::ConfigBuilder& out_metadata
    ) noexcept -> ice::Task<bool>
    {
        ice::Data data{};
        if (co_await ice::resource_meta(resource_handle, data) == S_Ok)
        {
            out_metadata.merge(ice::config::from_data(data));
            co_return true;
        }
        co_return true;
    }

    auto fn_finalize_default(
        ice::ResourceCompilerCtx& ctx,
        ice::ResourceHandle const& resource_handle,
        ice::Span<ice::URI const> dependencies,
        ice::Span<ice::ResourceCompilerResult> compiled_sources,
        ice::Allocator& result_alloc
    ) noexcept -> ice::Memory
    {
        // If we have less that 1, that means compilation actually failed, else what are we doing here?
        // If we have more than 1, then we need to provide our own custom finalizer, else how do we access the data?
        ICE_ASSERT_CORE(compiled_sources.size() == 1);

        // Move the allocated memory away from the compiled source, we don't want it to be deallocated.
        return ice::exchange(compiled_sources[0].result, {});
    }

} // namespace ice::api::resource_compiler::v1
