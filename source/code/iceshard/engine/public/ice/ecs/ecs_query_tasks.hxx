#pragma once
#include <ice/ecs/ecs_query_awaitable.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/task_scheduler.hxx>

namespace ice::ecs
{

    template<ice::ecs::QueryType... QueryComponents>
    struct QueryExecutionScope
    {
        ice::ecs::Query<QueryComponents...> const* _query;

        inline QueryExecutionScope(
            ice::ecs::Query<QueryComponents...> const& query
        ) noexcept
            : _query{ ice::addressof(query) }
        {
        }

        inline QueryExecutionScope(QueryExecutionScope&& other) noexcept
            : _query{ ice::exchange(other._query, nullptr) }
        {
        }

        inline auto operator=(QueryExecutionScope&& other) noexcept -> QueryExecutionScope&
        {
            if (ice::addressof(other) != this)
            {
                _query = ice::exchange(other._query, nullptr);
            }
            return *this;
        }

        inline ~QueryExecutionScope() noexcept
        {
            if (_query == nullptr || ice::ecs::query::entity_count(*_query) == 0)
            {
                return;
            }

            ice::ecs::detail::internal_query_release_access_counters(*_query);
        }

        inline operator ice::ecs::Query<QueryComponents...> const&() const noexcept
        {
            ICE_ASSERT_CORE(_query != nullptr);
            return *_query;
        }

        inline operator bool() const noexcept
        {
            return _query != nullptr && ice::ecs::query::entity_count(*_query) > 0;
        }
    };

    template<ice::ecs::QueryType... QueryComponents>
    struct QueryAwaitable : ice::ecs::detail::QueryAwaitableBase<QueryComponents...>
    {
        using ice::ecs::detail::QueryAwaitableBase<QueryComponents...>::QueryAwaitableBase;

        inline bool await_ready() const noexcept
        {
            return ice::ecs::query::entity_count(this->query()) == 0;
        }

        inline auto await_resume() const noexcept -> ice::ecs::QueryExecutionScope<QueryComponents...>
        {
            return QueryExecutionScope<QueryComponents...>{ this->query() };
        }
    };

    template<ice::ecs::QueryType... QueryComponents>
    auto await_query_on(
        ice::ecs::Query<QueryComponents...> const& query,
        ice::TaskScheduler& scheduler
    ) noexcept -> ice::ecs::QueryAwaitable<QueryComponents...>
    {
        return ice::ecs::QueryAwaitable<QueryComponents...>{ query, scheduler.schedule()._queue };
    }

    namespace query
    {

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_entity(
            ice::ecs::QueryExecutionScope<QueryComponents...> const& query_scope
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>>
        {
            ICE_ASSERT_CORE(query_scope);
            return ice::ecs::query::for_each_entity(*query_scope._query);
        }

    } // namespace query

} // namespace ice::ecs
