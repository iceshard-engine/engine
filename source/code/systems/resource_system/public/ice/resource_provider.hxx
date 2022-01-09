#pragma once
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    class Resource_v2;

    struct ResourceProviderAction_v2;
    struct ResourceAction_v2;

    class ResourceProvider_v2
    {
    public:
        virtual ~ResourceProvider_v2() noexcept = default;

        virtual auto supported_provider_actions_count() const noexcept -> ice::u32 = 0;
        virtual auto supported_provider_actions() const noexcept -> ice::Span<ice::ResourceProviderAction_v2 const> = 0;

        virtual auto supported_resource_actions_count() const noexcept -> ice::u32 = 0;
        virtual auto supported_resource_actions() const noexcept -> ice::Span<ice::ResourceAction_v2 const> = 0;

        virtual bool query_changes(ice::pod::Array<ice::Resource_v2 const*>& out_changes) const noexcept = 0;
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider_v2>;

} // namespace ice
