#include <ice/asset_compiler.hxx>
#include <ice/resource_tracker.hxx>

namespace ice::api::asset_compiler::v1
{

    bool fn_collect_sources_default(
        ice::ResourceHandle* resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Array<ice::ResourceHandle*>& out_sources
    ) noexcept
    {
        return true; // No additional sources are needed.
    }

    bool fn_collect_dependencies_default(
        ice::ResourceHandle* resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Array<ice::URI>& out_dependencies
    ) noexcept
    {
        return true; // No additional dependencies are needed.
    }

    auto fn_validate_source_default(
        ice::ResourceHandle* resource_handle,
        ice::ResourceTracker& resource_tracker
    ) noexcept -> ice::Task<bool>
    {
        co_return true; // The resource is already loaded and we don't checke the data.
    }

    auto fn_compile_source_default(
        ice::ResourceHandle* resource_handle,
        ice::ResourceTracker& resource_tracker,
        ice::Span<ice::ResourceHandle* const> sources,
        ice::Span<ice::URI const> dependencies,
        ice::Allocator& result_alloc
    ) noexcept -> ice::Task<ice::AssetCompilerResult>
    {
        ice::ResourceResult const res = co_await resource_tracker.load_resource(resource_handle);
        ice::Memory result = result_alloc.allocate(res.data.size);

        // We copy the data as-is in the default implementation.
        ice::memcpy(result, res.data);

        co_return AssetCompilerResult{ .result = result };
    }

    auto fn_finalize_default(
        ice::ResourceHandle* resource_handle,
        ice::Span<ice::AssetCompilerResult const> compiled_sources,
        ice::Span<ice::URI const> dependencies,
        ice::Allocator& result_alloc
    ) noexcept -> ice::Memory
    {
        ice::usize total_size = 0_B;
        for (ice::AssetCompilerResult const& source : compiled_sources)
        {
            total_size += source.result.size;
        }

        ice::Memory const result = result_alloc.allocate(total_size);

        ice::Memory copytarget = result;
        for (ice::AssetCompilerResult const& source : compiled_sources)
        {
            // We copy the data as-is in the default implementation.
            ice::memcpy(copytarget, ice::data_view(source.result));
            copytarget = ice::ptr_add(copytarget, source.result.size);
        }

        return result;
    }

} // namespace ice::api::asset_compiler::v1
