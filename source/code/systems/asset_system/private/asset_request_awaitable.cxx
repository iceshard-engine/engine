/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_request_awaitable.hxx"
#include "asset_transaction.hxx"
#include "asset_entry.hxx"

#include <ice/task_utils.hxx>
#include <ice/assert.hxx>

namespace ice
{

    AssetRequestAwaitable::AssetRequestAwaitable(
        ice::StringID_Arg asset_name,
        ice::AssetStateTransaction& transaction
    ) noexcept
        : _next{ nullptr }
        , _prev{ nullptr }
        , _chained{ nullptr }
        , _asset_name{ asset_name }
        , _asset_shelve{ transaction.shelve }
        , _transaction{ transaction }
        , _result_data{ }
        , _coroutine{ nullptr }
    {
        // TODO: Might require atomic exchange to always work properly
        if (transaction.request_awaitable == nullptr)
        {
            transaction.request_awaitable = this;
        }
        else
        {
            // TODO: Might require atomic exchange to always work properly
            ice::AssetRequestAwaitable* last_request = transaction.request_awaitable;
            while (last_request->_chained != nullptr)
            {
                last_request = last_request->_chained;
            }

            last_request->_chained = this;
        }
    }

    auto AssetRequestAwaitable::state() const noexcept -> ice::AssetState
    {
        return _transaction.asset.state();
    }

    auto AssetRequestAwaitable::data() const noexcept -> ice::Data
    {
        return _transaction.asset.data_for_state(this->state());
    }

    auto AssetRequestAwaitable::metadata() const noexcept -> ice::Data
    {
        ice::Data data{};
        ice::Result result = ice::wait_for_result(ice::asset_metadata_find(_transaction.asset._data, data));
        ICE_LOG_IF(result == E_Fail, LogSeverity::Error, LogTag::Asset, "Failed to access asset metadata: {}", result.error());
        return data;
    }

    auto AssetRequestAwaitable::asset_name() const noexcept -> ice::StringID_Arg
    {
        return _transaction.asset._identifier;
    }

    auto AssetRequestAwaitable::asset_definition() const noexcept -> ice::AssetCategoryDefinition const&
    {
        return _asset_shelve.definition;
    }

    // auto AssetRequestAwaitable::resource() const noexcept -> ice::Resource const&
    // {
    //     return *_transaction.asset.resource;
    // }

    auto AssetRequestAwaitable::allocate(ice::usize size) const noexcept -> ice::Memory
    {
        return _asset_shelve.asset_allocator().allocate({ size, ice::ualign::b_default });
    }

    auto AssetRequestAwaitable::resolve(
        ice::AssetResolveData resolve_data
    ) noexcept -> ice::Asset
    {
        ice::Asset asset_handle{ };

        if (resolve_data.result != AssetRequestResult::Success)
        {
            _asset_shelve.asset_allocator().deallocate(resolve_data.memory);
            _result_data = { };
        }
        else
        {
            ICE_ASSERT_CORE(resolve_data.resolver != nullptr);
            _transaction.set_result_data(_asset_shelve.asset_allocator(), _transaction.target_state, resolve_data);
            _result_data = resolve_data.memory;

            _transaction.asset._refcount.fetch_add(1, std::memory_order_release);
            asset_handle = Asset{ ice::addressof(_transaction.asset) };
        }

        // After the coroutine finishes the request awaitable might be already dead.
        //  So we need to store some variables in local scope.
        ice::Memory result_data = _result_data;
        ice::AssetRequestAwaitable* chained = _chained;

        if (_transaction.request_awaitable == this)
        {
            _transaction.request_awaitable = nullptr;
        }

        _coroutine.resume(); // Introducing, dead 'this' pointer!

        while (chained != nullptr)
        {
            auto coro = chained->_coroutine;
            chained->_result_data = result_data;
            chained = chained->_chained;
            coro.resume(); // same "death" on 'this' applies here
        }

        return asset_handle;
    }

    void AssetRequestAwaitable::await_suspend(std::coroutine_handle<void> coro) noexcept
    {
        _coroutine = coro;

        if (_transaction.request_awaitable == this)
        {
            // Only append the top most request to the shelve.
            _asset_shelve.append_request(this, _transaction.target_state);
        }
    }

    auto AssetRequestAwaitable::await_resume() const noexcept -> ice::Memory
    {
        return _result_data;
    }

} // namespace ice
