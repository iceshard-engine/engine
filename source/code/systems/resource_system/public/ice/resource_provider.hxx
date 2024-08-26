/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/stringid.hxx>
#include <ice/string_types.hxx>
#include <ice/mem_memory.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/container_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/task_types.hxx>

namespace ice
{

    struct NativeAIO;

    enum class ResourceProviderResult : ice::u32
    {
        Success,
        Failure,
        Skipped,
    };

    class ResourceProvider
    {
    public:
        virtual ~ResourceProvider() noexcept = default;

        virtual auto schemeid() const noexcept -> ice::StringID = 0;

        virtual auto collect(
            ice::Array<ice::Resource const*>& out_changes
        ) noexcept -> ice::ucount
        {
            return 0;
        }

        virtual auto refresh(
            ice::Array<ice::Resource const*>& out_changes
        ) noexcept -> ice::ResourceProviderResult = 0;

        virtual auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource const*
        {
            return nullptr;
        }

        virtual auto access_loose_resource(
            ice::Resource const* resource
        ) const noexcept -> ice::LooseResource const*
        {
            return nullptr;
        }

        virtual void unload_resource(
            ice::Allocator& alloc,
            ice::Resource const* resource,
            ice::Memory memory
        ) noexcept = 0;

        virtual auto load_resource(
            ice::Allocator& alloc,
            ice::Resource const* resource,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> = 0;

        virtual auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource const* root_resource
        ) const noexcept -> ice::Resource const*
        {
            return nullptr;
        }
    };

    struct ResourceFileEntry
    {
        ice::String path;
        ice::String basepath = {};
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Span<ice::String const> paths,
        ice::TaskScheduler* scheduler = nullptr,
        ice::String virtual_hostname = {}
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>;

    auto create_resource_provider_files(
        ice::Allocator& alloc,
        ice::Span<ice::ResourceFileEntry const> entries,
        ice::TaskScheduler* scheduler = nullptr,
        ice::String virtual_hostname = {}
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>;

    auto create_resource_provider_dlls(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>;

    auto create_resource_provider_hailstorm(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>;

    auto create_resource_provider_custom(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>;

} // namespace ice
