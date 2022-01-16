#pragma once
//#include <ice/uri.hxx>
//#include <ice/resource.hxx>
//#include <ice/unique_ptr.hxx>
//#include <ice/pod/collections.hxx>
//
//namespace ice
//{
//
//    struct ResourceQuery;
//
//    class ResourceIndex
//    {
//    public:
//        virtual ~ResourceIndex() noexcept = default;
//
//        virtual auto find_relative(ice::Resource const& root_resource, ice::String path) noexcept -> ice::Resource* = 0;
//
//        virtual bool query_changes(ice::ResourceQuery& query) noexcept = 0;
//
//        virtual bool mount(ice::URI const& uri) noexcept = 0;
//
//        virtual auto request(ice::URI const& uri) noexcept -> ice::Resource* = 0;
//
//        virtual bool release(ice::URI const& uri) noexcept = 0;
//    };
//
//    auto create_filesystem_index(ice::Allocator& alloc, ice::String base_path) noexcept -> ice::UniquePtr<ice::ResourceIndex>;
//
//    auto create_dynlib_index(ice::Allocator& alloc, ice::String app_relative_path) noexcept -> ice::UniquePtr<ice::ResourceIndex>;
//
//} // namespace ice
