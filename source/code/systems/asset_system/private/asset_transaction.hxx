#pragma once
#include "asset_shelve.hxx"
#include "asset_entry.hxx"

namespace ice
{

    struct AssetStateTransaction
    {
        AssetStateTransaction(
            ice::AssetState target_state,
            ice::Allocator& alloc,
            ice::AssetEntry& asset_entry,
            ice::AssetStateTrackers& asset_trackers
        ) noexcept
            : target_state{ target_state }
            , alloc{ alloc }
            , asset{ asset_entry }
            , shelve{ *asset_entry._shelve }
            , trackers{ asset_trackers }
        {
        }

        auto calculate_state(
            ice::AssetState min_state,
            ice::Metadata const& meta
        ) const noexcept -> ice::AssetState;

        void set_result_data(
            ice::Allocator& alloc,
            ice::AssetState min_state,
            ice::Metadata const& meta,
            ice::Memory data
        ) noexcept;

        void set_result_data(ice::Allocator& alloc, ice::AssetState state, ice::AssetResolveData const& resolved_data) noexcept;
        void set_result_data(ice::Allocator& alloc, ice::AssetState state, ice::Memory data) noexcept;
        void set_result_data(ice::AssetState state, ice::Data data) noexcept;


        ice::AssetState const target_state;
        ice::Allocator& alloc;
        ice::AssetEntry& asset;
        ice::AssetShelve& shelve;
        ice::AssetStateTrackers& trackers;

        ice::AssetRequestAwaitable* request_awaitable = nullptr;
    };

    static constexpr ice::ErrorCode E_RequestEvaluationNeeded{ "E.4400:Assets:Request needs evaluation." };
    static constexpr ice::ErrorCode E_RequestEvaluationFailed{ "E.4401:Assets:Request evaluation failed." };

    struct AssetStateRequest : ice::TaskAwaitableBase
    {
        AssetStateRequest(
            ice::AssetStateTransaction& transaction,
            ice::AssetState requested_state
        ) noexcept
            : TaskAwaitableBase{
                ._params = {
                    .modifier = TaskAwaitableModifier::CustomValue,
                    .u32_value = static_cast<ice::u32>(requested_state)
                }
            }
            , _transaction{ transaction }
            , awaiting_index{ 0 }
        {
        }

        bool await_ready() noexcept
        {
            ice::AssetState const accessible_state = _transaction.asset.state();
            ice::AssetState const requested_state = static_cast<ice::AssetState>(_params.u32_value);

            ice::AssetState const state = static_cast<ice::AssetState>(_params.u32_value);
            std::atomic<ice::u8>* counter = nullptr;
            switch (state)
            {
            case AssetState::Raw:
                counter = &_transaction.trackers.raw_awaiting;
                break;
            case AssetState::Baked:
                counter = &_transaction.trackers.bake_awaiting;
                break;
            case AssetState::Loaded:
                counter = &_transaction.trackers.load_awaiting;
                break;
            case AssetState::Runtime:
                counter = &_transaction.trackers.runtime_awaiting;
                break;
            default:
                ICE_ASSERT(false, "Invalid state!");
            }

            awaiting_index = counter->fetch_add(1, std::memory_order_relaxed);
            if (accessible_state >= requested_state)
            {
                counter->fetch_sub(1, std::memory_order_relaxed);

                awaiting_index = ice::u32_max;
            }
            return accessible_state >= requested_state || awaiting_index == 0;
        }

        auto await_suspend(ice::coroutine_handle<> coroutine) noexcept
        {
            _coro = coroutine;
            _transaction.trackers.tasks_queue.push_back(this);
        }

        auto await_resume() const noexcept -> ice::Expected<ice::Data, ice::ErrorCode>
        {
            ice::AssetState const accessible_state = _transaction.asset.state();
            ice::AssetState const requested_state = static_cast<ice::AssetState>(_params.u32_value);

            if (awaiting_index == 0)
            {
                return E_RequestEvaluationNeeded;
            }
            else if (accessible_state < requested_state)
            {
                return E_RequestEvaluationFailed;
            }

            return _transaction.asset.data_for_state(requested_state);
        }

        ice::AssetStateTransaction& _transaction;
        ice::u32 awaiting_index;
    };

    inline auto AssetStateTransaction::calculate_state(
        ice::AssetState min_state,
        ice::Metadata const& meta
    ) const noexcept -> ice::AssetState
    {
        ice::AssetState const calculated_state = asset._shelve->definition.fn_asset_state(
            asset._shelve->definition.ud_asset_state,
            asset._shelve->definition,
            meta,
            ice::resource_uri(ice::asset_data_resource(asset._data))
        );
        return ice::max(calculated_state, min_state);
    }

    inline void AssetStateTransaction::set_result_data(
        ice::Allocator& dataalloc,
        ice::AssetState min_state,
        ice::Metadata const& meta,
        ice::Memory data
    ) noexcept
    {
        ice::AssetState const state = calculate_state(min_state, meta);

        // Assign new metadata
        asset._metadata = ice::create_asset_data_entry(alloc, AssetState::Baked, alloc, ice::meta_save(meta, alloc));
        asset._metadata->_flags |= AssetDataFlags::Metadata;

        // Append the next asset data block
        ice::UniquePtr<ice::AssetData> entry_data = ice::create_asset_data_entry(alloc, state, dataalloc, data);
        entry_data->_next = ice::move(asset._data);
        asset._data = ice::move(entry_data);
    }

    inline void AssetStateTransaction::set_result_data(
        ice::Allocator& dataalloc,
        ice::AssetState state,
        ice::AssetResolveData const& resolved_data
    ) noexcept
    {
        ice::UniquePtr<ice::AssetData> entry_data = ice::create_asset_data_entry(alloc, state, dataalloc, resolved_data.memory);
        entry_data->_next = ice::move(asset._data);
        asset._data = ice::move(entry_data);
    }

    inline void AssetStateTransaction::set_result_data(
        ice::Allocator& dataalloc,
        ice::AssetState state,
        ice::Memory memory
    ) noexcept
    {
        ice::UniquePtr<ice::AssetData> entry_data = ice::create_asset_data_entry(alloc, state, dataalloc, memory);
        entry_data->_next = ice::move(asset._data);
        asset._data = ice::move(entry_data);
    }

    inline void AssetStateTransaction::set_result_data(
        ice::AssetState state,
        ice::Data data
    ) noexcept
    {
        ice::UniquePtr<ice::AssetData> entry_data = ice::create_asset_data_entry(alloc, state, data);
        entry_data->_next = ice::move(asset._data);
        asset._data = ice::move(entry_data);
    }

} // namespace ice