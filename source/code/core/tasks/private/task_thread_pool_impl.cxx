/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "task_thread_pool_impl.hxx"
#include <ice/static_string.hxx>
#include <ice/string.hxx>
#include <ice/assert.hxx>

namespace ice
{

    namespace detail
    {

        template<typename... Args>
        void format_string(ice::StaticString<32>& out_string, ice::String format, Args&&... args) noexcept
        {
            char raw_buffer[32];
            auto const result = fmt::vformat_to_n(
                raw_buffer,
                ice::count(raw_buffer),
                fmt::string_view{ format.data(), format.size() },
                fmt::make_format_args(std::forward<Args>(args)...)
            );

            out_string = ice::String{ raw_buffer, (ice::u32) result.size };
        }

        auto aio_thread_routine(void* userdata, ice::TaskQueue&) noexcept -> ice::u32
        {
            ice::native_aio::AIOPort port = reinterpret_cast<ice::native_aio::AIOPort>(userdata);
            ice::native_aio::aio_process_events(port, {.timeout_ms = 250, .events_max = 4});
            return 0;
        }

    } // namespace detail

    TaskThreadPoolImplementation::TaskThreadPoolImplementation(
        ice::Allocator& alloc,
        ice::TaskQueue& queue,
        ice::TaskThreadPoolCreateInfo const& info
    ) noexcept
        : _allocator{ alloc }
        , _queue{ queue }
        , _info{ info }
        , _thread_pool{ _allocator }
        , _managed_threads{ _allocator }
        , _created_threads{ _allocator }
        , _user_threads{ _allocator }
    {
        _thread_pool.reserve(info.thread_count);
        _managed_threads.reserve(info.thread_count);
        _created_threads.reserve(info.thread_count);
        _user_threads.reserve(info.thread_count);

        ice::TaskThreadInfo thread_info{
            .exclusive_queue = false,
            .sort_by_priority = false,
            .stack_size = 0_B // default
        };

        ice::StaticString<32> thread_name;
        for (ice::u32 idx = 0; idx < _info.thread_count; ++idx)
        {
            detail::format_string(thread_name, info.debug_name_format, idx);

            thread_info.debug_name = thread_name;
            _managed_threads.push_back(
                ice::make_unique<ice::NativeTaskThread>(
                    _allocator,
                    _queue,
                    thread_info
                )
            );
        }

        // Create one additional thread for the AIO port. (TODO: Allow the port to be awaited on existing threads?)
        for (ice::u32 idx = 0; idx < ice::native_aio::aio_worker_limit(_info.aioport); ++idx)
        {
            detail::format_string(thread_name, "ice.aio {}", idx);

            _managed_threads.push_back(
                ice::make_unique<ice::NativeTaskThread>(
                    _allocator,
                    _queue,
                    ice::TaskThreadInfo{
                        .exclusive_queue = true,
                        .sort_by_priority = false,
                        .wait_on_queue = false,
                        .custom_procedure = detail::aio_thread_routine,
                        .custom_procedure_userdata = _info.aioport,
                        .debug_name = thread_name,
                    }
                )
            );
        }
    }

    TaskThreadPoolImplementation::~TaskThreadPoolImplementation() noexcept
    {
        _user_threads.clear();
        _created_threads.clear();
        _managed_threads.clear();
        _thread_pool.clear();
    }

    auto TaskThreadPoolImplementation::thread_count() const noexcept -> ice::ncount
    {
        return _thread_pool.size();
    }

    auto TaskThreadPoolImplementation::managed_thread_count() const noexcept -> ice::ncount
    {
        return _managed_threads.size() + _created_threads.size();
    }

    auto TaskThreadPoolImplementation::estimated_task_count() const noexcept -> ice::ncount
    {
        return 0; // TODO:
    }

    auto TaskThreadPoolImplementation::create_thread(ice::StringID name) noexcept -> ice::TaskThread&
    {
        ICE_ASSERT(
            _created_threads.missing(name),
            "A pool thread with name '{}' already exists",
            name
        );

        std::string_view const name_hint = ice::stringid_hint(name);
        ice::TaskThreadInfo const thread_info{
            .exclusive_queue = false,
            .sort_by_priority = false,
            .stack_size = 0_B, // default
            .debug_name = ice::String{ name_hint.data(), static_cast<ice::u32>(name_hint.size()) }
        };

        _created_threads.set(
            name,
            ice::make_unique<ice::NativeTaskThread>(
                _allocator,
                _queue,
                thread_info
            )
        );

        return **_created_threads.try_get(ice::hash(name));
    }

    auto TaskThreadPoolImplementation::find_thread(ice::StringID name) noexcept -> ice::TaskThread*
    {
        if (auto const& unique_ptr = _created_threads.try_get(ice::hash(name)))
        {
            return unique_ptr->get();
        }
        return nullptr;
    }

    bool TaskThreadPoolImplementation::destroy_thread(ice::StringID name) noexcept
    {
        return _created_threads.remove(name);
    }

    auto TaskThreadPoolImplementation::attach_thread(
        ice::StringID name,
        //ice::TaskFlags accepting_flags,
        ice::UniquePtr<ice::TaskThread> thread
    ) noexcept -> ice::TaskThread&
    {
        ICE_ASSERT(
            _user_threads.missing(name),
            "A user thread with name '{}' already exists",
            name
        );

        return *_user_threads.set(name, ice::move(thread));
    }

    auto TaskThreadPoolImplementation::detach_thread(
        ice::StringID name
    ) noexcept -> ice::UniquePtr<ice::TaskThread>
    {
        ice::u64 const name_hash = ice::hash(name);
        ice::UniquePtr<ice::TaskThread> result;
        if (_user_threads.has(name_hash))
        {
            // Move the thread out of the map
            result = ice::move(*_user_threads.try_get(name_hash));
            // Remove the element from the map
            _user_threads.remove(name_hash);
        }
        return result;
    }

} // namespace ice
