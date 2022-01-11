#pragma once
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    class Resource_v2;

    enum class ResourceProviderResult : ice::u32;

    class ResourceProvider_v2
    {
    public:
        virtual ~ResourceProvider_v2() noexcept = default;

        virtual bool query_changes(ice::pod::Array<ice::Resource_v2 const*>& out_changes) const noexcept = 0;

        virtual auto refresh() noexcept -> ice::Task<ice::ResourceProviderResult> = 0;
        virtual auto reset() noexcept -> ice::Task<ice::ResourceProviderResult> = 0;
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider_v2>;

} // namespace ice
