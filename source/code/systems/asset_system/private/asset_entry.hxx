/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/asset.hxx>
#include <ice/asset_types.hxx>
#include <ice/asset_request.hxx>
#include <ice/mem_memory.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/task.hxx>
#include <ice/task_queue.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/assert.hxx>

#include "asset_data.hxx"
#include "asset_types_internal.hxx"

namespace ice
{

    class Resource;
    class AssetRequestAwaitable;
    struct AssetRequestResolver;
    struct AssetStateTransaction;

    struct ResourceHandle;

    struct AssetHandle { };

    struct AssetStateTrackers
    {
        ice::TaskQueue tasks_queue;
        std::atomic<ice::u8> raw_awaiting;
        std::atomic<ice::u8> bake_awaiting;
        std::atomic<ice::u8> load_awaiting;
        std::atomic<ice::u8> runtime_awaiting;
    };

    struct AssetEntryBase : AssetHandle
    {
        inline AssetEntryBase() noexcept;
        inline AssetEntryBase(ice::StringID_Hash id, ice::ResourceHandle* resource, ice::AssetShelve* shelve) noexcept;
        inline AssetEntryBase(ice::AssetEntryBase&& other) noexcept;

        inline auto data_for_state(ice::AssetState state) noexcept -> ice::Data;

        inline auto state() const noexcept -> ice::AssetState
        {
            return _data == nullptr ? AssetState::Exists : _data->_state;
        }

        ice::StringID assetid;
        ice::ResourceHandle* resource_handle;
        std::atomic<ice::u32> refcount;
        ice::AssetShelve* _shelve;

        // Data pointers
        ice::UniquePtr<ice::AssetData> _metadata;
        ice::UniquePtr<ice::AssetData> _data;

        // Load only object
        std::atomic<ice::AssetStateTrackers*> _request_trackers;
    };

    inline AssetEntryBase::AssetEntryBase() noexcept
        : assetid{ }
        , resource_handle{ nullptr }
        , refcount{ 0 }
        , _shelve{ nullptr }
        , _metadata{ }
        , _data{ }
        , _request_trackers{ }
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
        , _shelve{ shelve }
        , _metadata{ }
        , _data{ }
        , _request_trackers{ }
    {
    }

    inline AssetEntryBase::AssetEntryBase(AssetEntryBase&& other) noexcept
        : assetid{ other.assetid }
        , resource_handle{ other.resource_handle }
        , refcount{ other.refcount.load(std::memory_order_relaxed) }
        , _shelve{ other._shelve }
        , _metadata{ ice::move(other._metadata) }
        , _data{ ice::move(other._data) }
        , _request_trackers{ other._request_trackers.load(std::memory_order_relaxed) }
    {

    }

    inline auto AssetEntryBase::data_for_state(ice::AssetState state) noexcept -> ice::Data
    {
        return ice::asset_data_find(_data, state);
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


} // namespace ice
