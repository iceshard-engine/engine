/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem.hxx>
#include <ice/asset.hxx>
#include <ice/asset_type.hxx>

namespace ice
{

    class Resource;

    struct AssetTypeDefinition;

    enum class AssetRequestResult : ice::u8
    {
        Error,
        Skipped,
        Success,
    };

    struct AssetRequestResolver
    {
        virtual ~AssetRequestResolver() noexcept = default;

        virtual auto on_asset_released(ice::Asset const& asset) noexcept -> ice::Task<> = 0;
    };

    struct AssetResolveData
    {
        ice::AssetRequestResolver* resolver;
        ice::AssetRequestResult result;
        ice::Memory memory;
    };

    class AssetRequest
    {
    public:
        virtual ~AssetRequest() noexcept = default;

        virtual auto state() const noexcept -> ice::AssetState = 0;
        virtual auto data() const noexcept -> ice::Data = 0;

        virtual auto asset_name() const noexcept -> ice::StringID_Arg = 0;
        virtual auto asset_definition() const noexcept -> ice::AssetTypeDefinition const& = 0;
        virtual auto resource() const noexcept -> ice::Resource const& = 0;

        virtual auto allocate(ice::usize size) const noexcept -> ice::Memory = 0;

        virtual auto resolve(
            ice::AssetResolveData resolve_data
        ) noexcept -> ice::Asset = 0;
    };

} // namespace ice
