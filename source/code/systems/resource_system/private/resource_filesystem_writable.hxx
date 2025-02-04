/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/native_aio.hxx>
#include <ice/native_file.hxx>
#include <ice/resource_flags.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container_types.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/uri.hxx>

#include "resource_filesystem.hxx"

namespace ice
{

    class WritableFileResource final : public ice::WritableFileSystemResource, public ice::LooseResource
    {
    public:
        WritableFileResource(
            ice::Allocator& alloc,
            ice::usize meta_size,
            ice::usize data_size,
            ice::HeapString<> origin_path,
            ice::String origin_name,
            ice::String uri_path
        ) noexcept;
        ~WritableFileResource() noexcept;

    public: // ice::Resource
        auto uri() const noexcept -> ice::URI const& override;
        auto flags() const noexcept -> ice::ResourceFlags override;

        auto name() const noexcept -> ice::String override;
        auto origin() const noexcept -> ice::String override;

    public: // ice::FileSystemResource
        auto size() const noexcept -> ice::usize override;

        auto load_data(
            ice::Allocator& alloc,
            ice::Memory& memory,
            ice::String fragment,
            ice::native_aio::AIOPort aioport
        ) const noexcept -> ice::TaskExpected<ice::Data> override;

    public: // ice::LooseResource
        auto load_named_part(
            ice::StringID_Arg part_name,
            ice::Allocator& alloc
        ) const noexcept -> ice::Task<Memory> override;

        void add_named_part(
            ice::StringID_Arg name,
            ice::HeapString<> path
        ) noexcept;

    public: // ice::WritableFileSystemResource
        auto write_data(
            ice::Allocator& alloc,
            ice::Data data,
            ice::usize write_offset,
            ice::String fragment,
            ice::native_aio::AIOPort aioport
        ) const noexcept -> ice::TaskExpected<ice::usize> override;

    private:
        ice::Allocator& _allocator;
        ice::HeapString<> _origin_path;
        ice::String _origin_name;
        ice::String _uri_path;
        ice::URI _uri;

        ice::usize _metasize;
        ice::usize _datasize;
    };

    auto create_writable_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::ResourceProvider& provider,
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath uri_base_path,
        ice::native_file::FilePath meta_file,
        ice::native_file::FilePath data_file
    ) noexcept -> ice::FileSystemResource*;

} // namespace ice
