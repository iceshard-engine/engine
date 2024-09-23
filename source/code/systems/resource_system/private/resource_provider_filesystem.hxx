/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
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

namespace ice
{

    class FileSystemResourceProvider;

    auto create_filesystem_provider_devui(
        ice::Allocator& alloc,
        ice::HashMap<ice::FileSystemResource*> const& resources
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

    class FileSystemResourceProvider final : public ice::ResourceProvider
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

        void create_resource_from_file(
            ice::native_file::FilePath base_path,
            ice::native_file::FilePath file_path
        ) noexcept;

        struct TraverseResourceRequest;

        auto traverse_async(
            ice::native_file::HeapFilePath dir_path,
            TraverseResourceRequest& request
        ) noexcept -> ice::Task<>;

        auto create_resource_from_file_async(
            ice::native_file::FilePath base_path,
            ice::native_file::HeapFilePath file_path,
            TraverseResourceRequest& request
        ) noexcept -> ice::Task<>;

        static auto traverse_callback(
            ice::native_file::FilePath,
            ice::native_file::FilePath path,
            ice::native_file::EntityType type,
            void* userdata
        ) noexcept -> ice::native_file::TraverseAction;

        void initial_traverse() noexcept;

        void initial_traverse_mt() noexcept;

        auto collect(
            ice::Array<ice::Resource const*>& out_changes
        ) noexcept -> ice::ucount override;

        auto refresh(
            ice::Array<ice::Resource const*>& out_changes
        ) noexcept -> ice::ResourceProviderResult override;

        auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource const* override;

        auto access_loose_resource(
            ice::Resource const* resource
        ) const noexcept -> ice::LooseResource const* override;

        void unload_resource(
            ice::Allocator& alloc,
            ice::Resource const* /*resource*/,
            ice::Memory memory
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

    protected:
        ice::ProxyAllocator _named_allocator;
        ice::ProxyAllocator _data_allocator;
        ice::Array<ice::native_file::HeapFilePath, ice::ContainerLogic::Complex> _base_paths;
        ice::TaskScheduler* _scheduler;
        ice::native_aio::AIOPort _aioport;
        ice::String _virtual_hostname;

        ice::HashMap<ice::FileSystemResource*> _resources;
        ice::Array<ice::Memory> _resources_data;
        ice::UniquePtr<ice::DevUIWidget> _devui_widget;
    };

} // namespace ice
