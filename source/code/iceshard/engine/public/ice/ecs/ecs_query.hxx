/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_query_object.hxx>
#include <ice/ecs/ecs_query_operations.hxx>

namespace ice::ecs
{

    //! \brief Utility type to access component data for one or multiple entities.
    //!
    //! \details A query consists of at least one \a main part, and zero or more sub-parts. Each part defines one additional
    //!   step-down in a entity hierarchy. This means, that if you have a two part query, the first part will be executed on the main
    //!   list of entities, while the second part will be executed on any entity linked <em>(part of a components data)</em> in the \a main part.
    //!
    //! \note Because there is no simple way of pre-fetching linked entities, their archetypes and storage data accesing their components
    //!   will be less efficient than when doing it in bulk. This feature is mainly provided as \a syntax-sugar, however minor optimizations
    //!   are where achieved. In the end user doesn't need to create and use an entirely separate query object.
    //!
    //! \tparam Type Type of the query, which can be either `Unchecked` or `Synchronized`.
    //! \tparam ...Parts All parts the query is required to execute before returning data.
    //!
    //! \see ice::ecs::TraitQueryOperations for the definition of each operation a query can perform.
    //! \see ice::ecs::QueryObject<Parts...>
    template<ice::ecs::QueryType Type, typename... Parts>
    struct Query : public ice::ecs::TraitQueryOperations
    {
    public:
        using ObjectType = ice::ecs::QueryObject<Parts...>;
        using ResultType = typename ObjectType::ResultType;
        using BlockResultType = typename ObjectType::BlockResultType;
        using ComponentsTypeList = typename ObjectType::ComponentsTypeList;

        static constexpr ice::ecs::QueryType Type = QueryType::Unchecked;
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
    struct Query<QueryType::Synchronized, Parts...> : public TraitQueryOperations
    {
    public:
        using ObjectType = ice::ecs::QueryObject<Parts...>;
        using ResultType = typename ObjectType::ResultType;
        using BlockResultType = typename ObjectType::BlockResultType;
        using ComponentsTypeList = typename ObjectType::ComponentsTypeList;

        static constexpr ice::ecs::QueryType Type = QueryType::Synchronized;
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

    template<ice::ecs::QueryType Type, typename... Parts>
    auto Query<Type, Parts...>::synchronized_on(ice::TaskScheduler& scheduler) const noexcept
    {
        struct Awaitable : ice::ecs::detail::QueryAwaitableBase<Parts...>
        {
            using ice::ecs::detail::QueryAwaitableBase<Parts...>::QueryAwaitableBase;

            inline auto await_resume() const noexcept -> ice::ecs::Query<QueryType::Synchronized, Parts...>
            {
                return Query<QueryType::Synchronized, Parts...>{ this->query_object(), this->_is_empty == false };
            }
        };

        return Awaitable{ _query, scheduler.schedule()._queue };
    }

    template<typename... Parts> Query(QueryObject<Parts...>&&) noexcept -> Query<QueryType::Unchecked, Parts...>;
    template<typename... Parts> Query(QueryObject<Parts...> const&) noexcept -> Query<QueryType::Unchecked, Parts...>;

} // namespace ice::ecs
