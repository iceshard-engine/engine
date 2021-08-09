#pragma once
#include <ice/uri.hxx>
#include <ice/urn.hxx>
#include <ice/span.hxx>
#include <ice/string.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    class Sink;

    class Resource;

    class ResourceIndex;

    struct ResourceQuery;

    class ResourceSystem
    {
    public:
        virtual ~ResourceSystem() noexcept = default;

        virtual void register_index(
            ice::Span<ice::StringID> schemes,
            ice::UniquePtr<ice::ResourceIndex> index
        ) noexcept = 0;

        virtual auto locate(ice::URN urn) const noexcept -> ice::URI = 0;

        virtual bool query_changes(ice::ResourceQuery& query) noexcept = 0;

        virtual auto mount(ice::URI const& uri) noexcept -> ice::u32 = 0;

        virtual auto find_relative(
            ice::Resource& root_resource,
            ice::String path
        ) noexcept -> ice::Resource* = 0;

        virtual auto request(ice::URI const& uri) noexcept -> ice::Resource* = 0;

        virtual void release(ice::URI const& uri) noexcept = 0;

        virtual auto open(ice::URI const& uri) noexcept -> ice::Sink* = 0;

        virtual void close(ice::URI const& uri) noexcept = 0;
    };

    auto create_resource_system(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ResourceSystem>;

} // namespace ice
