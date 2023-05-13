/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "task_thread_pool_impl.hxx"
#include <ice/string/static_string.hxx>
#include <ice/string/string.hxx>
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
                fmt::string_view{format._data, format._size},
                fmt::make_format_args(std::forward<Args>(args)...)
            );

            out_string = ice::String{ raw_buffer, (ice::ucount) result.size };
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
        ice::array::reserve(_thread_pool, info.thread_count);
        ice::array::reserve(_managed_threads, info.thread_count);
        ice::hashmap::reserve(_created_threads, info.thread_count);
        ice::hashmap::reserve(_user_threads, info.thread_count);

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
            ice::array::push_back(
                _managed_threads,
                ice::make_unique<ice::NativeTaskThread>(
                    _allocator,
                    _queue,
                    thread_info
                )
            );
        }
    }

    TaskThreadPoolImplementation::~TaskThreadPoolImplementation() noexcept
    {
        ice::hashmap::clear(_user_threads);
        ice::hashmap::clear(_created_threads);
        ice::array::clear(_managed_threads);
        ice::array::clear(_thread_pool);
    }

    auto TaskThreadPoolImplementation::thread_count() const noexcept -> ice::ucount
    {
        return ice::array::count(_thread_pool);
    }

    auto TaskThreadPoolImplementation::managed_thread_count() const noexcept -> ice::ucount
    {
        return ice::array::count(_managed_threads) + ice::hashmap::count(_created_threads);
    }

    auto TaskThreadPoolImplementation::estimated_task_count() const noexcept -> ice::ucount
    {
        return 0; // TODO:
    }

    auto TaskThreadPoolImplementation::create_thread(ice::StringID name) noexcept -> ice::TaskThread&
    {
        ICE_ASSERT(
            ice::hashmap::has(_created_threads, ice::hash(name)) == false,
            "A pool thread with name '{}' already exists",
            name
        );

        std::string_view const name_hint = ice::stringid_hint(name);
        ice::TaskThreadInfo const thread_info{
            .exclusive_queue = false,
            .sort_by_priority = false,
            .stack_size = 0_B, // default
            .debug_name = ice::String{ name_hint.data(), static_cast<ice::ucount>(name_hint.size()) }
        };

        ice::hashmap::set(
            _created_threads,
            ice::hash(name),
            ice::make_unique<ice::NativeTaskThread>(
                _allocator,
                _queue,
                thread_info
            )
        );

        return **ice::hashmap::try_get(_created_threads, ice::hash(name));
    }

    auto TaskThreadPoolImplementation::find_thread(ice::StringID name) noexcept -> ice::TaskThread*
    {
        if (auto const& unique_ptr = ice::hashmap::try_get(_created_threads, ice::hash(name)))
        {
            return unique_ptr->get();
        }
        return nullptr;
    }

    bool TaskThreadPoolImplementation::destroy_thread(ice::StringID name) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);
        bool const exists = ice::hashmap::has(_created_threads, name_hash);
        if (exists)
        {
            ice::hashmap::remove(_created_threads, name_hash);
        }
        return exists;
    }

    auto TaskThreadPoolImplementation::attach_thread(
        ice::StringID name,
        //ice::TaskFlags accepting_flags,
        ice::UniquePtr<ice::TaskThread> thread
    ) noexcept -> ice::TaskThread&
    {
        ice::u64 const name_hash = ice::hash(name);
        ICE_ASSERT(
            ice::hashmap::has(_user_threads, name_hash) == false,
            "A user thread with name '{}' already exists",
            name
        );

        ice::hashmap::set(
            _user_threads,
            name_hash,
            ice::move(thread)
        );

        return **ice::hashmap::try_get(_user_threads, name_hash);
    }

    auto TaskThreadPoolImplementation::detach_thread(
        ice::StringID name
    ) noexcept -> ice::UniquePtr<ice::TaskThread>
    {
        ice::u64 const name_hash = ice::hash(name);
        ice::UniquePtr<ice::TaskThread> result;
        if (ice::hashmap::has(_user_threads, name_hash))
        {
            // Move the thread out of the map
            result = ice::move(*ice::hashmap::try_get(_user_threads, name_hash));
            // Remove the element from the map
            ice::hashmap::remove(_user_threads, name_hash);
        }
        return result;
    }

} // namespace ice
