#pragma once
#include <ice/container/linked_queue.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <coroutine>


namespace ice::tasks::v2
{
    template<typename Result = void>
    class Task;

    struct PromiseTypeBase
    {
        struct FinalAwaitable
        {
            bool await_ready() const noexcept { return false; }

            template<typename Promise>
            auto await_suspend(std::coroutine_handle<Promise> coro) noexcept -> std::coroutine_handle<>
            {
                return coro.promise().continuation();
            }

            void await_resume() noexcept { }
        };

        auto initial_suspend() const noexcept
        {
            return std::suspend_always{ };
        }

        auto final_suspend() const noexcept
        {
            return FinalAwaitable{ };
        }

        auto set_continuation(std::coroutine_handle<> coro) noexcept
        {
            _continuation = coro;
        }

        auto continuation() const noexcept -> std::coroutine_handle<>
        {
            return _continuation;
        }

        void unhandled_exception() const noexcept
        {
            // TODO:
        }

    protected:
        PromiseTypeBase() noexcept = default;

    private:
        std::coroutine_handle<> _continuation;
    };

    //template<>
    //struct PromiseType<void>
    //{
    //    auto initial_suspend() const noexcept { return std::suspend_always{ }; }
    //    auto final_suspend() const noexcept { return std::suspend_always{ }; }

    //    void unhandled_exception() const noexcept { }

    //    auto get_return_object() noexcept -> ice::tasks::v2::Task<void>;

    //    void return_void() const noexcept { }
    //};

    template<typename T, bool = false>
    class PromiseType final : public PromiseTypeBase
    {
    public:
        inline auto get_return_object() noexcept -> Task<T>;

        template<typename Value, typename = std::enable_if_t<std::is_convertible_v<Value&&, T>>>
        void return_value(Value&& value) noexcept(std::is_nothrow_move_assignable_v<T>)
        {
            _value = std::move(value);
        }

        auto result() & noexcept -> T&
        {
            return _value;
        }

        auto result() && noexcept -> T&&
        {
            return std::move(_value);
        }

        void unhandled_exception() const noexcept
        {
            ICE_ASSERT(false, "Unhandled exception in Task object!");
        }

    private:
        T _value;
    };

    template<typename T>
    class PromiseType<T, true> final : public PromiseTypeBase
    {
    public:
        inline auto get_return_object() noexcept -> Task<T>;

        template<typename Value, typename = std::enable_if_t<std::is_convertible_v<Value&&, T>>>
        void return_value(Value&& value) noexcept(std::is_nothrow_constructible_v<T, Value&&>)
        {
            ::new (static_cast<void*>(ice::addressof(_value))) T{ ice::forward<Value>(value) };
        }

        auto result() & noexcept -> T&
        {
            return _value;
        }

        auto result() && noexcept -> T&&
        {
            return std::move(_value);
        }

        void unhandled_exception() const noexcept
        {
            ICE_ASSERT(false, "Unhandled exception in Task object!");
        }

    private:
        T _value;
    };

    template<bool Trivial>
    class PromiseType<void, Trivial> : public PromiseTypeBase
    {
    public:
        PromiseType() noexcept = default;

        inline auto get_return_object() noexcept -> Task<void>;

        void return_void() noexcept { }

        void result() noexcept { }

        void unhandled_exception() const noexcept
        {
            ICE_ASSERT(false, "Unhandled exception in Task object!");
        }
    };

    template<typename T>
    class PromiseType<T&, false> : public PromiseTypeBase
    {
    public:
        PromiseType() noexcept = default;

        inline auto get_return_object() noexcept -> Task<T&>;

        void return_value(T& value) noexcept
        {
            _value = ice::addressof(value);
        }

        auto result() noexcept -> T&
        {
            return *_value;
        }

        void unhandled_exception() const noexcept
        {
            ICE_ASSERT(false, "Unhandled exception in Task object!");
        }

    private:
        T* _value = nullptr;
    };

    struct TaskRef;

    template<typename Result>
    class Task final
    {
    public:
        using ValueType = Result;
        using PromiseType = ice::tasks::v2::PromiseType<ValueType, ice::TrivialContainerLogicAllowed<ValueType>>;

    private:
        struct AwaitableBase
        {
            std::coroutine_handle<PromiseType> _coro;

            explicit AwaitableBase(
                std::coroutine_handle<PromiseType> coro
            ) noexcept
                : _coro{ coro }
            { }

            bool await_ready() const noexcept
            {
                return !_coro || _coro.done();
            }

            auto await_suspend(
                std::coroutine_handle<> awaiting_coroutine
            ) const noexcept -> std::coroutine_handle<>
            {
                _coro.promise().set_continuation(awaiting_coroutine);
                return _coro;
            }
        };

    public:
        explicit Task(std::coroutine_handle<PromiseType> coro) noexcept
            : _coro{ coro }
        { }

        Task(Task&& other) noexcept
            : _coro{ std::exchange(other._coro, nullptr) }
        { }

        Task(Task const&) noexcept = delete;
        auto operator=(Task const&) noexcept = delete;

        ~Task() noexcept
        {
            if (_coro)
            {
                _coro.destroy();
            }
        }

        auto operator=(Task&& other) noexcept -> Task&
        {
            if (this != &other)
            {
                if (_coro != nullptr)
                {
                    _coro.destroy();
                }

                _coro = ice::exchange(other._coro, nullptr);
            }

            return *this;
        }

        auto is_ready() const noexcept
        {
            return !_coro || _coro.done();
        }

        auto operator co_await() & noexcept
        {
            struct TaskAwaitable : AwaitableBase
            {
                using AwaitableBase::AwaitableBase;

                auto await_resume() const noexcept -> decltype(auto)
                {
                    ICE_ASSERT(
                        this->_coro.operator bool(),
                        "Broken promise on coroutine Task!"
                    );

                    if constexpr (std::is_same_v<ValueType, void> == false)
                    {
                        return this->_coro.promise().result();
                    }
                }
            };

            return TaskAwaitable{ _coro };
        }

        auto operator co_await() && noexcept
        {
            struct TaskAwaitable : AwaitableBase
            {
                using AwaitableBase::AwaitableBase;

                auto await_resume() const noexcept -> decltype(auto)
                {
                    ICE_ASSERT(
                        this->_coro.operator bool(),
                        "Broken promise on coroutine Task!"
                    );

                    if constexpr (std::is_same_v<ValueType, void> == false)
                    {
                        return std::move(this->_coro.promise().result());
                    }
                }
            };

            return TaskAwaitable{ std::move(_coro) };
        }

        auto when_ready() noexcept
        {
            struct TaskAwaitable : AwaitableBase
            {
                using AwaitableBase::AwaitableBase;

                void await_resume() const noexcept {}
            };

            return TaskAwaitable{ _coro };
        }

        auto get_ref() noexcept -> ice::tasks::v2::TaskRef;

    private:
        std::coroutine_handle<PromiseType> _coro;
    };

    struct TaskRef
    {
        std::coroutine_handle<> coro;
        //using FnDestroy = void(void*) noexcept;

        //char alignas(alignof(Task<void>)) taskdata[sizeof(Task<void>)];
        //FnDestroy* fn_destroy;
    };

    template<typename Result>
    auto Task<Result>::get_ref() noexcept -> ice::tasks::v2::TaskRef
    {
        return { _coro };
    }

    template<typename Value, bool Trivial>
    auto PromiseType<Value, Trivial>::get_return_object() noexcept -> ice::tasks::v2::Task<Value>
    {
        return ice::tasks::v2::Task<Value>{ std::coroutine_handle<ice::tasks::v2::PromiseType<Value, ice::TrivialContainerLogicAllowed<Value>>>::from_promise(*this) };
    }

    template<typename Value>
    auto PromiseType<Value, true>::get_return_object() noexcept -> ice::tasks::v2::Task<Value>
    {
        return ice::tasks::v2::Task<Value>{ std::coroutine_handle<ice::tasks::v2::PromiseType<Value, true>>::from_promise(*this) };
    }

    template<typename Value>
    auto PromiseType<Value&>::get_return_object() noexcept -> ice::tasks::v2::Task<Value&>
    {
        return ice::tasks::v2::Task<Value&>{ std::coroutine_handle<ice::tasks::v2::PromiseType<Value&>>::from_promise(this) };
    }

    auto PromiseType<void>::get_return_object() noexcept -> ice::tasks::v2::Task<void>
    {
        return ice::tasks::v2::Task<void>{ std::coroutine_handle<ice::tasks::v2::PromiseType<void>>::from_address(this) };
    }

    struct TaskCompletionToken
    {
        using Fn = void();
    };

    struct TaskCancelationToken
    {
        using Fn = bool(void const*);

        void const* const data;
        Fn* const fn;

        operator bool() const noexcept
        {
            return fn(data);
        }
    };

    struct TaskScheduler;

    struct TaskSchedulerParams
    {
        enum class Payload : ice::u32
        {
            None,
            DelayInfo,
        };

        Payload payload;
        union
        {
            ice::u32 delay;
        };
    };

    struct TaskAwaitableBase
    {
        ice::tasks::v2::TaskScheduler* const scheduler;
        ice::tasks::v2::TaskAwaitableBase* next;
        ice::tasks::v2::TaskSchedulerParams params;

        std::coroutine_handle<> coro;
    };

    struct TaskAwaitable : TaskAwaitableBase
    {
        bool await_ready() const noexcept { return false; }
        void await_suspend(std::coroutine_handle<> coroutine) noexcept;
        void await_resume() const noexcept { }
    };

    //template<typename Type>
    //struct TaskAwaitableReturnValue : TaskAwaitableBase
    //{
    //    bool await_ready() const noexcept { return false; }
    //    void await_suspend(std::coroutine_handle<ice::tasks::v2::Task<Type>> coro) noexcept;
    //    auto await_resume() const noexcept -> Type&&;
    //};

    class TaskQueue
    {
    public:
        TaskQueue() noexcept;
        ~TaskQueue() noexcept; // Destroy everything left in the queue

        void push(ice::tasks::v2::TaskAwaitableBase& awaitable) noexcept;
        auto pop() noexcept -> ice::tasks::v2::TaskAwaitableBase;
        auto pop_all() noexcept -> ice::LinkedQueueRange<ice::tasks::v2::TaskAwaitableBase>;

    private:
        ice::AtomicLinkedQueue<ice::tasks::v2::TaskAwaitableBase> _queue;
    };

    struct TaskThreadInfo
    {
        // \brief May be ignored in some builds.
        ice::String debug_name;
    };

    //template<typename T>
    //auto task_flags_from_uint(ice::u32 raw_value) noexcept -> T = delete;

    //template<typename T>
    //auto task_flags_to_uint(T flags) noexcept -> ice::u32 = delete;

    template<typename Enum>
    static constexpr bool TaskFlagsEnabled = false;

    using TaskFlagBaseType = ice::u16;
    static constexpr TaskFlagBaseType TaskFlagNormalPrioValue = 0x4000;

    template<typename Enum>
    concept TaskFlagType = ice::FlagType<Enum> && FlagAllValue<Enum> // Ensure it's a IceShard flag enabled type
        // Ensure the flag type is manually enabled and matches the expected base type
        && TaskFlagsEnabled<Enum> && std::is_same_v<TaskFlagBaseType, std::underlying_type_t<Enum>>
        // Ensure the specified names are part of the enum.
        && requires(Enum t) {
            { Enum::Long } -> std::convertible_to<Enum>;
            { Enum::PrioLow } -> std::convertible_to<Enum>;
            { Enum::PrioNormal } -> std::convertible_to<Enum>;
            { Enum::PrioHigh } -> std::convertible_to<Enum>; }
        // Check if the flag is part of 'All' and does it have the expected value
        && static_cast<TaskFlagBaseType>(Enum::All & Enum::Long) == (TaskFlagNormalPrioValue >> 2)
        && static_cast<TaskFlagBaseType>(Enum::All & Enum::PrioLow) == (TaskFlagNormalPrioValue >> 1)
        && static_cast<TaskFlagBaseType>(Enum::All & Enum::PrioNormal) == TaskFlagNormalPrioValue
        && static_cast<TaskFlagBaseType>(Enum::All & Enum::PrioHigh) == (TaskFlagNormalPrioValue << 1);

    struct TaskFlags
    {
        TaskFlagBaseType value = TaskFlagNormalPrioValue;

        constexpr TaskFlags() noexcept = default;

        template<ice::tasks::v2::TaskFlagType Flags>
        constexpr explicit TaskFlags(Flags flags) noexcept
            : value{ static_cast<TaskFlagBaseType>(flags) }
        {
        }

        template<ice::tasks::v2::TaskFlagType Flags>
        constexpr auto operator=(Flags flags) noexcept -> TaskFlags&
        {
            value = static_cast<TaskFlagBaseType>(flags);
        }

        template<ice::tasks::v2::TaskFlagType Flags>
        constexpr operator Flags() const noexcept
        {
            return static_cast<Flags>(value);
        }
    };

    enum class ProjectTaskFlags : ice::u16
    {
        None = 0x00,

        Main = 0x01,
        Gfx = 0x02,
        IO = 0x04,
        UI = 0x08,

        Long = 0x1000,
        PrioLow = 0x2000,
        PrioNormal = 0x4000,
        PrioHigh = 0x8000,

        All = Main | Gfx | IO | UI | PrioHigh | PrioNormal | PrioLow | Long | IO
    };

    template<>
    constexpr bool TaskFlagsEnabled<ProjectTaskFlags> = true;

    //static constexpr bool is_enabled = TaskFlagType<ProjectTaskFlags>;

    //static constexpr TaskFlags accepted_value{ ProjectTaskFlags::All };
    //static constexpr TaskFlags accepted_value;

    struct TaskQueuePusher
    {
        virtual ~TaskQueuePusher() noexcept = default;
        virtual void push(ice::tasks::v2::TaskAwaitableBase& awaitable, ice::tasks::v2::TaskFlags) noexcept = 0;
    };

    struct TaskThreadPoolInfo
    {
        ice::ucount min_balanced_thread_count = 0;
        ice::ucount max_balanced_thread_count = 2;

        //! \brief If not set, creates a single queue for all task types.
        ice::Span<ice::tasks::v2::TaskFlags> queues = { };

        //! \brief The queue that should be returned if flags didn't match any other queues.
        //!
        //! \note If '-1' no queue will be returned.
        ice::icount missing_flags_queue = -1;

        bool allow_attaching = true;

        //! \brief May be ignored in some builds.
        ice::String debug_name_format;
    };

    struct TaskThread
    {
        virtual auto info() const noexcept -> ice::tasks::v2::TaskThreadInfo const& = 0;
        virtual bool is_busy() const noexcept = 0;
        virtual bool is_running() const noexcept = 0;

        virtual auto task_count() const noexcept -> ice::ucount = 0;
        virtual auto queue_pusher() const noexcept -> ice::tasks::v2::TaskQueuePusher& = 0;

        virtual void join() noexcept = 0;
    };

    auto create_thread(
        ice::Allocator& alloc,
        ice::tasks::v2::TaskThreadInfo const& thread_info
    ) noexcept -> ice::UniquePtr<ice::tasks::v2::TaskThread>;

    struct TaskThreadPool : public ice::tasks::v2::TaskQueuePusher
    {
        virtual auto thread_count() const noexcept -> ice::ucount = 0;
        virtual auto managed_thread_count() const noexcept -> ice::ucount = 0;
        virtual auto estimated_task_count() const noexcept -> ice::ucount = 0;

        virtual auto create_thread(ice::StringID name) noexcept -> ice::tasks::v2::TaskThread& = 0;
        virtual auto find_thread(ice::StringID name) noexcept -> ice::tasks::v2::TaskThread* = 0;
        virtual bool destroy_thread(ice::StringID name) noexcept = 0;

        virtual auto attach_thread(
            ice::StringID name,
            ice::tasks::v2::TaskFlags accepting_flags,
            ice::UniquePtr<ice::tasks::v2::TaskThread> thread
        ) noexcept -> ice::tasks::v2::TaskThread& = 0;

        virtual auto detach_thread(
            ice::StringID name
        ) noexcept -> ice::UniquePtr<ice::tasks::v2::TaskThread> = 0;

        //! \brief Returns the default queue pusher for the given task flags.
        //!
        //! \note If no queues exists that can accept such flags it will return a nullptr.
        virtual auto queue_pusher(
            ice::tasks::v2::TaskFlags flags
        ) const noexcept -> ice::tasks::v2::TaskQueuePusher* = 0;
    };

    auto create_thread_pool(
        ice::Allocator& alloc,
        ice::tasks::v2::TaskThreadPoolInfo const& threadpool_info
    ) noexcept -> ice::UniquePtr<ice::tasks::v2::TaskThreadPool>;

    struct TaskScheduler
    {
        ice::tasks::v2::TaskQueuePusher& queue;
        //ice::tasks::v2::TaskFlags enqueue_flags = { };

        //template<typename Type>
        auto schedule() noexcept
        {
            //if constexpr (std::is_same_v<Type, void>)
            {
                ice::tasks::v2::TaskAwaitable result{ this };
                result.params.payload = TaskSchedulerParams::Payload::None;
                return result;
            }
            //else
            //{
            //    ice::tasks::v2::TaskAwaitableReturnValue<Type> result{ };
            //    result.next = nullptr;
            //    result.scheduler = this;
            //    result.params.payload = TaskSchedulerParams::Payload::None;
            //    return result;
            //}
        }

        //template<typename Type>
        auto schedule(ice::u32 delay_in_ms) noexcept // -> ice::tasks::v2::TaskAwaitableReturnValue<Type>
        {
            //if constexpr (std::is_same_v<Type, void>)
            {
                ice::tasks::v2::TaskAwaitable result{ this };
                result.params.payload = TaskSchedulerParams::Payload::DelayInfo;
                result.params.delay = delay_in_ms;
                return result;
            }
            //else
            //{
            //    ice::tasks::v2::TaskAwaitableReturnValue<Type> result{ this };
            //    result.next = nullptr;
            //    result.params.payload = TaskSchedulerParams::Payload::DelayInfo;
            //    result.params.delay = delay_in_ms;
            //    return result;
            //}
        }

        auto operator co_await() noexcept
        {
            return schedule();
        }
    };

    void TaskAwaitable::await_suspend(std::coroutine_handle<> coroutine) noexcept
    {
        coro = coroutine;
        scheduler->queue.push(*this, TaskFlags{ });// scheduler->enqueue_flags);
    }

    template<typename Value>
    auto schedule_on(ice::tasks::v2::Task<Value>&& task, ice::tasks::v2::TaskScheduler& scheduler) noexcept -> ice::tasks::v2::Task<Value>
    {
        co_await scheduler;
        co_return co_await std::move(task);
    }

    template<typename Value>
    auto resume_on(ice::tasks::v2::Task<Value>&& task, ice::tasks::v2::TaskScheduler& scheduler) noexcept -> ice::tasks::v2::Task<Value>
    {
        Value value = co_await std::move(task);
        co_await scheduler;
        co_return std::move(value);
    }

    void manual_sync_tasks(
        ice::Span<ice::ManualResetEvent> manual_reset_events,
        ice::Span<ice::tasks::v2::TaskRef> task_refs
    ) noexcept;

    template<typename... Args>
    void manual_sync_tasks(ice::Span<ice::ManualResetEvent> events, ice::tasks::v2::Task<Args>&&... tasks) noexcept
    {
        auto tasks = std::make_tuple(std::move(tasks)...);
        auto tasks_refs = std::apply([](auto& task) { return task.get_ref(); }, tasks);
        manual_sync_tasks(events, tasks_refs);
    }

    template<typename... Args>
    auto sync_tasks(ice::tasks::v2::Task<Args>&&... tasks) noexcept -> std::tuple<Args...>
    {
        auto tasks = std::make_tuple(std::move(tasks)...);
        auto tasks_refs = std::apply([](auto& task) { return task.get_ref(); }, tasks);
    }

    struct EngineSchedulers
    {
        ice::tasks::v2::TaskScheduler& main;
        ice::tasks::v2::TaskScheduler& gfx;
        ice::tasks::v2::TaskScheduler& io;
        ice::tasks::v2::TaskScheduler& short_tasks;
        ice::tasks::v2::TaskScheduler& long_tasks;
    };

    struct TaskTokens
    {
        TaskCompletionToken completed;
    };

} // namespace ice

template<typename Result, typename... Args>
struct std::coroutine_traits<ice::tasks::v2::Task<Result>, Args...>
{
    using promise_type = typename ice::tasks::v2::Task<Result>::PromiseType;
};

auto test2(ice::tasks::v2::EngineSchedulers&) noexcept -> ice::tasks::v2::Task<int>
{
    co_return 11;
}

auto test(ice::tasks::v2::EngineSchedulers& q, ice::tasks::v2::TaskCancelationToken& cancel) noexcept -> ice::tasks::v2::Task<>
{
    co_await q.io;

    int foo = co_await test2(q);
    int foo2 = co_await schedule_on(test2(q), q.main);
    int foo3 = co_await resume_on(test2(q), q.long_tasks);
    foo2 = foo + foo3;

    if (cancel)
    {
        co_return;
    }

    co_await q.short_tasks.schedule();
    co_await q.gfx.schedule(230u);
}


//// Enable the use of std::future<T> as a coroutine type
//// by using a std::promise<T> as the promise type.
//template <typename T, typename... Args>
//    requires(!std::is_void_v<T> && !std::is_reference_v<T>)
//struct std::coroutine_traits<std::future<T>, as_coroutine, Args...> {
//    struct promise_type : std::promise<T> {
//        std::future<T> get_return_object() noexcept {
//            return this->get_future();
//        }
//
//        std::suspend_never initial_suspend() const noexcept { return {}; }
//        std::suspend_never final_suspend() const noexcept { return {}; }
//
//        void return_value(const T& value)
//            noexcept(std::is_nothrow_copy_constructible_v<T>) {
//            this->set_value(value);
//        }
//        void return_value(T&& value)
//            noexcept(std::is_nothrow_move_constructible_v<T>) {
//            this->set_value(std::move(value));
//        }
//        void unhandled_exception() noexcept {
//            this->set_exception(std::current_exception());
//        }
//    };
//};
//
//// Same for std::future<void>.
//template <typename... Args>
//struct std::coroutine_traits<std::future<void>, as_coroutine, Args...> {
//    struct promise_type : std::promise<void> {
//        std::future<void> get_return_object() noexcept {
//            return this->get_future();
//        }
//
//        std::suspend_never initial_suspend() const noexcept { return {}; }
//        std::suspend_never final_suspend() const noexcept { return {}; }
//
//        void return_void() noexcept {
//            this->set_value();
//        }
//        void unhandled_exception() noexcept {
//            this->set_exception(std::current_exception());
//        }
//    };
//};
