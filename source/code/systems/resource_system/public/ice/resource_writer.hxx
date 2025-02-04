/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_provider.hxx>

namespace ice
{

    enum class ResourceCreationFlags : ice::u8
    {
        None,

        Overwrite = None,
        Append = 1,

        All = Overwrite | Append,
    };

    class ResourceWriter : public ice::ResourceProvider
    {
    public:
        virtual ~ResourceWriter() noexcept = default;

        virtual auto create_resource(
            ice::URI const& uri,
            ice::ResourceCreationFlags flags = ResourceCreationFlags::Overwrite
        ) noexcept -> ice::TaskExpected<ice::Resource*> = 0;

        virtual auto write_resource(
            ice::Resource const* resource,
            ice::Data data,
            ice::usize write_offset = 0_B
        ) noexcept -> ice::Task<bool> = 0;
    };

    auto create_resource_writer(
        ice::Allocator& alloc,
        ice::String const base_path,
        ice::TaskScheduler* scheduler = nullptr,
        ice::native_aio::AIOPort aioport = nullptr,
        ice::String virtual_hostname = {}
    ) noexcept -> ice::UniquePtr<ice::ResourceWriter>;

} // namespace ice
