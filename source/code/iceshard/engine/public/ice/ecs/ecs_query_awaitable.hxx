/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_awaitable.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/ecs/ecs_types.hxx>
// #include <ice/ecs/ecs_query.hxx>

namespace ice::ecs
{

    namespace detail
    {

        static constexpr ice::u32 QueryAccessCounterAddition[]{ 0x0000'0001, 0x0001'0000 };
        static constexpr ice::u32 QueryAccessCounterMask[]{ 0xffff'0000, 0xffff'ffff };

        template<typename T1, typename T2>
        struct filter_query_type { static constexpr bool keep = true; };

        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1 const&, T2&> { static constexpr bool keep = false; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1 const*, T2&> { static constexpr bool keep = false; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1 const&, T2*> { static constexpr bool keep = false; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1 const*, T2*> { static constexpr bool keep = false; };

        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1&, T2 const&> { static constexpr bool keep = true; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1&, T2 const*> { static constexpr bool keep = true; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1*, T2 const&> { static constexpr bool keep = true; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1*, T2 const*> { static constexpr bool keep = true; };

        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1&, T2*> { static constexpr bool keep = true; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1*, T2&> { static constexpr bool keep = false; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1 const&, T2 const*> { static constexpr bool keep = true; };
        template<typename T1, typename T2> requires(std::is_same_v<T1, T2>) struct filter_query_type<T1 const*, T2 const&> { static constexpr bool keep = false; };

        template <typename T, typename... Ts>
        struct filtered_query_types : std::type_identity<T> {};

        template <typename... Ts, typename U>
        struct filtered_query_types<std::tuple<Ts...>, U>
            : std::conditional_t<(filter_query_type<U, Ts>::keep && ...)
            , ice::ecs::detail::filtered_query_types<std::tuple<Ts..., U>>
            , ice::ecs::detail::filtered_query_types<std::tuple<Ts...>>> {
        };

        template <typename... Ts, typename U, typename... Us>
        struct filtered_query_types<std::tuple<Ts...>, U, Us...>
            : std::conditional_t<(filter_query_type<U, Us>::keep && ...)
            , ice::ecs::detail::filtered_query_types<std::tuple<Ts..., U>, Us...>
            , ice::ecs::detail::filtered_query_types<std::tuple<Ts...>, Us...>> { };

        template <typename... Ts>
        using filtered_query_types_t = typename ice::ecs::detail::filtered_query_types<std::tuple<>, Ts...>::type;

        template <typename T>
        struct unique_query_types_from_tuple;

        template <typename... Ts>
        struct unique_query_types_from_tuple<std::tuple<Ts...>>
        {
            using type = ice::make_unique_tuple<ice::ecs::detail::filtered_query_types_t<Ts...>>;
        };

        template <typename QueryTypes>
        using query_access_types_t = typename ice::ecs::detail::unique_query_types_from_tuple<QueryTypes>::type;

    } // namespace detail

    namespace query
    {

        template<typename MainPart, typename... RefParts>
        inline auto entity_count(
            ice::ecs::QueryObject<MainPart, RefParts...> const& query
        ) noexcept -> ice::ucount;

    } // namespace query

    namespace detail
    {

        template<ice::u32 Size, typename... Parts>
        bool internal_query_is_resumable(
            ice::ecs::QueryObject<Parts...> const& query,
            ice::u32 const(&awaited_access_stage)[Size]
        ) noexcept
        {
            using Query = typename ice::ecs::QueryObject<Parts...>;
            using Definition = QueryDefinitionFromTuple<detail::query_access_types_t<typename Query::ComponentsTypeList>>;

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

        template<ice::u32 Size, typename... Parts>
        inline auto internal_query_request_access_counters(
            ice::ecs::QueryObject<Parts...> const& query,
            ice::u32(&awaited_access_stage)[Size]
        ) noexcept
        {
            using Query = typename ice::ecs::QueryObject<Parts...>;
            using Definition = QueryDefinitionFromTuple<detail::query_access_types_t<typename Query::ComponentsTypeList>>;

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

        template<typename... Parts>
        inline void internal_query_release_access_counters(
            ice::ecs::QueryObject<Parts...> const& query
        ) noexcept
        {
            using Query = typename ice::ecs::QueryObject<Parts...>;
            using Definition = QueryDefinitionFromTuple<detail::query_access_types_t<typename Query::ComponentsTypeList>>;

            for (ice::u32 idx = 0; idx < Definition::Constant_ComponentCount; ++idx)
            {
                ice::ecs::QueryAccessTracker& tracker = *query.access_trackers[idx];

                bool const is_writable = Definition::Constant_Requirements[idx].is_writable;

                // Update exec counter properly
                tracker.access_stage_executed.fetch_add(QueryAccessCounterAddition[is_writable], std::memory_order_relaxed);
            }
        }

        template<typename... Parts>
        struct QueryAwaitableBase : ice::TaskAwaitableBase
        {
            using QueryArg = ice::ecs::QueryObject<Parts...>;

            ice::TaskQueue* _task_queue;
            ice::TaskAwaitableCustomResumer _custom_resumer;
            ice::u32 _awaited_access_stage[QueryArg::ComponentCount]{};
            bool _is_empty;

            static inline auto internal_get_query(void* userdata) noexcept -> QueryArg const&
            {
                return *reinterpret_cast<QueryArg const*>(userdata);
            }

            static inline bool internal_query_resumer(void* userdata, ice::TaskAwaitableBase const& awaitable) noexcept
            {
                QueryAwaitableBase<Parts...> const& self = static_cast<QueryAwaitableBase<Parts...> const&>(awaitable);

                return internal_query_is_resumable(internal_get_query(userdata), self._awaited_access_stage);
            }

            constexpr QueryAwaitableBase(QueryArg const& query, ice::TaskQueue& task_queue) noexcept
                : ice::TaskAwaitableBase{ ._params = {.modifier = ice::TaskAwaitableModifier::CustomResumer } }
                , _task_queue{ ice::addressof(task_queue) }
                , _custom_resumer{ }
                , _awaited_access_stage{ }
                , _is_empty{ ice::ecs::query::entity_count(query) == 0 }
            {
                _custom_resumer.ud_resumer = (void*)ice::addressof(query);
                _custom_resumer.fn_resumer = internal_query_resumer;
                this->result.ptr = ice::addressof(_custom_resumer);
            }

            constexpr QueryAwaitableBase(QueryAwaitableBase&& other) noexcept
                : ice::TaskAwaitableBase{ ._params = {.modifier = ice::TaskAwaitableModifier::CustomResumer } }
                , _task_queue{ std::exchange(other._task_queue, nullptr) }
                , _custom_resumer{ std::exchange(other._custom_resumer, {}) }
                , _awaited_access_stage{ }
                , _is_empty{ other._is_empty }
            {
                this->result.ptr = ice::addressof(_custom_resumer);
            }

            constexpr auto operator=(QueryAwaitableBase&& other) noexcept -> QueryAwaitableBase&
            {
                if (std::addressof(other) != this)
                {
                    _task_queue = std::exchange(other._task_queue, nullptr);
                    _custom_resumer = std::exchange(other._custom_resumer, {});
                }
                return *this;
            }

            inline auto query_object() const noexcept -> QueryArg const&
            {
                return internal_get_query(_custom_resumer.ud_resumer);
            }

            constexpr bool await_ready() const noexcept
            {
                return _is_empty;
            }

            constexpr void await_suspend(std::coroutine_handle<> coroutine) noexcept
            {
                ice::ecs::detail::internal_query_request_access_counters(this->query_object(), _awaited_access_stage);

                _coro = coroutine;
                _task_queue->push_back(this);
            }
        };

    } // namespace detail

} // namespace ice::ecs
