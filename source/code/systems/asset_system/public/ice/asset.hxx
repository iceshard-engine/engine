/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_types.hxx>
#include <ice/uri.hxx>

namespace ice
{

    enum class AssetState : ice::u8
    {
        //! \brief The asset could not be accessed.
        //! \detail Either an error occured or the asset does not exist.
        Invalid,

        //! \brief The asset data state is not known and needs another pass to the Asset Type Resolver.
        Unknown,

        //! \bried The resource handle associated exists, but the data was not requested nor validated.
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
        explicit Asset(ice::AssetHandle* handle) noexcept;
        ~Asset() noexcept;

        Asset(Asset&& other) noexcept;
        auto operator=(Asset&& other) noexcept -> ice::Asset&;
        Asset(Asset const&) noexcept = delete;
        auto operator=(Asset const&) noexcept -> ice::Asset& = delete;

        bool valid() const noexcept;
        bool empty() const noexcept;

        void release() noexcept;

        auto uri() const noexcept -> ice::URI;
        auto name() const noexcept -> ice::StringID_Arg;
        auto metadata(ice::Data& out_metadata) const noexcept -> ice::Task<ice::Result>;
        bool available(ice::AssetState state) const noexcept;
        auto preload(ice::AssetState state) noexcept -> ice::Task<>;
        auto data(ice::AssetState state) noexcept -> ice::Task<ice::Data>;

        auto operator[](ice::AssetState state) noexcept -> ice::Task<ice::Data>;

    public:
        auto start_transaction(
            ice::AssetState state,
            ice::AssetStateTrackers& trackers
        ) const noexcept -> ice::AssetStateTransaction;

        void finish_transaction(ice::AssetStateTransaction& transaction) const noexcept;
    };

} // namespace ice
