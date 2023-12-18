#pragma once
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/task.hxx>

namespace ice
{

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
        ice::Allocator& _allocator;

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
            : _allocator{ alloc }
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

    class IceshardEngineRunner : public ice::EngineRunner
    {
    public:
        IceshardEngineRunner(ice::Allocator& alloc, ice::EngineRunnerCreateInfo const& create_info) noexcept;
        ~IceshardEngineRunner() noexcept override;

        auto aquire_frame() noexcept -> ice::Task<ice::UniquePtr<ice::EngineFrame>> override;
        auto update_frame(ice::EngineFrame& frame, ice::EngineFrame const& prev_frame, ice::Clock const& clock) noexcept -> ice::Task<> override;
        void release_frame(ice::UniquePtr<ice::EngineFrame> frame) noexcept override;

        void destroy() noexcept;

    private:
        ice::Allocator& _allocator;
        ice::Engine& _engine;
        ice::EngineSchedulers const _schedulers;
        ice::EngineFrameFactory const _frame_factory;
        ice::EngineFrameFactoryUserdata const _frame_factory_userdata;
        ice::u32 const _frame_count;

        ice::IceshardDataStorage _frame_storage[2];
        ice::IceshardDataStorage _runtime_storage;

        std::atomic<ice::IceshardFrameData*> _frame_data_freelist;
        std::atomic<ice::u32> _next_frame_index;

        ice::TaskQueue _main_queue;
        ice::TaskScheduler _main_scheduler;
        ice::ManualResetBarrier _barrier;
    };

} // namespace ice::v2
