#pragma once
#include <ice/uri.hxx>
#include <ice/urn.hxx>
#include <ice/unique_ptr.hxx>

namespace ice
{

    class Sink;

    class Resource;

    class ResourceIndex;

    //using ResourceIndexFactory = auto(ice::Allocator&, ice::URI const&) noexcept -> ice::UniquePtr<ice::ResourceIndex>;


    class ResourceSystem
    {
    public:
        virtual ~ResourceSystem() noexcept = default;

        virtual void register_index(
            ice::StringID_Arg scheme,
            ice::UniquePtr<ice::ResourceIndex> index
        ) noexcept = 0;

        virtual auto locate(ice::URN urn) const noexcept -> ice::URI = 0;

        virtual auto mount(ice::URI const& uri) noexcept -> ice::u32 = 0;

        virtual auto request(ice::URI const& uri) noexcept -> ice::Resource* = 0;

        virtual void release(ice::URI const& uri) noexcept = 0;

        virtual auto open(ice::URI const& uri) noexcept -> ice::Sink* = 0;

        virtual void close(ice::URI const& uri) noexcept = 0;
    };

    auto create_resource_system(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ResourceSystem>;

} // namespace ice
