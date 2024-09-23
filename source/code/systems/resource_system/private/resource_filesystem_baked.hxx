/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


#pragma once
#include <ice/native_file.hxx>
#include <ice/resource_flags.hxx>
#include <ice/resource_format.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/container_types.hxx>
#include <ice/uri.hxx>

#include "resource_filesystem.hxx"

namespace ice
{

    class BakedFileResource final : public ice::FileSystemResource, public ice::LooseResource /* REDESIGN */
    {
    public:
        BakedFileResource(
            ice::Allocator& alloc,
            ice::ResourceFormatHeader const& header,
            ice::HeapString<> origin,
            ice::HeapString<> name,
            ice::Memory metadata
        ) noexcept;

        ~BakedFileResource() noexcept override;

        auto uri() const noexcept -> ice::URI const& override;
        auto flags() const noexcept -> ice::ResourceFlags override;

        auto name() const noexcept -> ice::String override;
        auto origin() const noexcept -> ice::String override;

        auto size() const noexcept -> ice::usize override;

        auto load_data(
            ice::Allocator& alloc,
            ice::Memory& memory,
            ice::String fragment,
            ice::native_aio::AIOPort aioport
        ) const noexcept -> ice::TaskExpected<ice::Data> override;

        auto load_named_part(
            ice::StringID_Arg part_name,
            ice::Allocator& alloc
        ) const noexcept -> ice::Task<ice::Memory> override
        {
            co_return{};
        }

    private:
        ice::Allocator& _allocator;
        ice::ResourceFormatHeader const _header;
        ice::HeapString<> _origin;
        ice::HeapString<> _name;
        ice::URI _uri;
    };

    auto create_resource_from_baked_file(
        ice::Allocator& alloc,
        ice::String uri_base,
        ice::native_file::FilePath file_path
    ) noexcept -> ice::FileSystemResource*;

} // namespace ice
