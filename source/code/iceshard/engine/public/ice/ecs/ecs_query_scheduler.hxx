/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/ecs/ecs_query.hxx>

namespace ice::ecs
{

    template<typename Query>
    struct ScheduledQueryOperation;
    struct ScheduledQueryData;

    class QueryScheduler
    {
    public:
        virtual ~QueryScheduler() noexcept = default;

        template<typename QueryDefinition>
        auto schedule_query(ice::ecs::Query<QueryDefinition>&& query) noexcept -> ice::ecs::ScheduledQueryOperation<ice::ecs::Query<QueryDefinition>>;

    protected:
        virtual void schedule_query_internal(ScheduledQueryData& query_Data) noexcept = 0;
    };


    struct ScheduledQueryData
    {
        std::coroutine_handle<> coroutine;
        ice::ecs::ScheduledQueryData* next;
        ice::Span<ice::ecs::detail::QueryTypeInfo const> requirements;
    };

    template<typename Query>
    struct ScheduledQueryOperation final
    {
        inline ScheduledQueryOperation(
            QueryScheduler& scheduler,
            Query&& query
        ) noexcept;

        inline bool await_ready() const noexcept { return false; }
        inline void await_suspend(std::coroutine_handle<> coro) noexcept;
        inline auto await_resume() const noexcept -> Query const&;

    protected:
        QueryScheduler& _scheduler;
        Query _query;
        ScheduledQueryData _data;
    };


    template<typename Query>
    inline ScheduledQueryOperation<Query>::ScheduledQueryOperation(
        QueryScheduler& scheduler,
        Query&& query
    ) noexcept
        : _scheduler{ scheduler }
        , _query{ ice::move(query) }
        , _data{
            .coroutine = nullptr,
            .next = nullptr,
            .requirements = Query::Constant_QueryDefinition.requirements
        }
    {
    }

    template<typename Query>
    inline void ScheduledQueryOperation<Query>::await_suspend(std::coroutine_handle<> coro) noexcept
    {
        _data.coroutine = coro;
        _scheduler.schedule_query_internal(_data);
    }

    template<typename Query>
    inline auto ScheduledQueryOperation<Query>::await_resume() const noexcept -> Query const&
    {
        return _query;
    }

    template<typename QueryDefinition>
    inline auto QueryScheduler::schedule_query(
        ice::ecs::Query<QueryDefinition>&& query
    ) noexcept -> ice::ecs::ScheduledQueryOperation<ice::ecs::Query<QueryDefinition>>
    {
        return { *this, ice::move(query) };
    }

} // namespace ice::ecs
