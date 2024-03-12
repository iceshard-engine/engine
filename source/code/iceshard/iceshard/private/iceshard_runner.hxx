/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/engine_state_definition.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_allocator_forward.hxx>
#include <ice/mem_allocator_proxy.hxx>
#include <ice/task.hxx>

namespace ice
{

    static constexpr ice::EngineStateGraph StateGraph_WorldRuntime = "world-runtime"_state_graph;
    static constexpr ice::EngineState State_WorldRuntimeActive = StateGraph_WorldRuntime | "active";
    static constexpr ice::EngineState State_WorldRuntimeInactive = StateGraph_WorldRuntime | "inactive";

    static constexpr ice::ShardID ShardID_WorldRuntimeActivated = "event/world-runtime/activated`ice::StringID_Hash"_shardid;

    class IceshardEngine;

    struct IceshardDataStorage : ice::DataStorage
    {
        ice::HashMap<void*> _values;

        IceshardDataStorage(ice::Allocator& alloc) noexcept
            : _values{ alloc }
        {
        }

        bool set(ice::StringID name, void* value) noexcept override
        {
            ice::u64 const hash = ice::hash(name);
            bool const missing = ice::hashmap::has(_values, hash) == false;
            //if (missing)
            {
                ice::hashmap::set(_values, ice::hash(name), value);
            }
            return missing;
        }

        bool get(ice::StringID name, void*& value) noexcept override
        {
            value = ice::hashmap::get(_values, ice::hash(name), nullptr);
            return value != nullptr;
        }

        bool get(ice::StringID name, void const*& value) const noexcept override
        {
            value = ice::hashmap::get(_values, ice::hash(name), nullptr);
            return value != nullptr;
        }
    };

    struct IceshardFrameData : ice::EngineFrameData
    {
        ice::ProxyAllocator _allocator;
        ice::ForwardAllocator _fwd_allocator;

        ice::IceshardDataStorage& _storage_frame;
        ice::IceshardDataStorage& _storage_runtime;
        ice::IceshardDataStorage const& _storage_persistent;

        ice::u32 _index;

        // Only used for internal purposes
        ice::IceshardFrameData* _internal_next;

        IceshardFrameData(
            ice::Allocator& alloc,
            ice::IceshardDataStorage& frame_storage,
            ice::IceshardDataStorage& runtime_storage,
            ice::IceshardDataStorage& persistent_storage
        ) noexcept
            : _allocator{ alloc, "frame" }
            , _fwd_allocator{ _allocator, "frame-forward", ForwardAllocatorParams{.bucket_size = 16_KiB, .min_bucket_count = 2} }
            , _storage_frame{ frame_storage }
            , _storage_runtime{ runtime_storage }
            , _storage_persistent{ persistent_storage }
            , _index{ }
            , _internal_next{ nullptr }
        { }

        virtual ~IceshardFrameData() noexcept override = default;

        auto frame() noexcept -> ice::DataStorage& override
        {
            return _storage_frame;
        }

        auto runtime() noexcept -> ice::DataStorage& override
        {
            return _storage_runtime;
        }

        auto frame() const noexcept -> ice::DataStorage const& override
        {
            return _storage_frame;
        }

        auto runtime() const noexcept -> ice::DataStorage const& override
        {
            return _storage_runtime;
        }

        auto persistent() const noexcept -> ice::DataStorage const& override
        {
            return _storage_persistent;
        }
    };

    class IceshardEngineRunner
        : public ice::EngineRunner
        , public ice::EngineStateCommitter
    {
    public:
        IceshardEngineRunner(ice::Allocator& alloc, ice::EngineRunnerCreateInfo const& create_info) noexcept;
        ~IceshardEngineRunner() noexcept override;

        void update_states(
            ice::WorldStateTracker& state_tracker,
            ice::WorldStateParams const& update_params
        ) noexcept override;

        auto aquire_frame() noexcept -> ice::Task<ice::UniquePtr<ice::EngineFrame>> override;
        auto update_frame(ice::EngineFrame& current_frame, ice::EngineFrame const& previous_frame) noexcept -> ice::Task<> override;
        void release_frame(ice::UniquePtr<ice::EngineFrame> frame) noexcept override;

        void destroy() noexcept;

    public: // Impl: ice::EngineStateCommiter
        bool commit(
            ice::EngineStateTrigger const& trigger,
            ice::Shard trigger_shard,
            ice::ShardContainer& out_shards
        ) noexcept override;

    private:
        ice::ProxyAllocator _allocator;
        ice::Engine& _engine;
        ice::Clock const& _clock;
        ice::EngineSchedulers _schedulers;
        ice::EngineFrameFactory const _frame_factory;
        ice::EngineFrameFactoryUserdata const _frame_factory_userdata;
        ice::u32 const _frame_count;


        ice::u8 _flow_id;
        ice::IceshardDataStorage _frame_storage[2];
        ice::IceshardDataStorage _runtime_storage;

        std::atomic<ice::IceshardFrameData*> _frame_data_freelist;
        std::atomic<ice::u32> _next_frame_index;

        ice::ManualResetBarrier _barrier;
        char _padding[3];

        char alignas(alignof(ice::WorldStateParams)) _params_storage[sizeof(ice::WorldStateParams)];
    };

} // namespace ice
