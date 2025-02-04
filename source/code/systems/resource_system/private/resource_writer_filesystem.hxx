/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/uri.hxx>
#include <ice/task.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/task_utils.hxx>
#include <ice/resource.hxx>
#include <ice/resource_writer.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/devui_widget.hxx>

#include "resource_filesystem_traverser.hxx"

namespace ice
{

    auto create_filesystem_provider_devui(
        ice::Allocator& alloc,
        ice::HashMap<ice::FileSystemResource*> const& resources
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

    class FileSystemResourceWriter : public ice::ResourceWriter, public ice::FileSystemTraverserCallbacks
    {
    public:
        FileSystemResourceWriter(
            ice::Allocator& alloc,
            ice::String base_path,
            ice::TaskScheduler* scheduler,
            ice::native_aio::AIOPort aioport,
            ice::String virtual_hostname
        ) noexcept;

        ~FileSystemResourceWriter() noexcept override;

        auto schemeid() const noexcept -> ice::StringID override;

        auto hostname() const noexcept -> ice::String override { return _virtual_hostname; }

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

    public: // ice::ResourceWriter
        auto create_resource(
            ice::URI const& uri,
            ice::ResourceCreationFlags flags = ResourceCreationFlags::Overwrite
        ) noexcept -> ice::TaskExpected<ice::Resource*> override;

        auto write_resource(
            ice::Resource const* resource,
            ice::Data data,
            ice::usize write_offset = 0_B
        ) noexcept -> ice::Task<bool> override;

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
        ice::native_file::HeapFilePath _base_path;
        ice::TaskScheduler* _scheduler;
        ice::native_aio::AIOPort _aioport;
        ice::String _virtual_hostname;

        ice::FileSystemTraverser _traverser;

        ice::HashMap<ice::WritableFileSystemResource*> _resources;
        ice::Array<ice::Memory> _resources_data;
        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

} // namespace ice
