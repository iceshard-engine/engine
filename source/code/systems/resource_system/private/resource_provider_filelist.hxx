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
#include "resource_filesystem_baked.hxx"

namespace ice
{

    struct FileListEntry
    {
        ice::native_file::HeapFilePath path;
        ice::ucount basepath_size;
    };

    class FileListResourceProvider;

    auto create_filesystem_provider_devui(
        ice::Allocator& alloc,
        ice::HashMap<ice::FileSystemResource*> const& resources
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

    class FileListResourceProvider final : public ice::ResourceProvider
    {
    public:
        FileListResourceProvider(
            ice::Allocator& alloc,
            ice::Span<ice::ResourceFileEntry const> entries,
            ice::native_aio::AIOPort aioport,
            ice::String virtual_hostname
        ) noexcept;

        ~FileListResourceProvider() noexcept override;

        auto schemeid() const noexcept -> ice::StringID override;

        void create_resource_from_file(
            ice::native_file::FilePath base_path,
            ice::native_file::FilePath file_path
        ) noexcept;

        void initial_traverse() noexcept;

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
        ) noexcept -> ice::TaskExpected<ice::Data> override;

        auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource const* root_resource
        ) const noexcept -> ice::Resource const* override;

        class DevUI;

    protected:
        ice::ProxyAllocator _named_allocator;
        ice::ProxyAllocator _data_allocator;
        ice::Array<ice::FileListEntry> _file_paths;
        ice::String _virtual_hostname;
        ice::native_aio::AIOPort _aioport;

        ice::HashMap<ice::FileSystemResource*> _resources;
        ice::Array<ice::Memory> _resources_data;
        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

} // namespace ice
