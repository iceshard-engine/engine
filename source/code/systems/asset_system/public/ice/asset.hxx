#pragma once
#include <ice/data.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    enum class Asset : ice::u64;
    enum class AssetType : ice::u32;
    enum class AssetStatus : ice::u32;

    auto asset_status(ice::Asset asset) noexcept -> ice::AssetStatus;
    auto asset_data(ice::Asset asset, ice::Data& out_data) noexcept -> ice::AssetStatus;
    auto asset_metadata(ice::Asset asset, ice::Data& out_data) noexcept -> ice::AssetStatus;


    enum class Asset : ice::u64
    {
        Invalid = 0x0
    };

    enum class AssetType : ice::u32
    {
        Invalid = 0x0,
        Config,
        Mesh,
        Shape2D,
        Shader,
        Texture,

        Level,

        Reserved = 0x00ff'ffff,
        Unresolved = 0xffff'ffff,
    };

    enum class AssetStatus : ice::u32
    {
        Invalid = 0x0,
        Available_Raw,
        Available,
        Requested,
        Baking,
        Baked,
        Loading,
        Loaded,
        Unused,
        Unloading,
    };

} // namespace ice
