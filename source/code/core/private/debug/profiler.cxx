#include <core/debug/profiler.hxx>
#include <core/threading/spinlock.hxx>

#include <unordered_map>
#include <mutex>
#include <vector>


namespace core::debug
{
    namespace detail
    {

        //! Defines a profiler_context interface.
        struct profiler_context
        {
            virtual int threadid() const noexcept = 0;

            virtual void begin(std::string_view name, const void* user_data) noexcept = 0;

            virtual void end(std::string_view name) noexcept = 0;

            virtual void clear() noexcept = 0;

            virtual void visit(void* data, detail::profiler_event_callback callback) noexcept(false) = 0;

            virtual void save() noexcept = 0;
        };

        //////////////////////////////////////////////////////////////////////////
        //

        //! Defines a central storage for all 'instanced' thread_local_profiler_contexts.
        class profiler_context_system
        {
        public:
            profiler_context_system() noexcept = default;
            ~profiler_context_system() noexcept = default;

            //! Returns the next free context slot index.
            //! \note this is later passed as the threadid to the visit callback.
            int next_context_slot_index() noexcept
            {
                int context_index = _next_context_index++;
                //assert(context_index < detail::profiler_context_count);
                return context_index;
            }

            //! Register a new context in the system
            void register_context(profiler_context* context) noexcept
            {
                std::lock_guard<core::threading::spinlock> guard{ _spinlock_mutext };

                //assert(nullptr == _contexts[context->threadid()]);
                _contexts[context->threadid()] = context;
            }

            //! Unregister a context from the system
            void unregister_context(profiler_context* context) noexcept
            {
                std::lock_guard<core::threading::spinlock> guard{ _spinlock_mutext };

                //assert(context == _contexts[context->threadid()]);
                _contexts[context->threadid()] = nullptr;
            }

            //! Iterates over each context and visits it's events.
            void visit_all(void* data, detail::profiler_event_callback callback) noexcept(false)
            {
                std::lock_guard<core::threading::spinlock> guard{ _spinlock_mutext };

                for (auto& context : _contexts)
                {
                    if (nullptr != context)
                    {
                        // We pass the index as a thread id for now #todo Lets think of a better solution.
                        context->visit(data, callback);
                    }
                }
            }

            //! Iterates over each context and saves the current table of events.
            void save_all() noexcept
            {
                std::lock_guard<core::threading::spinlock> guard{ _spinlock_mutext };

                for (auto& context : _contexts)
                {
                    if (nullptr != context)
                    {
                        context->save();
                    }
                }
            }

        private:
            //! Holds a table of pointers to all registered contexts.
            profiler_context* _contexts[detail::profiler_context_count];

            //! Atomic counter of the current number of registered contexts.
            std::atomic_int _next_context_index{ 0 };

            //! A basic spinlock to make some operations thread save.
            core::threading::spinlock _spinlock_mutext;
        };

        profiler_context_system& get_profiler_context_system() noexcept
        {
            static profiler_context_system system;
            return system;
        }

        //////////////////////////////////////////////////////////////////////////
        //

        //! Makes a new profiler event and returns it.
        auto make_profiler_event(std::string_view event_name, profiler_clock::time_point timepoint, unsigned scope, const void* user_data) noexcept -> profiler_event
        {
            profiler_event ev{ event_name, timepoint, timepoint, static_cast<int>(scope), user_data };
            //std::memset(ev.name, '\0', sizeof(profiler_event::name));
            //std::memcpy(ev.name, name.data(), std::min(name.length(), sizeof(profiler_event::name) - 1));
            return ev;
        }

        static_assert(sizeof(profiler_event) * 100 < 102400, "Profiler event struct is too big!");

        //! Defines a thread_local profiling context which captures all events on a single thread.
        class thread_local_profiler_context : public profiler_context
        {
        public:
            thread_local_profiler_context() noexcept
                : _context_id{ get_profiler_context_system().next_context_slot_index() }
                , _events{ { }, { } }
            {
                _events[0].reserve(100);
                _events[1].reserve(100);
            }

            ~thread_local_profiler_context() noexcept
            {
                _events[0].clear();
                _events[1].clear();
            }

            int threadid() const noexcept
            {
                return _context_id;
            }

            void begin(std::string_view name, const void* user_data) noexcept override
            {
                // Get the time now
                auto timepoint_now = profiler_clock::now();

                // Get the context name hash
                auto hashed_name = std::hash_value(name.data());
                //assert(!pod::hash::has(_events[_current_mutable_table], hashed_name));

                // Store the debug context
                _events[_current_mutable_table].emplace(hashed_name, make_profiler_event(std::move(name), timepoint_now, _current_scope, user_data));

                // Push the scope
                _current_scope++;
            }

            void end(std::string_view name) noexcept override
            {
                // Get the time now.
                auto timepoint_now = profiler_clock::now();

                // Get the context name hash
                auto hashed_name = std::hash_value(name.data());
                //assert(pod::hash::has(_events[_current_mutable_table], hashed_name));

                auto& context = _events[_current_mutable_table][hashed_name];

                // Modify the end timepoint.
                const_cast<profiler_event&>(context).event_end = timepoint_now;

                // Pop the scope
                _current_scope--;
            }

            void clear() noexcept override
            {
                if (_save_flag.test_and_set(std::memory_order_relaxed))
                {
                    _current_mutable_table = (_current_mutable_table + 1) % 2;
                }
                _save_flag.clear(std::memory_order_relaxed);
                _events[_current_mutable_table].clear();
            }

            void visit(void* data, detail::profiler_event_callback callback) noexcept(false) override
            {
                for (auto& entry : _events[(_current_mutable_table + 1) % 2])
                {
                    callback(data, _context_id, entry.second);
                }
            }

            void save() noexcept override
            {
                _save_flag.test_and_set(std::memory_order_relaxed);
            }

        private:
            //! The given context id.
            const int _context_id;

            ////! A stack allocator holding 1kb of stack data.
            //core::memory::allocator& _backing_allocator;

            ////! A ring allocator to be used for the hash map.
            //core::memory::scratch_allocator _allocator;

            ////! Holds information about profiling contexts. #todo Move to a debug profiler type.
            //core::pod::Hash<profiler_event> _events[2];
            std::unordered_map<std::size_t, core::debug::detail::profiler_event> _events[2];

            unsigned int _current_scope = 0;

            //! Holds the index for the unlocked events hash table.
            unsigned int _current_mutable_table = 0;

            //! Holds a atomic_flag indicating if a profiling context save was scheduled.
            std::atomic_flag _save_flag = ATOMIC_FLAG_INIT;

            //! Dummy profiler event to avoid crashes ;)
            static const profiler_event _unknown_event;
        };

        //! Initialize the dummy profiler event
        const profiler_event thread_local_profiler_context::_unknown_event = make_profiler_event("<unknown_event>", profiler_clock::time_point{}, 0, nullptr);

        //! Holds a function thread local value so we don't need to dynamic allocate the objects.
        static profiler_context* get_thread_local_context()
        {
            static thread_local thread_local_profiler_context context{ };
            return &context;
        }

        //////////////////////////////////////////////////////////////////////////
        //

        //! Defines a global passive profiling context which ignore all profiling events.
        class passive_profiling_context : public profiler_context
        {
        public:
            int threadid() const noexcept { return -1; }
            void begin(std::string_view /*name*/, const void* /*user_data*/) noexcept override { /* empty */ }
            void end(std::string_view /*name*/) noexcept override { /* empty */ }
            void clear() noexcept override { /* empty */ }
            void visit(void* /*data*/, core::debug::detail::profiler_event_callback /*callback*/) noexcept(false) override { /* empty */ }
            void save() noexcept override { /* empty */ };
        };

        //! Holds the global passive profiling context to be set when a context was not initialized or was released.
        static profiler_context* get_passive_context()
        {
            static passive_profiling_context context{ };
            return &context;
        }

        //! Hold a global value which we can set or unset to initialize / release profiling contexts.
        static thread_local profiler_context* _profiler_context = get_passive_context();

    }

    //////////////////////////////////////////////////////////////////////////

    void initialize_profiling_context() noexcept
    {
        //assert(detail::get_passive_context() == detail::_profiler_context);

        detail::_profiler_context = core::debug::detail::get_thread_local_context();
        detail::get_profiler_context_system().register_context(detail::_profiler_context);
    }

    void release_profiling_context() noexcept
    {
        //assert(detail::get_passive_context() != detail::_profiler_context);

        detail::get_profiler_context_system().unregister_context(detail::_profiler_context);
        detail::_profiler_context = detail::get_passive_context();
    }

    void clear_profiling_context() noexcept
    {
        detail::_profiler_context->clear();
    }

    void save_profiling_context() noexcept
    {
        detail::_profiler_context->save();
    }

    void visit_profiling_context(void* data, core::debug::detail::profiler_event_callback callback) noexcept(false)
    {
        detail::_profiler_context->visit(data, callback);
    }

    void save_all_profiling_contexts() noexcept
    {
        detail::get_profiler_context_system().save_all();
    }

    void visit_all_profiling_contexts(void* data, core::debug::detail::profiler_event_callback callback) noexcept(false)
    {
        detail::get_profiler_context_system().visit_all(data, callback);
    }

    //////////////////////////////////////////////////////////////////////////

    Profiler::Profiler(std::string_view name) noexcept
        : _name{ std::move(name) }
        , _context{ detail::_profiler_context }
    {
        _context->begin(_name, nullptr);
    }

    Profiler::Profiler(std::string_view name, const void* user_data) noexcept
        : _name{ std::move(name) }
        , _context{ detail::_profiler_context }
    {
        _context->begin(_name, user_data);
    }

    Profiler::~Profiler() noexcept
    {
        _context->end(_name);
    }

} // namespace core::debug
