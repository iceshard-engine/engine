/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_flags.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container_types.hxx>
#include <ice/string_types.hxx>
#include <ice/os/windows.hxx>
#include <ice/uri.hxx>

#include "resource_common_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    class Resource_LooseFilesWin32 final : public ice::Resource_Win32
    {
    public:
        Resource_LooseFilesWin32(
            ice::Allocator& alloc,
            ice::MutableMetadata metadata,
            ice::HeapString<> origin_path,
            ice::String origin_name,
            ice::String uri_path
        ) noexcept;

        ~Resource_LooseFilesWin32() noexcept override;

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
            ice::NativeIO* nativeio
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

    class Resource_LooseFilesWin32::ExtraResource final : public ice::Resource_Win32
    {
    public:
        ExtraResource(
            ice::Resource_LooseFilesWin32& parent,
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
            ice::NativeIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> override;

    private:
        ice::Resource_LooseFilesWin32& _parent;

        ice::HeapString<> _origin_path;
        ice::ResourceFlags _flags;
    };

    auto create_resources_from_loose_files(
        ice::Allocator& alloc,
        ice::WString base_path,
        ice::WString uri_base_path,
        ice::WString meta_file,
        ice::WString data_file
    ) noexcept -> ice::Resource_Win32*;

} // namespace ice

#endif // #if ISP_WINDOWS
