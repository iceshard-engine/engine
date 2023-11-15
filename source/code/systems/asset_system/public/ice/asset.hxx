/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/stringid.hxx>
#include <ice/resource_meta.hxx>
#include <ice/asset_types.hxx>

namespace ice
{

    struct Metadata;
    struct AssetHandle;

    enum class AssetState : ice::u32
    {
        //! \brief The asset could not be accessed.
        //! \detail Either an error occured or the asset does not exist.
        Invalid,

        //! \brief The asset data state is not known and needs another pass to the Asset Type Resolver.
        Unknown,

        //! \bried The resource handle associated exists, but the data was not requested not validated.
        Exists,

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

    struct Asset
    {
        ice::AssetHandle* _handle = nullptr;

        Asset() noexcept = default;
        ~Asset() noexcept;
        Asset(Asset&& other) noexcept = default;
        auto operator=(Asset&& other) noexcept -> ice::Asset& = default;

        Asset(Asset const&) noexcept = delete;
        auto operator=(Asset const&) noexcept -> ice::Asset& = delete;

        bool valid() const noexcept;
        auto metadata(ice::Metadata& out_meta) const noexcept -> ice::Task<ice::Result>;
        bool available(ice::AssetState state) const noexcept;
        auto preload(ice::AssetState state) noexcept -> ice::Task<bool>;
        auto data(ice::AssetState state) noexcept -> ice::Task<ice::Data>;

        auto operator[](ice::AssetState state) noexcept -> ice::Task<ice::Data>;
    };

} // namespace ice
