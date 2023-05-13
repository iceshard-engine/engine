/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset.hxx>
#include <ice/asset_types.hxx>
#include <ice/mem_memory.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/assert.hxx>

namespace ice
{

    class Resource;
    class AssetRequestAwaitable;

    struct ResourceHandle;

    struct AssetHandle { };

    struct AssetEntry : AssetHandle
    {
        inline AssetEntry() noexcept;
        inline AssetEntry(ice::ResourceHandle* resource, ice::AssetShelve* shelve) noexcept;
        inline AssetEntry(ice::AssetEntry const& other) noexcept;

        inline auto data_for_state(ice::AssetState state) noexcept -> ice::Data;

        ice::Resource const* resource;
        ice::ResourceHandle* resource_handle;
        std::atomic<ice::u32> refcount;

        ice::TaskQueue queue;
        std::atomic<ice::u8> raw_awaiting;
        std::atomic<ice::u8> bake_awaiting;
        std::atomic<ice::u8> load_awaiting;
        std::atomic<ice::u8> runtime_awaiting;

        ice::AssetStorage* storage;
        ice::AssetShelve* shelve;
        ice::AssetState resource_state;
        ice::AssetState current_state;
        ice::Data data;
        ice::Memory data_baked;
        ice::Memory data_loaded;
        ice::Memory data_runtime;

        ice::AssetRequestAwaitable* request_awaitable;
    };

    inline AssetEntry::AssetEntry() noexcept
        : resource_state{ AssetState::Invalid }
        , current_state{ AssetState::Invalid }
        , request_awaitable{ nullptr }
    {
    }

    inline AssetEntry::AssetEntry(ice::ResourceHandle* resource, ice::AssetShelve* shelve) noexcept
        : resource_handle{ resource }
        , refcount{ 0 }
        , shelve{ shelve }
        , resource_state{ AssetState::Unknown }
        , current_state{ AssetState::Unknown }
        , data_baked{ }
        , data_loaded{ }
        , data_runtime{ }
        , request_awaitable{ nullptr }
    {
    }

    inline AssetEntry::AssetEntry(AssetEntry const& other) noexcept
        : resource{ other.resource }
        , resource_handle{ other.resource_handle }
        , refcount{ other.refcount.load(std::memory_order_relaxed) }
        , shelve{ other.shelve }
        , resource_state{ other.resource_state }
        , current_state{ other.current_state }
        , data_baked{ other.data_baked }
        , data_loaded{ other.data_loaded }
        , data_runtime{ other.data_runtime }
        , request_awaitable{ other.request_awaitable }
    {

    }

    inline auto AssetEntry::data_for_state(ice::AssetState state) noexcept -> ice::Data
    {
        switch (state)
        {
        case AssetState::Raw:
            // If the resource state is higher that 'raw' we cannot satisfy this request.
            return resource_state == AssetState::Raw ? data : Data{ };
        case AssetState::Baked:
            // If the resource state is 'baked' then we return 'data' instead.
            return resource_state == AssetState::Baked ? data : ice::data_view(data_baked);
        case AssetState::Loaded:
            return ice::data_view(data_loaded);
        case AssetState::Runtime:
            return ice::data_view(data_runtime);
        default:
            ICE_ASSERT(false, "Invalid state!");
        }
        return { };
    }

    struct AssetStateRequest : ice::TaskAwaitableBase
    {
        AssetStateRequest(AssetEntry& entry, ice::AssetState state) noexcept
            : TaskAwaitableBase{
                ._params = {
                    .modifier = TaskAwaitableModifier_v3::CustomValue,
                    .u32_value = static_cast<ice::u32>(state)
                }
            }
            , _entry{ entry }
        {
        }

        bool await_ready() noexcept
        {
            ice::AssetState const state = static_cast<ice::AssetState>(_params.u32_value);
            std::atomic<ice::u8>* counter = nullptr;
            switch (state)
            {
            case AssetState::Raw:
                counter = &_entry.raw_awaiting;
                break;
            case AssetState::Baked:
                counter = &_entry.bake_awaiting;
                break;
            case AssetState::Loaded:
                counter = &_entry.load_awaiting;
                break;
            case AssetState::Runtime:
                counter = &_entry.runtime_awaiting;
                break;
            default:
                ICE_ASSERT(false, "Invalid state!");
            }

            // Ensure we will be handled either way
            awaiting_index = counter->fetch_add(1, std::memory_order_relaxed);

            bool const is_ready = _entry.current_state >= static_cast<ice::AssetState>(_params.u32_value);
            if (is_ready)
            {
                // We got just after the data was published, reduce awaiting and continue with data
                counter->fetch_sub(1, std::memory_order_relaxed);

                // We set awaiting to max, which indicates that we where ready to proceed.
                awaiting_index = ice::u32_max;
            }
            return is_ready || awaiting_index == 0;
        }

        auto await_suspend(ice::coroutine_handle<> coroutine) noexcept
        {
            _coro = coroutine;

            // The thread loading the data will now handle us
            ice::linked_queue::push(_entry.queue._awaitables, this);
        }

        auto await_resume() const noexcept -> ice::Data
        {
            ice::AssetState const state = static_cast<ice::AssetState>(_params.u32_value);
            ICE_ASSERT(_entry.current_state >= state || awaiting_index == 0, "Resuming when state is invalid!");
            return _entry.data_for_state(state);
        }

        ice::u32 awaiting_index;
        ice::AssetEntry& _entry;
    };

} // namespace ice
