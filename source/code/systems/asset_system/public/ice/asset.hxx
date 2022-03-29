#pragma once
#include <coroutine>
#include <ice/data.hxx>
#include <ice/stringid.hxx>

namespace ice
{

    struct Metadata;

    class AssetStorage;

    enum class Asset : ice::u64;
    enum class AssetType : ice::u32;
    enum class AssetStatus : ice::u32;

    auto asset_name(ice::Asset asset) noexcept -> ice::StringID;
    auto asset_status(ice::Asset asset) noexcept -> ice::AssetStatus;
    auto asset_data(ice::Asset asset, ice::Data& out_data) noexcept -> ice::AssetStatus;
    auto asset_metadata(ice::Asset asset, ice::Metadata& out_metadata) noexcept -> ice::AssetStatus;


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
        TileMap,

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


    enum class AssetState : ice::u32
    {
        //! \brief The asset could not be accessed.
        //! \detail Either an error occured or the asset does not exist.
        Invalid,

        //! \brief The asset data is represented in raw format of the source file.
        //! \detail This means that you might get raw jpg or png image data.
        Raw,

        //! \brief The asset data is baked into a specific engine representation.
        //! \detail Please take a look at the data representation for the asset you are trying to access.
        Baked,

        //! \brief The asset is loaded and pointers are patched using a specific engine representation.
        //! \detail Please take a look at the data representation for the asset you are trying to access.
        Loaded,

        //! \brief The asset is loaded into a system and only contains a runtime handle.
        //! \detail This is particullary useful if you want to have a single system to manage specific assets, for example a texture loader.
        Runtime,
    };

    struct AssetHandle;

    struct Asset_v2
    {
        ice::AssetHandle* handle;
        ice::AssetState state;
        ice::Data data;
    };

} // namespace ice
