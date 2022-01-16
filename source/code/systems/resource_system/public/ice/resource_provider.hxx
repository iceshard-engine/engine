#pragma once
#include <ice/memory.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>
#include <ice/resource_action.hxx>

namespace ice
{

    class Resource_v2;
    class TaskScheduler_v2;

    enum class ResourceProviderResult : ice::u32
    {
        Success,
        Failure,
        Skipped,
    };

    class ResourceProvider_v2
    {
    public:
        virtual ~ResourceProvider_v2() noexcept = default;

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
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider_v2>;

    auto create_resource_provider_dlls(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider_v2>;

} // namespace ice
