/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/string_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/task.hxx>

namespace ice
{

    struct Metadata;

    class Resource
    {
    public:
        virtual ~Resource() noexcept = default;

        virtual auto uri() const noexcept -> ice::URI const& = 0;
        virtual auto flags() const noexcept -> ice::ResourceFlags = 0;

        virtual auto name() const noexcept -> ice::String = 0;
        virtual auto origin() const noexcept -> ice::String = 0;

        virtual auto load_metadata(ice::Metadata& out_view) const noexcept -> ice::Task<bool>
        {
            co_return false;
        }
    };

    class LooseResource : public Resource
    {
    public:
        virtual auto size() const noexcept -> ice::usize = 0;

        virtual auto load_named_part(
            ice::StringID_Arg part_name,
            ice::Allocator& alloc
        ) const noexcept -> ice::Task<ice::Memory> = 0;
    };

} // ice
