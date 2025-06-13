/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_storage_entry.hxx>
#include <ice/container/hashmap.hxx>

namespace ice::ecs
{

    template<typename... Parts>
    class QueryBuilder;

    template<typename... Parts, ice::ecs::QueryTagType... Tags>
    class QueryBuilder<ice::ecs::QueryObject<Parts...>, Tags...> : public ice::ecs::TraitQueryOperations
    {
    public:
        //! \brief
        template<ice::u32 ReferencedIdx, ice::ecs::QueryArg... Components>
        using Part = ice::ecs::detail::QueryObjectPart<ReferencedIdx, Components...>;

        //! \brief
        using Entry = ice::ecs::QueryObjectEntry<ice::ecs::QueryObject<Parts...>, Tags...>;

        //! \brief
        using ObjectType = ice::ecs::QueryObject<Parts...>;
        using ResultType = typename ObjectType::ResultType;
        using BlockResultType = typename ObjectType::BlockResultType;

        static constexpr ice::ecs::QueryType Type = QueryType::Unchecked;
        static constexpr ice::u32 ComponentCount = ObjectType::ComponentCount;

        QueryBuilder(
            ice::Allocator& alloc,
            ice::HashMap<ice::UniquePtr<ice::ecs::QueryStorageEntry>>& queries,
            ice::ecs::QueryProvider const& query_provider
        ) noexcept
            : _allocator{ alloc }
            , _queries{ queries }
            , _query_provider{ query_provider }
        { }

        ~QueryBuilder() noexcept = default;

        template<ice::u32 ReferencedIdx, ice::ecs::QueryArg... Components>
        auto with() const noexcept -> ice::ecs::QueryBuilder<ice::ecs::QueryObject<Parts..., Part<ReferencedIdx, Components...>>, Tags...>
        {
            return { _allocator, _queries, _query_provider };
        }

        template<ice::ecs::QueryTagType... NewTags>
        auto tags() const noexcept -> ice::ecs::QueryBuilder<ice::ecs::QueryObject<Parts...>, NewTags...>
        {
            static_assert(sizeof...(Tags) == 0, "Setting tags more than once for a query is not supported!");
            return { _allocator, _queries, _query_provider };
        }

        auto query_object() const noexcept -> ice::ecs::QueryObject<Parts...> const&
        {
            if (ice::hashmap::has(_queries, Entry::hash_value()) == false)
            {
                ice::UniquePtr entry = ice::make_unique<Entry>(_allocator, _allocator);
                entry->initialize(_query_provider);
                ice::hashmap::set(_queries, Entry::hash_value(), ice::move(entry));
            }

            return static_cast<Entry const*>(
                ice::hashmap::try_get(_queries, Entry::hash_value())->get()
            )->object_query();
        }

        auto filter_object() const noexcept -> ice::ecs::detail::DataBlockFilter::QueryFilter
        {
            return {};
        }

        auto synchronized_on(ice::TaskScheduler& scheduler) const noexcept
        {
            struct Awaitable : ice::ecs::detail::QueryAwaitableBase<Parts...>
            {
                using ice::ecs::detail::QueryAwaitableBase<Parts...>::QueryAwaitableBase;

                inline auto await_resume() const noexcept -> ice::ecs::Query<QueryType::Synchronized, Parts...>
                {
                    return Query<QueryType::Synchronized, Parts...>{ this->query_object(), this->_is_empty == false };
                }
            };

            return Awaitable{ query_object(), scheduler.schedule()._queue };
        }

        auto filtered(ice::ecs::Archetype arch) noexcept -> ice::ecs::Query<QueryType::Unchecked, Parts...>
        {
            Query result = ice::ecs::Query<QueryType::Unchecked, Parts...>{ query_object() };
            result.filtered(arch);
            return result;
        }

        template<ice::ecs::detail::FilterType T>
        auto filtered(T const& filter) noexcept -> ice::ecs::Query<QueryType::Unchecked, Parts...>
        {
            Query result = ice::ecs::Query<QueryType::Unchecked, Parts...>{ query_object() };
            result.filtered(filter);
            return result;
        }

        operator ice::ecs::Query<QueryType::Unchecked, Parts...> () const noexcept
        {
            return ice::ecs::Query{ query_object() };
        }

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::UniquePtr<ice::ecs::QueryStorageEntry>>& _queries;
        ice::ecs::QueryProvider const& _query_provider;
    };

    template<typename... Parts, ice::ecs::QueryTagType... Tags>
    Query(ice::ecs::QueryBuilder<QueryObject<Parts...>, Tags...>) -> Query<QueryType::Unchecked, Parts...>;

} // namespace ice::ecs
