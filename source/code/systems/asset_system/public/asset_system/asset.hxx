#pragma once
#include <core/base.hxx>
#include <core/data/view.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/string_view.hxx>
#include <resource/resource_meta.hxx>

namespace asset
{

    //! \brief This enumeration defines all supported assert types.
    enum class AssetType : uint32_t
    {
        Invalid = 0x0,
        Resource,
        Binary,
        Config,

        Mesh,
        Shader,
        Texture,

        //! \brief Used during asset resolving.
        Unresolved = 0xffff'ffff,
    };

    //! \brief This enumeration defines all stages an asset can have.
    enum class AssetStatus : uint32_t
    {
        Invalid = 0x0,
        Available,
        Available_Raw,
        Requested,
        Compiling,
        Loading,
        Loaded,
        Unused,
        Unloading,
    };

    //! \brief This struct describes a single asset reference.
    struct Asset
    {
        Asset() noexcept = default;

        constexpr Asset(core::stringid_arg_type asset_name, AssetType type) noexcept
            : type{ type }
            , name{ asset_name }
        { }

        constexpr Asset(core::StringView asset_name, AssetType type) noexcept
            : type{ type }
            , name{ core::stringid(asset_name) }
        { }

        //! \brief The asset type.
        AssetType type = AssetType::Invalid;

        //! \brief The asset name.
        core::stringid_type name = core::stringid_invalid;
    };

    //! \brief This struct describes a single asset data.
    //!
    //! \details Each asset is just a raw data buffer which with an associated metadata buffer.
    //!     The assets metadata should always be valid and contain which describes the given asset.
    //!     The provided metadata can be anything, and the specific asset implementation uses it to validate the content data.
    struct AssetData
    {
        core::data_view content;
        resource::ResourceMetaView metadata = resource::load_meta_view({});
    };

} // namespace asset
