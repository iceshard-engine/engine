#pragma once
#include <ice/ecs/ecs_query_awaitable.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/task_scheduler.hxx>

namespace ice::ecs
{

    template<typename T>
    struct QueryExecutionScope;

    template<ice::ecs::QueryType... QueryComponents>
    struct QueryExecutionScope<ice::ecs::QueryDefinition<QueryComponents...>>
    {
        ice::ecs::Query<ice::ecs::QueryDefinition<QueryComponents...>> const* _query;

        inline QueryExecutionScope(
            ice::ecs::Query<ice::ecs::QueryDefinition<QueryComponents...>> const& query
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

        inline operator bool() const noexcept
        {
            return _query != nullptr && ice::ecs::query::entity_count(*_query) > 0;
        }
    };

    template<typename Definition>
    struct QueryAwaitable : ice::ecs::detail::QueryAwaitableBase<Definition>
    {
        using ice::ecs::detail::QueryAwaitableBase<Definition>::QueryAwaitableBase;

        inline bool await_ready() const noexcept
        {
            return ice::ecs::query::entity_count(this->query()) == 0;
        }

        inline auto await_resume() const noexcept -> ice::ecs::QueryExecutionScope<Definition>
        {
            return QueryExecutionScope<Definition>{ this->query() };
        }
    };

    template<typename Definition>
    auto await_query_on(ice::ecs::Query<Definition> const& query, ice::TaskScheduler& scheduler) noexcept -> ice::ecs::QueryAwaitable<Definition>
    {
        return ice::ecs::QueryAwaitable<Definition>{ query, scheduler.schedule()._queue };
    }

} // namespace ice::ecs
