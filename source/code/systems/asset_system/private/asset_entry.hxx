/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset.hxx>
#include <ice/asset_types.hxx>
#include <ice/mem_memory.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/assert.hxx>

namespace ice
{

    class Resource;
    class AssetRequestAwaitable;
    struct AssetRequestResolver;

    struct ResourceHandle;

    struct AssetHandle { };

    struct AssetEntryBase : AssetHandle
    {
        inline AssetEntryBase() noexcept;
        inline AssetEntryBase(ice::StringID_Hash id, ice::ResourceHandle* resource, ice::AssetShelve* shelve) noexcept;
        inline AssetEntryBase(ice::AssetEntryBase const& other) noexcept;

        inline auto data_for_state(ice::AssetState state) noexcept -> ice::Data;

        ice::StringID assetid;
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
        ice::Memory metadata_baked;

        ice::AssetRequestAwaitable* request_awaitable;
        ice::AssetRequestResolver* request_resolver;
    };

    inline AssetEntryBase::AssetEntryBase() noexcept
        : resource_state{ AssetState::Invalid }
        , current_state{ AssetState::Invalid }
        , request_awaitable{ nullptr }
    {
    }

    inline AssetEntryBase::AssetEntryBase(
        ice::StringID_Hash id,
        ice::ResourceHandle* resource,
        ice::AssetShelve* shelve
    ) noexcept
        : assetid{ id }
        , resource_handle{ resource }
        , refcount{ 0 }
        , shelve{ shelve }
        , resource_state{ AssetState::Unknown }
        , current_state{ AssetState::Unknown }
        , data_baked{ }
        , data_loaded{ }
        , data_runtime{ }
        , metadata_baked{ }
        , request_awaitable{ nullptr }
    {
    }

    inline AssetEntryBase::AssetEntryBase(AssetEntryBase const& other) noexcept
        : assetid{ other.assetid }
        , resource{ other.resource }
        , resource_handle{ other.resource_handle }
        , refcount{ other.refcount.load(std::memory_order_relaxed) }
        , shelve{ other.shelve }
        , resource_state{ other.resource_state }
        , current_state{ other.current_state }
        , data_baked{ other.data_baked }
        , data_loaded{ other.data_loaded }
        , data_runtime{ other.data_runtime }
        , metadata_baked{ other.metadata_baked }
        , request_awaitable{ other.request_awaitable }
    {

    }

    inline auto AssetEntryBase::data_for_state(ice::AssetState state) noexcept -> ice::Data
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

    template<bool IsDebug = true>
    struct AssetEntryFinal : AssetEntryBase
    {
        static constexpr bool HoldsDebugData = IsDebug;

        inline AssetEntryFinal() noexcept;
        inline AssetEntryFinal(ice::HeapString<> name, ice::ResourceHandle* resource, ice::AssetShelve* shelve) noexcept;
        inline AssetEntryFinal(AssetEntryFinal const& other) noexcept;

        inline static ice::StackAllocator<8_B> _empty_alloc;
        ice::HeapString<> debug_name;
    };

    template<>
    struct AssetEntryFinal<false> : AssetEntryBase
    {
        static constexpr bool HoldsDebugData = false;

        using AssetEntryBase::AssetEntryBase;
    };

    template<bool IsDebug>
    inline AssetEntryFinal<IsDebug>::AssetEntryFinal() noexcept
        : AssetEntryBase{ }
        , debug_name{ _empty_alloc }
    {
    }

    template<bool IsDebug>
    inline AssetEntryFinal<IsDebug>::AssetEntryFinal(ice::HeapString<> name, ice::ResourceHandle* resource, ice::AssetShelve* shelve) noexcept
        : AssetEntryBase{ ice::stringid(name), resource, shelve }
        , debug_name{ ice::move(name) }
    {
    }

    template<bool IsDebug>
    inline AssetEntryFinal<IsDebug>::AssetEntryFinal(ice::AssetEntryFinal<IsDebug> const& other) noexcept
        : AssetEntryBase{ other }
        , debug_name{ other.debug_name } // copy? Might want to remove it
    {
        assetid = ice::stringid(debug_name);
    }


    using AssetEntry = AssetEntryFinal<ice::build::is_debug || ice::build::is_develop>;

    struct AssetStateRequest : ice::TaskAwaitableBase
    {
        AssetStateRequest(AssetEntry& entry, ice::AssetState state) noexcept
            : TaskAwaitableBase{
                ._params = {
                    .modifier = TaskAwaitableModifier::CustomValue,
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
            _entry.queue.push_back(this);
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
