/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>
#include <ice/asset.hxx>
#include <ice/asset_type.hxx>

namespace ice
{

    class Resource_v2;

    struct AssetTypeDefinition;

    class AssetRequest
    {
    public:
        enum class Result
        {
            Error,
            Skipped,
            Success,
        };

        virtual ~AssetRequest() noexcept = default;

        virtual auto state() const noexcept -> ice::AssetState = 0;
        virtual auto data() const noexcept -> ice::Data = 0;

        virtual auto resource() const noexcept -> ice::Resource_v2 const& = 0;
        virtual auto asset_definition() const noexcept -> ice::AssetTypeDefinition const& = 0;

        virtual auto allocate(ice::usize size) const noexcept -> ice::Memory = 0;

        virtual auto resolve(
            ice::AssetRequest::Result result,
            ice::Memory memory
        ) noexcept -> ice::AssetHandle const* = 0;
    };

} // namespace ice
