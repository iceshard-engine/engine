
/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_file.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_flags.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container_types.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/uri.hxx>

#include "resource_filesystem.hxx"

namespace ice
{

    class LooseFilesResource final : public ice::FileSystemResource, public ice::LooseResource
    {
    public:
        LooseFilesResource(
            ice::Allocator& alloc,
            ice::Memory metadata,
            ice::HeapString<> origin_path,
            ice::String origin_name,
            ice::String uri_path
        ) noexcept;

        ~LooseFilesResource() noexcept override;

        auto uri() const noexcept -> ice::URI const& override;
        auto flags() const noexcept -> ice::ResourceFlags override;

        auto name() const noexcept -> ice::String override;
        auto origin() const noexcept -> ice::String override;

        auto load_metadata() const noexcept -> ice::Task<ice::Data> override;

        auto load_named_part(
            ice::StringID_Arg part_name,
            ice::Allocator& alloc
        ) const noexcept -> ice::Task<Memory> override;

        void add_named_part(
            ice::StringID_Arg name,
            ice::HeapString<> path
        ) noexcept;

        auto load_data(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> override;

        auto size() const noexcept -> ice::usize override;

        class ExtraResource;

    private:
        ice::Allocator& _allocator;
        ice::Memory _raw_metadata;
        ice::HeapString<> _origin_path;
        ice::String _origin_name;
        ice::String _uri_path;
        ice::URI _uri;

        ice::HashMap<ice::HeapString<>, ContainerLogic::Complex> _extra_resources;
    };

    class LooseFilesResource::ExtraResource final : public ice::FileSystemResource, public ice::LooseResource
    {
    public:
        ExtraResource(
            ice::LooseFilesResource& parent,
            ice::HeapString<> origin_path,
            ice::ResourceFlags flags
        ) noexcept;

        auto uri() const noexcept -> ice::URI const& override;
        auto flags() const noexcept -> ice::ResourceFlags override;

        auto name() const noexcept -> ice::String override;
        auto origin() const noexcept -> ice::String override;

        auto load_metadata() const noexcept -> ice::Task<ice::Data> override;

        auto load_data(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> override;

        auto load_named_part(
            ice::StringID_Arg part_name,
            ice::Allocator& alloc
        ) const noexcept -> ice::Task<Memory> override { co_return {}; }

        auto size() const noexcept -> ice::usize override;

    private:
        ice::LooseFilesResource& _parent;

        ice::HeapString<> _origin_path;
        ice::ResourceFlags _flags;
    };

    auto create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath uri_base_path,
        ice::native_file::FilePath meta_file,
        ice::native_file::FilePath data_file
    ) noexcept -> ice::FileSystemResource*;

} // namespace ice
