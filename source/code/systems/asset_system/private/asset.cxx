/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/asset.hxx>
#include <ice/asset_storage.hxx>
#include <ice/resource.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_utils.hxx>

#include "asset_entry.hxx"
#include "asset_shelve.hxx"
#include "asset_transaction.hxx"
#include "asset_storage.hxx"

namespace ice
{

    Asset::Asset(ice::AssetHandle* handle) noexcept
        : _handle{ handle }
    {
    }

    Asset::~Asset() noexcept
    {
        this->release();
    }

    Asset::Asset(Asset&& other) noexcept
        : _handle{ ice::exchange(other._handle, nullptr) }
    {
    }

    auto Asset::operator=(Asset&& other) noexcept -> ice::Asset&
    {
        if (this != ice::addressof(other))
        {
            _handle = ice::exchange(other._handle, nullptr);
        }
        return *this;
    }

    auto Asset::name() const noexcept -> ice::StringID_Arg
    {
        return _handle->_identifier;
    }

    bool Asset::valid() const noexcept
    {
        return _handle != nullptr;
    }

    bool Asset::empty() const noexcept
    {
        return valid() == false;
    }

    void Asset::release() const noexcept
    {
        if (this->empty())
        {
            return;
        }

        ice::u32 const oldcount = _handle->_refcount.fetch_sub(1, std::memory_order_relaxed);
        if (oldcount == 1)
        {
            // Release all data and metadata loaded
            _handle->_data.reset();

            // Unload the resource (if not yet done yet)
            // ice::wait_for(_info.resource_tracker.unload_resource(entry->_resource));
        }
    }

    auto Asset::metadata(ice::Data& out_metadata) const noexcept -> ice::Task<ice::Result>
    {
        co_return co_await ice::asset_metadata_find(_handle->_data, out_metadata);
    }

    bool Asset::available(ice::AssetState state) const noexcept
    {
        return _handle->data_for_state(state).location != nullptr;
    }

    auto Asset::preload(ice::AssetState state) noexcept -> ice::Task<>
    {
        co_await data(state);
    }

    auto Asset::data(ice::AssetState state) noexcept -> ice::Task<ice::Data>
    {
        co_return co_await _handle->_shelve->storage.request(*this, state);
    }

    auto Asset::operator[](ice::AssetState state) noexcept -> ice::Task<ice::Data>
    {
        ICE_ASSERT(_handle != nullptr, "Invalid Asset object!");
        co_return co_await data(state);
    }

    auto Asset::start_transaction(
        ice::AssetState state,
        ice::AssetStateTrackers& trackers
    ) const noexcept -> ice::AssetStateTransaction
    {
        // This object might be unused (it's only alive for the request duration)
        ice::AssetStateTrackers* trackersptrs = nullptr;
        if (_handle->_request_trackers.compare_exchange_strong(trackersptrs, ice::addressof(trackers), std::memory_order_relaxed))
        {
            trackersptrs = ice::addressof(trackers);
        }

        return ice::AssetStateTransaction{
            state, _handle->_shelve->asset_allocator(), static_cast<ice::AssetEntry&>(*_handle), *trackersptrs
        };
    }

    void Asset::finish_transaction(ice::AssetStateTransaction& transaction) const noexcept
    {
        // TODO: Check no other asset requests are awaiting completion.

        // Release the trackers pointers
        if (_handle->_request_trackers.load(std::memory_order_relaxed) == ice::addressof(transaction.trackers))
        {
            _handle->_request_trackers.exchange(nullptr, std::memory_order_relaxed);
        }
    }

} // namespace ice
