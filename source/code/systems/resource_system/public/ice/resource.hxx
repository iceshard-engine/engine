/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/string_types.hxx>
#include <ice/resource_types.hxx>

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

        virtual auto metadata() const noexcept -> ice::Metadata const& = 0;
    };

} // ice::res_v2
