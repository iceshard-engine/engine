/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset_types.hxx>
#include <ice/asset_errors.hxx>
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
        Asset(Asset const&) noexcept;
        auto operator=(Asset const&) noexcept -> ice::Asset&;

        bool valid() const noexcept;
        bool empty() const noexcept;

        void release() noexcept;

        auto uri() const noexcept -> ice::URI;
        auto name() const noexcept -> ice::StringID_Arg;
        auto metadata(ice::Data& out_metadata) const noexcept -> ice::Task<ice::Result>;
        bool available(ice::AssetState state) const noexcept;
        auto resource() const noexcept -> ice::Resource const*;

        auto preload(this ice::Asset self, ice::AssetState state) noexcept -> ice::Task<>;
        auto data(this ice::Asset self, ice::AssetState state) noexcept -> ice::TaskExpected<ice::Data>;

        auto operator[](this ice::Asset self, ice::AssetState state) noexcept -> ice::TaskExpected<ice::Data>;

        template <typename T>
        auto object(this ice::Asset self, ice::AssetState state = AssetState::Runtime) noexcept -> ice::TaskExpected<T const*>;

    public:
        auto start_transaction(
            ice::AssetState state,
            ice::AssetStateTrackers& trackers
        ) const noexcept -> ice::AssetStateTransaction;

        void finish_transaction(ice::AssetStateTransaction& transaction) const noexcept;
    };

    template<typename T>
    auto Asset::object(this ice::Asset self, ice::AssetState state) noexcept -> ice::TaskExpected<T const*>
    {
        ice::Data data = co_await self[state];
        if (data.location == nullptr)
        {
            ice::u8 const error_result_idx = std::to_underlying(state) - std::to_underlying(AssetState::Raw);
            co_return E_AssetDataNotFound[error_result_idx];
        }
        else if (data.size != ice::size_of<T> || data.alignment < ice::align_of<T>)
        {
            co_return E_AssetInvalidObjectData;
        }
        co_return static_cast<T const*>(data.location);
    }

} // namespace ice
