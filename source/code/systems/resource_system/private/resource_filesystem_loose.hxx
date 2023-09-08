
/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_meta.hxx>
#include <ice/resource_flags.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container_types.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/uri.hxx>

#include "resource_filesystem.hxx"
#include "native/native_fileio.hxx"

namespace ice
{

    class LooseFilesResource final : public ice::FileSystemResource
    {
    public:
        LooseFilesResource(
            ice::Allocator& alloc,
            ice::MutableMetadata metadata,
            ice::HeapString<> origin_path,
            ice::String origin_name,
            ice::String uri_path
        ) noexcept;

        ~LooseFilesResource() noexcept override;

        auto uri() const noexcept -> ice::URI const& override;
        auto flags() const noexcept -> ice::ResourceFlags override;

        auto name() const noexcept -> ice::String override;
        auto origin() const noexcept -> ice::String override;

        auto metadata() const noexcept -> ice::Metadata const& override;

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

        class ExtraResource;

    private:
        ice::MutableMetadata _mutable_metadata;
        ice::Metadata _metadata;

        ice::HeapString<> _origin_path;
        ice::String _origin_name;
        ice::String _uri_path;
        ice::URI _uri;

        ice::HashMap<ice::HeapString<>, ContainerLogic::Complex> _extra_resources;
    };

    class LooseFilesResource::ExtraResource final : public ice::FileSystemResource
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

        auto metadata() const noexcept -> ice::Metadata const& override;

        auto load_data(
            ice::Allocator& alloc,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> override;

        auto load_named_part(
            ice::StringID_Arg part_name,
            ice::Allocator& alloc
        ) const noexcept -> ice::Task<Memory> override { co_return {}; }

    private:
        ice::LooseFilesResource& _parent;

        ice::HeapString<> _origin_path;
        ice::ResourceFlags _flags;
    };

    auto create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::native_fileio::FilePath base_path,
        ice::native_fileio::FilePath uri_base_path,
        ice::native_fileio::FilePath meta_file,
        ice::native_fileio::FilePath data_file
    ) noexcept -> ice::FileSystemResource*;

} // namespace ice
