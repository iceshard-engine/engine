/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_data_block.hxx>
#include <ice/ecs/ecs_query_object.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/ecs/ecs_query_operations.hxx>

#include <ice/task_generator.hxx>
#include <ice/container/array.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    template<ice::ecs::QueryPolicy Policy, typename... Parts>
    struct Query : public TraitQueryOperations
    {
    public:
        using ObjectType = ice::ecs::QueryObject<Parts...>;
        using ResultType = typename ObjectType::ResultType;
        using BlockResultType = typename ObjectType::BlockResultType;
        using ComponentsTypeList = typename ObjectType::ComponentsTypeList;

        static constexpr ice::ecs::QueryPolicy Policy = QueryPolicy::Unchecked;
        static constexpr ice::u32 ComponentCount = ObjectType::ComponentCount;

        inline Query(ObjectType const& query) noexcept
            : _query{ query }
        { }

    public:
        auto query_object() const noexcept -> ObjectType const&
        {
            return _query;
        }

        auto synchronized_on(ice::TaskScheduler& scheduler) const noexcept;

    private:
        ObjectType const& _query;
    };

    template<typename... Parts>
    struct Query<QueryPolicy::Synchronized, Parts...> : public TraitQueryOperations
    {
    public:
        using ObjectType = ice::ecs::QueryObject<Parts...>;
        using ResultType = typename ObjectType::ResultType;
        using BlockResultType = typename ObjectType::BlockResultType;
        using ComponentsTypeList = typename ObjectType::ComponentsTypeList;

        static constexpr ice::ecs::QueryPolicy Policy = QueryPolicy::Synchronized;
        static constexpr ice::u32 ComponentCount = ObjectType::ComponentCount;

        inline Query(ObjectType const& query, bool requires_release) noexcept
            : _query{ query }
            , _requires_release{ requires_release }
        { }

        inline Query(Query&& other) noexcept
            : _query{ other._query }
            , _requires_release{ ice::exchange(other._requires_release, false) }
        { }

        inline ~Query() noexcept
        {
            if (_requires_release == false)
            {
                return;
            }

            ice::ecs::detail::internal_query_release_access_counters(_query);
        }

    public:
        auto query_object() const noexcept -> ObjectType const&
        {
            return _query;
        }

    private:
        ObjectType const& _query;
        bool _requires_release;
    };

    template<ice::ecs::QueryPolicy Policy, typename... Parts>
    auto Query<Policy, Parts...>::synchronized_on(ice::TaskScheduler& scheduler) const noexcept
    {
        struct Awaitable : ice::ecs::detail::QueryAwaitableBase<Parts...>
        {
            using ice::ecs::detail::QueryAwaitableBase<Parts...>::QueryAwaitableBase;

            inline auto await_resume() const noexcept -> ice::ecs::Query<QueryPolicy::Synchronized, Parts...>
            {
                return Query<QueryPolicy::Synchronized, Parts...>{ this->query_object(), this->_is_empty == false };
            }
        };

        return Awaitable{ _query, scheduler.schedule()._queue };
    }

    template<typename... Parts> Query(QueryObject<Parts...>&&) noexcept -> Query<QueryPolicy::Unchecked, Parts...>;
    template<typename... Parts> Query(QueryObject<Parts...> const&) noexcept -> Query<QueryPolicy::Unchecked, Parts...>;

} // namespace ice::ecs
