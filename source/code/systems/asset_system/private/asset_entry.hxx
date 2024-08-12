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

    struct AssetStateTrackers
    {
        ice::TaskQueue tasks_queue;
        std::atomic<ice::u8> raw_awaiting;
        std::atomic<ice::u8> bake_awaiting;
        std::atomic<ice::u8> load_awaiting;
        std::atomic<ice::u8> runtime_awaiting;
    };

    struct AssetHandle
    {
        inline AssetHandle() noexcept;
        inline AssetHandle(
            ice::StringID_Hash id,
            ice::AssetShelve* shelve,
            ice::UniquePtr<ice::ResourceAssetData> resource
        ) noexcept;
        inline AssetHandle(ice::AssetHandle&& other) noexcept;

        inline auto data_for_state(ice::AssetState state) noexcept -> ice::Data;

        inline auto state() const noexcept -> ice::AssetState
        {
            return _data == nullptr ? AssetState::Exists : _data->_state;
        }

        // State
        ice::StringID _identifier;
        ice::AssetShelve* _shelve;
        std::atomic<ice::u32> _refcount;

        // Data
        ice::UniquePtr<ice::AssetData> _metadata;
        ice::UniquePtr<ice::AssetData> _data;

        // Loading
        std::atomic<ice::AssetStateTrackers*> _request_trackers;
    };

    inline AssetHandle::AssetHandle() noexcept
        : _identifier{ }
        , _shelve{ nullptr }
        , _refcount{ 0 }
        , _metadata{ }
        , _data{ }
        , _request_trackers{ }
    {
    }

    inline AssetHandle::AssetHandle(
        ice::StringID_Hash id,
        ice::AssetShelve* shelve,
        ice::UniquePtr<ice::ResourceAssetData> resource
    ) noexcept
        : _identifier{ id }
        , _shelve{ shelve }
        , _refcount{ 0 }
        , _metadata{ }
        , _data{ ice::move(resource) }
        , _request_trackers{ }
    {
    }

    inline AssetHandle::AssetHandle(AssetHandle&& other) noexcept
        : _identifier{ other._identifier }
        , _shelve{ other._shelve }
        , _refcount{ other._refcount.load(std::memory_order_relaxed) }
        , _metadata{ ice::move(other._metadata) }
        , _data{ ice::move(other._data) }
        , _request_trackers{ other._request_trackers.load(std::memory_order_relaxed) }
    {

    }

    inline auto AssetHandle::data_for_state(ice::AssetState state) noexcept -> ice::Data
    {
        return ice::asset_data_find(_data, state);
    }

    template<bool IsDebug = true>
    struct AssetEntryFinal : AssetHandle
    {
        static constexpr bool HoldsDebugData = IsDebug;

        inline AssetEntryFinal() noexcept;
        inline AssetEntryFinal(
            ice::HeapString<> name,
            ice::AssetShelve* shelve,
            ice::UniquePtr<ice::ResourceAssetData> resource
        ) noexcept;
        inline AssetEntryFinal(AssetEntryFinal const& other) noexcept;

        inline static ice::StackAllocator<8_B> _empty_alloc;
        ice::HeapString<> debug_name;
    };

    template<>
    struct AssetEntryFinal<false> : AssetHandle
    {
        static constexpr bool HoldsDebugData = false;

        using AssetHandle::AssetHandle;
    };

    template<bool IsDebug>
    inline AssetEntryFinal<IsDebug>::AssetEntryFinal() noexcept
        : AssetHandle{ }
        , debug_name{ _empty_alloc }
    {
    }

    template<bool IsDebug>
    inline AssetEntryFinal<IsDebug>::AssetEntryFinal(
        ice::HeapString<> name,
        ice::AssetShelve* shelve,
        ice::UniquePtr<ice::ResourceAssetData> resource
    ) noexcept
        : AssetHandle{ ice::stringid(name), shelve, ice::move(resource) }
        , debug_name{ ice::move(name) }
    {
    }

    template<bool IsDebug>
    inline AssetEntryFinal<IsDebug>::AssetEntryFinal(ice::AssetEntryFinal<IsDebug> const& other) noexcept
        : AssetHandle{ other }
        , debug_name{ other.debug_name } // copy? Might want to remove it
    {
        _identifier = ice::stringid(debug_name);
    }


    using AssetEntry = AssetEntryFinal<ice::build::is_debug || ice::build::is_develop>;

} // namespace ice
