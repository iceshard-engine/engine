/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset.hxx>
#include <ice/asset_storage.hxx>
#include <ice/resource.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_utils.hxx>

#include "asset_entry.hxx"

namespace ice
{

    namespace detail
    {

        auto entry(ice::AssetHandle* handle) noexcept -> ice::AssetEntry&
        {
            return *static_cast<ice::AssetEntry*>(handle);
        }

        auto storage(ice::AssetHandle* handle) noexcept -> ice::AssetStorage&
        {
            return *static_cast<ice::AssetEntry*>(handle)->storage;
        }

    } // namespace detail

    Asset::~Asset() noexcept
    {
        // TODO: Fix loading of asses as this releases memory too early!
        // ice::wait_for(detail::storage(_handle).release(*this));
    }

    auto Asset::name() const noexcept -> ice::StringID_Arg
    {
        ice::AssetEntry const& entry = detail::entry(_handle);
        return entry.assetid;
    }

    bool Asset::valid() const noexcept
    {
        if (_handle != nullptr)
        {
            ice::AssetEntry const& entry = detail::entry(_handle);
            return entry.current_state != AssetState::Invalid && entry.resource_state != AssetState::Unknown;
        }
        return false;
    }

    auto Asset::metadata(ice::Data& out_metadata) const noexcept -> ice::Task<ice::Result>
    {
        ice::AssetEntry& entry = detail::entry(_handle);
        if (entry.metadata_baked.location != nullptr)
        {
            out_metadata = ice::data_view(entry.metadata_baked);
            co_return ice::S_Ok;
        }
        else
        {
            co_return co_await ice::resource_meta(detail::entry(_handle).resource_handle, out_metadata);
        }
    }

    bool Asset::available(ice::AssetState state) const noexcept
    {
        return detail::entry(_handle).data_for_state(state).location != nullptr;
    }

    auto Asset::preload(ice::AssetState state) noexcept -> ice::Task<>
    {
        co_await data(state);
    }

    auto Asset::data(ice::AssetState state) noexcept -> ice::Task<ice::Data>
    {
        co_return co_await detail::storage(_handle).request(*this, state);
    }

    auto Asset::operator[](ice::AssetState state) noexcept -> ice::Task<ice::Data>
    {
        ICE_ASSERT(_handle != nullptr, "Invalid Asset object!");
        co_return co_await data(state);
    }

} // namespace ice
