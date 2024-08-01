/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset.hxx>
#include <ice/asset_storage.hxx>
#include <ice/resource.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_utils.hxx>

#include "asset_entry.hxx"
#include "asset_shelve.hxx"

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
            return static_cast<ice::AssetEntry*>(handle)->_shelve->storage;
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
            return entry.state() >= AssetState::Exists;
        }
        return false;
    }

    auto Asset::metadata(ice::Data& out_metadata) const noexcept -> ice::Task<ice::Result>
    {
        ice::AssetEntry& entry = detail::entry(_handle);
        if (entry._metadata != nullptr)
        {
            out_metadata = ice::asset_data_find(entry._metadata, AssetState::Exists);
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

    auto Asset::preload(ice::AssetState state) noexcept -> ice::Task<bool>
    {
        ice::Data const result = co_await data(state);
        co_return result.location != nullptr;
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
