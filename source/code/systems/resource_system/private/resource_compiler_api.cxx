/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
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
        ice::Span<ice::ResourceCompilerResult const> compiled_sources,
        ice::Span<ice::URI const> dependencies,
        ice::Allocator& result_alloc
    ) noexcept -> ice::Memory
    {
        ice::usize total_size = 0_B;
        for (ice::ResourceCompilerResult const& source : compiled_sources)
        {
            total_size += source.result.size;
        }

        ice::Memory const result = result_alloc.allocate(total_size);

        ice::Memory copytarget = result;
        for (ice::ResourceCompilerResult const& source : compiled_sources)
        {
            // We copy the data as-is in the default implementation.
            ice::memcpy(copytarget, ice::data_view(source.result));
            copytarget = ice::ptr_add(copytarget, source.result.size);
        }

        return result;
    }

} // namespace ice::api::resource_compiler::v1
