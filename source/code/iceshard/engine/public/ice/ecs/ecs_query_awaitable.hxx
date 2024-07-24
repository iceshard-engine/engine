/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_awaitable.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/ecs/ecs_query_view.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
// #include <ice/ecs/ecs_query.hxx>

namespace ice::ecs
{

    namespace detail
    {

        static constexpr ice::u32 QueryAccessCounterAddition[]{ 0x0000'0001, 0x0001'0000 };
        static constexpr ice::u32 QueryAccessCounterMask[]{ 0xffff'0000, 0xffff'ffff };

        template<typename Definition>
        bool internal_query_is_resumable(
            ice::ecs::Query<Definition> const& query,
            ice::u32 const(&awaited_access_stage)[Definition::Constant_ComponentCount]
        ) noexcept
        {
            // Go through all components and check if we can be resumed
            bool resumable = true;
            for (ice::u32 idx = 0; resumable && idx < Definition::Constant_ComponentCount; ++idx)
            {
                ice::ecs::QueryAccessTracker& tracker = *query.access_trackers[idx];

                bool const is_writable = Definition::Constant_Requirements[idx].is_writable;
                ice::u32 const required_stage = awaited_access_stage[idx];
                ice::u32 const current_exec = tracker.access_stage_executed.load(std::memory_order_relaxed);

                // Check if this component enables resumption
                resumable = (current_exec & QueryAccessCounterMask[is_writable]) == (required_stage & QueryAccessCounterMask[is_writable]);
            }

            return resumable;
        }

        template<typename Definition>
        inline auto internal_query_request_access_counters(
            ice::ecs::Query<Definition> const& query,
            ice::u32(&awaited_access_stage)[Definition::Constant_ComponentCount]
        ) noexcept
        {
            for (ice::u32 idx = 0; idx < Definition::Constant_ComponentCount; ++idx)
            {
                ice::ecs::QueryAccessTracker& tracker = *query.access_trackers[idx];

                bool const is_writable = Definition::Constant_Requirements[idx].is_writable;
                ice::u32 const addition = QueryAccessCounterAddition[is_writable];
                ice::u32 const required_stage = tracker.access_stage_next.fetch_add(addition, std::memory_order_relaxed);

                // Save the required exec
                awaited_access_stage[idx] = required_stage;
            }
        }

        template<typename Definition>
        inline auto internal_query_release_access_counters(
            ice::ecs::Query<Definition> const& query
        ) noexcept
        {
            for (ice::u32 idx = 0; idx < Definition::Constant_ComponentCount; ++idx)
            {
                ice::ecs::QueryAccessTracker& tracker = *query.access_trackers[idx];

                bool const is_writable = Definition::Constant_Requirements[idx].is_writable;

                // Update exec counter properly
                tracker.access_stage_executed.fetch_add(QueryAccessCounterAddition[is_writable], std::memory_order_relaxed);
            }
        }

        template<typename Definition>
        struct QueryAwaitableBase : ice::TaskAwaitableBase
        {
            ice::TaskQueue* _task_queue;
            ice::TaskAwaitableCustomResumer _custom_resumer;
            ice::u32 _awaited_access_stage[Definition::Constant_ComponentCount]{};

            static inline auto internal_get_query(void* userdata) noexcept -> ice::ecs::Query<Definition> const&
            {
                return *reinterpret_cast<ice::ecs::Query<Definition> const*>(userdata);
            }

            static inline bool internal_query_resumer(void* userdata, ice::TaskAwaitableBase const& awaitable) noexcept
            {
                QueryAwaitableBase<Definition> const& self = static_cast<QueryAwaitableBase<Definition> const&>(awaitable);

                return internal_query_is_resumable(internal_get_query(userdata), self._awaited_access_stage);
            }

            constexpr QueryAwaitableBase(ice::ecs::Query<Definition> const& query, ice::TaskQueue& task_queue) noexcept
                : ice::TaskAwaitableBase{ ._params = { .modifier = ice::TaskAwaitableModifier::CustomResumer } }
                , _task_queue{ ice::addressof(task_queue) }
                , _custom_resumer{ }
                , _awaited_access_stage{ }
            {
                _custom_resumer.ud_resumer = (void*) ice::addressof(query);
                _custom_resumer.fn_resumer = internal_query_resumer;
                this->result.ptr = ice::addressof(_custom_resumer);
            }

            constexpr QueryAwaitableBase(QueryAwaitableBase&& other) noexcept
                : ice::TaskAwaitableBase{ .modifier = ice::TaskAwaitableModifier::CustomResumer }
                , _task_queue{ std::exchange(other._task_queue) }
                , _custom_resumer{ std::exchange(other._custom_resumer, {}) }
                , _awaited_access_stage{ }
            {
                this->result.ptr = ice::addressof(_custom_resumer);
            }

            constexpr auto operator=(QueryAwaitableBase&& other) noexcept -> QueryAwaitableBase&
            {
                if (std::addressof(other) != this)
                {
                    _task_queue = std::exchange(other._task_queue);
                    _custom_resumer = std::exchange(other._custom_resumer, {});
                }
                return *this;
            }

            inline auto query() const noexcept -> ice::ecs::Query<Definition> const&
            {
                return internal_get_query(_custom_resumer.ud_resumer);
            }

            constexpr void await_suspend(std::coroutine_handle<> coroutine) noexcept
            {
                ice::ecs::detail::internal_query_request_access_counters(this->query(), _awaited_access_stage);

                _coro = coroutine;
                _task_queue->push_back(this);
            }
        };

    } // namespace detail

} // namespace ice::ecs
