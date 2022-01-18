#pragma once
#include <ice/memory.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>
#include <ice/resource_types.hxx>

namespace ice
{

    class TaskScheduler_v2;

    enum class ResourceProviderResult : ice::u32
    {
        Success,
        Failure,
        Skipped,
    };

    class ResourceProvider
    {
    public:
        virtual ~ResourceProvider() noexcept = default;

        virtual auto query_resources(
            ice::pod::Array<ice::Resource_v2 const*>& out_changes
        ) const noexcept -> ice::u32 = 0;

        virtual auto refresh() noexcept -> ice::Task<ice::ResourceProviderResult> = 0;

        virtual auto load_resource(
            ice::Allocator& alloc,
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<ice::Memory> = 0;

        virtual auto release_resource(
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<> = 0;

        virtual auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource_v2 const* root_resource
        ) const noexcept -> ice::Resource_v2 const*
        {
            return nullptr;
        }
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>;

    auto create_resource_provider_dlls(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>;

} // namespace ice
