/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "asset_shelve.hxx"
#include "asset_request_awaitable.hxx"

#include <ice/string/heap_string.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/profiler.hxx>
#include <ice/assert.hxx>

namespace ice
{

    AssetShelve::AssetShelve(
        ice::Allocator& alloc,
        ice::AssetTypeDefinition const& definition,
        ice::ResourceCompiler const* compiler
    ) noexcept
        : definition{ definition }
        , compiler{ compiler }
        , _allocator{ alloc }
        , _asset_resources{ alloc }
    {
        ice::hashmap::reserve(_asset_resources, 25);
    }

    AssetShelve::~AssetShelve() noexcept
    {
        for (ice::AssetEntry* entry : _asset_resources)
        {
            _allocator.deallocate(entry->data_baked);
            _allocator.deallocate(entry->data_loaded);
            _allocator.deallocate(entry->data_runtime);
            _allocator.destroy(entry);
        }
    }

    auto AssetShelve::asset_allocator() noexcept -> ice::Allocator&
    {
        return _allocator;
    }

    auto AssetShelve::select(
        ice::StringID_Arg name
    ) noexcept -> ice::AssetEntry*
    {
        return ice::hashmap::get(_asset_resources, ice::hash(name), nullptr);
    }

    auto AssetShelve::select(
        ice::StringID_Arg name
    ) const noexcept -> ice::AssetEntry const*
    {
        ice::u64 const name_hash = ice::hash(name);

        static ice::AssetEntry invalid_resource{ };
        return ice::hashmap::get(_asset_resources, name_hash, &invalid_resource);
    }

    auto AssetShelve::store(
        ice::StringID_Arg name,
        ice::ResourceHandle* resource_handle
    ) noexcept -> ice::AssetEntry*
    {
        if constexpr (ice::AssetEntry::HoldsDebugData)
        {
            ice::HeapString<> asset_name{ _allocator, ice::stringid_hint(name) };

            ice::u64 const name_hash = ice::hash(name);
            ice::hashmap::set(
                _asset_resources,
                name_hash,
                _allocator.create<ice::AssetEntry>(ice::move(asset_name), resource_handle, this)
            );
        }
        else
        {
            ice::u64 const name_hash = ice::hash(name);
            ice::hashmap::set(
                _asset_resources,
                name_hash,
                _allocator.create<ice::AssetEntry>(name, resource_handle, this)
            );
        }

        return select(name);
    }

    void AssetShelve::append_request(
        ice::AssetRequestAwaitable* request,
        ice::AssetState state
    ) noexcept
    {
        ice::u32 const state_base_idx = static_cast<ice::u32>(AssetState::Baked);

        ice::u32 const state_idx = static_cast<ice::u32>(state);
        ICE_ASSERT(state_idx >= state_base_idx, "Invalid state used to apped requset!");

        std::atomic<ice::AssetRequestAwaitable*>& queue = _new_requests[state_idx - state_base_idx];

        ice::AssetRequestAwaitable* expected_head = queue.load(std::memory_order_acquire);

        do
        {
            request->_next = expected_head;
        } while (
            queue.compare_exchange_weak(
                expected_head,
                request,
                std::memory_order_release,
                std::memory_order_acquire
            ) == false
        );
    }

    auto AssetShelve::aquire_request(ice::AssetState state) noexcept -> ice::AssetRequestAwaitable*
    {
        ice::u32 const state_base_idx = static_cast<ice::u32>(AssetState::Baked);

        ice::u32 const state_idx = static_cast<ice::u32>(state);
        ICE_ASSERT(state_idx >= state_base_idx, "Invalid state used to apped requset!");

        std::atomic<ice::AssetRequestAwaitable*>& queue = _new_requests[state_idx - state_base_idx];
        std::atomic<ice::AssetRequestAwaitable*>& reversed_queue = _reversed_requests[state_idx - state_base_idx];

        ice::AssetRequestAwaitable* selected_request = reversed_queue.load(std::memory_order_relaxed);

        bool aquire_success = false;
        while (aquire_success == false && selected_request != nullptr)
        {
            aquire_success = reversed_queue.compare_exchange_weak(
                selected_request,
                selected_request->_prev,
                std::memory_order_release,
                std::memory_order_relaxed
            );
        }

        // If we couldn't aquire new requests
        if (aquire_success == false)
        {
            // Try to get new elements from the "new" request queue
            selected_request = queue.load(std::memory_order_relaxed);
            while (aquire_success == false && selected_request != nullptr)
            {
                aquire_success = queue.compare_exchange_weak(
                    selected_request,
                    nullptr,
                    std::memory_order_relaxed
                );
            };

            // We got a new unreversed head!
            if (selected_request != nullptr)
            {
                // Reverse the order we want to access it.
                ice::AssetRequestAwaitable* last_entry = selected_request;
                ice::AssetRequestAwaitable* first_entry = last_entry;
                while (first_entry->_next != nullptr)
                {
                    first_entry->_next->_prev = first_entry;
                    first_entry = first_entry->_next;
                }


                // Even when we destroy the order in this case. We can live with it, hopefully...
                ice::AssetRequestAwaitable* new_head = reversed_queue.load(std::memory_order_relaxed);
                do
                {
                    last_entry->_prev = new_head;
                } while (
                    reversed_queue.compare_exchange_weak(
                        new_head,
                        first_entry,
                        std::memory_order_relaxed
                    ) == false
                );
            }

            // Final try
            selected_request = reversed_queue.load(std::memory_order_relaxed);

            aquire_success = false;
            while (aquire_success == false && selected_request != nullptr)
            {
                aquire_success = reversed_queue.compare_exchange_weak(
                    selected_request,
                    selected_request->_prev,
                    std::memory_order_release,
                    std::memory_order_relaxed
                );
            }
        }

        return selected_request;
    }

} // namespace ice
