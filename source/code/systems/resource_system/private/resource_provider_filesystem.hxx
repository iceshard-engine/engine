/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/uri.hxx>
#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/task_utils.hxx>
#include <ice/resource.hxx>
#include <ice/resource_provider.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/devui_widget.hxx>

#include "resource_filesystem_loose.hxx"
#include "resource_filesystem_traverser.hxx"

namespace ice
{

    class FileSystemResourceProvider;

    auto create_filesystem_provider_devui(
        ice::Allocator& alloc,
        ice::HashMap<ice::FileSystemResource*> const& resources
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

    class FileSystemResourceProvider : public ice::ResourceProvider, public ice::FileSystemTraverserCallbacks
    {
    public:
        FileSystemResourceProvider(
            ice::Allocator& alloc,
            ice::Span<ice::String const> const& paths,
            ice::TaskScheduler* scheduler,
            ice::native_aio::AIOPort aioport,
            ice::String virtual_hostname
        ) noexcept;

        ~FileSystemResourceProvider() noexcept override;

        auto schemeid() const noexcept -> ice::StringID override;

        auto collect(
            ice::Array<ice::Resource*>& out_changes
        ) noexcept -> ice::ucount override;

        auto refresh(
            ice::Array<ice::Resource*>& out_changes
        ) noexcept -> ice::ResourceProviderResult override;

        auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource* override;

        auto access_loose_resource(
            ice::Resource const* resource
        ) const noexcept -> ice::LooseResource const* override;

        void unload_resource(
            ice::Resource const* /*resource*/
        ) noexcept override;

        auto load_resource(
            ice::Resource const* resource,
            ice::String fragment
        ) noexcept -> ice::TaskExpected<ice::Data, ice::ErrorCode> override;

        auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource const* root_resource
        ) const noexcept -> ice::Resource const* override;

        class DevUI;

    public: // ice::FileSystemTraverserCallbacks
        auto allocator() noexcept -> ice::Allocator& override;

        auto create_baked_resource(
            ice::native_file::FilePath filepath
        ) noexcept -> ice::FileSystemResource* override;

        auto create_loose_resource(
            ice::native_file::FilePath base_path,
            ice::native_file::FilePath uri_base_path,
            ice::native_file::FilePath meta_filepath,
            ice::native_file::FilePath data_filepath
        ) noexcept -> ice::FileSystemResource* override;

        auto register_resource(
            ice::FileSystemResource* resource
        ) noexcept -> ice::Result override;

        void destroy_resource(
            ice::FileSystemResource* resource
        ) noexcept override;


    protected:
        ice::ProxyAllocator _named_allocator;
        ice::ProxyAllocator _data_allocator;
        ice::Array<ice::native_file::HeapFilePath, ice::ContainerLogic::Complex> _base_paths;
        ice::TaskScheduler* _scheduler;
        ice::native_aio::AIOPort _aioport;
        ice::String _virtual_hostname;

        ice::FileSystemTraverser _traverser;

        ice::HashMap<ice::FileSystemResource*> _resources;
        ice::Array<ice::Memory> _resources_data;
        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

} // namespace ice
