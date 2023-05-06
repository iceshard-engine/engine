/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_request_awaitable.hxx"
#include "asset_shelve.hxx"
#include "asset_entry.hxx"

#include <ice/assert.hxx>

namespace ice
{

    AssetRequestAwaitable::AssetRequestAwaitable(
        ice::StringID_Arg asset_name,
        ice::AssetShelve& shelve,
        ice::AssetEntry* entry,
        ice::AssetState requested_state
    ) noexcept
        : _next{ nullptr }
        , _prev{ nullptr }
        , _chained{ nullptr }
        , _asset_name{ asset_name }
        , _asset_shelve{ shelve }
        , _asset_entry{ entry }
        , _requested_state{ requested_state }
        , _result_data{ }
        , _coroutine{ nullptr }
    {
        // TODO: Might require atomic exchange to always work properly
        if (entry->request_awaitable == nullptr)
        {
            entry->request_awaitable = this;
        }
        else
        {
            // TODO: Might require atomic exchange to always work properly
            ice::AssetRequestAwaitable* last_request = entry->request_awaitable;
            while (last_request->_chained != nullptr)
            {
                last_request = last_request->_chained;
            }

            last_request->_chained = this;
        }
    }

    auto AssetRequestAwaitable::state() const noexcept -> ice::AssetState
    {
        return _asset_entry->current_state;
    }

    auto AssetRequestAwaitable::data() const noexcept -> ice::Data
    {
        switch (_requested_state)
        {
        case AssetState::Baked:
            return _asset_entry->data;
        case AssetState::Loaded:
            return _asset_entry->data_baked.location != nullptr ? ice::data_view(_asset_entry->data_baked) : _asset_entry->data;
        case AssetState::Runtime:
            return ice::data_view(_asset_entry->data_loaded);
        default:
            ICE_ASSERT(false, "Required request data not available!");
        }
        return { };
    }

    auto AssetRequestAwaitable::resource() const noexcept -> ice::Resource const&
    {
        return *_asset_entry->resource;
    }

    auto AssetRequestAwaitable::asset_definition() const noexcept -> ice::AssetTypeDefinition const&
    {
        return _asset_shelve.definition;
    }

    auto AssetRequestAwaitable::allocate(ice::usize size) const noexcept -> ice::Memory
    {
        return _asset_shelve.asset_allocator().allocate({ size, ice::ualign::b_default });
    }

    auto AssetRequestAwaitable::resolve(
        ice::AssetRequest::Result result,
        ice::Memory memory
    ) noexcept -> ice::Asset
    {
        ice::Asset asset_handle{ };

        if (result != AssetRequest::Result::Success)
        {
            _asset_shelve.asset_allocator().deallocate(memory);
            _result_data = { };
        }
        else
        {
            _result_data = memory;
            asset_handle._handle = _asset_entry;
        }

        // After the coroutine finishes the request awaitable might be already dead.
        //  So we need to store some variables in local scope.
        ice::Memory result_data = _result_data;
        ice::AssetRequestAwaitable* chained = _chained;

        if (_asset_entry->request_awaitable == this)
        {
            _asset_entry->request_awaitable = nullptr;
        }

        _coroutine.resume(); // Introducing, dead 'this' pointer!

        while (chained != nullptr)
        {
            auto coro = chained->_coroutine;
            chained->_result_data = result_data;
            chained = chained->_chained;
            coro.resume(); // same death applies here
        }

        return asset_handle;
    }

    void AssetRequestAwaitable::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _coroutine = coro;

        if (_asset_entry->request_awaitable == this)
        {
            // Only append the top most request to the shelve.
            _asset_shelve.append_request(this, _requested_state);
        }
    }

    auto AssetRequestAwaitable::await_resume() const noexcept -> ice::Memory
    {
        return _result_data;
    }

} // namespace ice
