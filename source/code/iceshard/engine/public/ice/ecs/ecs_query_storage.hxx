#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>

namespace ice::ecs
{

    class QueryStorage;

    class QueryStorageEntry
    {
    public:
        virtual ~QueryStorageEntry() noexcept = default;

        virtual void initialize(ice::ecs::QueryProvider const& provider) noexcept = 0;
    };

    template<typename... QueryParts>
    class QueryEntry_v2 : public QueryStorageEntry
    {
    public:
        using QueryType = ice::ecs::Query_v2<QueryParts...>;
        //using QueryDefinition = ice::ecs::QueryDefinition<Types...>;

        ice::u64 id;
        ice::ecs::Query_v2<QueryParts...> query;

    public:
        QueryEntry_v2(ice::Allocator& alloc) noexcept
            : id{ typeid(QueryType).hash_code() }
            , query{ alloc }
        {
        }

        void initialize(ice::ecs::QueryProvider const& provider) noexcept override
        {
            //provider.initialize_query_v2(query);
            //provider.initialize_query(query);
        }

        static auto query_hash() noexcept -> ice::u64
        {
            return typeid(QueryType).hash_code();
        }
    };

    template<typename... QueryParts>
    class QueryBuilder
    {
    public:
        using QueryEntryType = ice::ecs::QueryEntry_v2<QueryParts...>;

        QueryBuilder(
            ice::Allocator& alloc,
            ice::HashMap<ice::UniquePtr<ice::ecs::QueryStorageEntry>>& queries,
            ice::ecs::QueryProvider const& query_provider
        ) noexcept
            : _allocator{ alloc }
            , _entry{ ice::make_unique<ice::ecs::QueryEntry_v2<QueryParts...>>(alloc, alloc) }
            , _queries{ queries }
            , _query_provider{ query_provider }
        { }

        ~QueryBuilder() noexcept
        {
        }

        template<ice::u32 RefIdx, ice::ecs::QueryType... QueryComponents>
        auto with() const noexcept -> QueryBuilder<QueryParts..., QueryPart<RefIdx, QueryComponents...>>
        {
            return { _allocator, _queries, _query_provider };
        }

        template<ice::ecs::ComponentTag... Tags>
        auto tags() noexcept -> QueryBuilder&
        {
            //ice::ecs::QueryDefinition<QueryComponents..., Tags const&...> definition;
            return *this;
        }

        operator ice::ecs::Query_v2<QueryParts...> const& () noexcept
        {
            if (ice::hashmap::has(_queries, QueryEntryType::query_hash()) == false)
            {
                _query_provider.initialize_query_v2(_entry->query);
                ice::hashmap::set(_queries, QueryEntryType::query_hash(), ice::move(_entry));
            }

            return static_cast<QueryEntryType const*>(
                ice::hashmap::get(_queries, QueryEntryType::query_hash(), ice::UniquePtr<ice::ecs::QueryStorageEntry>{}).get()
            )->query;
        }

    private:
        ice::Allocator& _allocator;
        ice::UniquePtr<QueryEntryType> _entry;
        ice::HashMap<ice::UniquePtr<ice::ecs::QueryStorageEntry>>& _queries;
        ice::ecs::QueryProvider const& _query_provider;
    };

    template<ice::ecs::QueryType... Types>
    class QueryEntry : public QueryStorageEntry
    {
    public:
        using QueryDefinition = ice::ecs::QueryDefinition<Types...>;

        ice::u64 id;
        ice::ecs::Query<Types...> query;

    public:
        QueryEntry(ice::Allocator& alloc) noexcept
            : id{ typeid(QueryDefinition).hash_code() }
            , query{ alloc }
        {
        }

        void initialize(ice::ecs::QueryProvider const& provider) noexcept override
        {
            provider.initialize_query(query);
        }

        static auto query_hash() noexcept -> ice::u64
        {
            return typeid(QueryDefinition).hash_code();
        }
    };

#if 0
    template<ice::ecs::QueryType... QueryComponents>
    class QueryBuilder
    {
    public:
        QueryBuilder(ice::Allocator& alloc, ice::ecs::QueryStorage& storage) noexcept
            : _allocator{ alloc }
            , _storage{ storage }
            , _query{ _allocator }
        { }

#if 0
        QueryBuilder(QueryBuilder&& other) noexcept
            : _allocator{ other._allocator }
            , _storage{ other._storage }
            , _query{ other._query }
        { }

        auto operator=(QueryBuilder&& other) noexcept
        {
            if (ice::addressof(other) != this)
            {
                ICE_ASSERT_CORE(ice::addressof(other._allocator) == ice::addressof(_allocator));
                ICE_ASSERT_CORE(ice::addressof(other._storage) == ice::addressof(_storage));
                _query = ice::move(other._query);
            }
            return *this;
        }
#endif

        template<ice::u32 RefIdx, ice::ecs::QueryType... SubQueryComponents>
            requires (sizeof...(SubQueryComponents) > 0)
        auto with() noexcept -> ice::ecs::QueryBuilder<QueryComponents..., SubQueryComponents...>
        {
            using Definition = ice::ecs::QueryDefinition<SubQueryComponents...>;
            static constexpr Definition definition{ };

            ice::ecs::QueryBuilder<QueryComponents..., SubQueryComponents...> expanded{ _allocator, _storage };

            // Copy the original query information into the expanded query, before actually expanding it.
            ice::ecs::Query<QueryComponents..., SubQueryComponents...>& newquery = expanded._query;
            newquery.provider = _query.provider;
            newquery.archetype_instances = _query.archetype_instances;
            newquery.archetype_data_blocks = _query.archetype_data_blocks;
            newquery.archetype_argument_idx_map = _query.archetype_argument_idx_map;
            ice::memcpy(ice::span::memory(newquery.access_trackers), ice::span::data_view(_query.access_trackers));


            //provider->expand_query<QueryComponents..., SubQueryComponents...>(*this);
            //static_assert(
            //    (sizeof...(QueryComponents) + sizeof...(SubQueryComponents)) <= 8,
            //    "Queries support access up to 8 components (including linked components)"
            //);
            return expanded;
        }

        inline operator ice::ecs::Query<QueryComponents...>& () && noexcept
        {
            return ice::move(_query);
        }

    private:
        ice::Allocator& _allocator;
        ice::ecs::QueryStorage& _storage;
        ice::ecs::Query<QueryComponents...> _query;
    };
#endif

    class QueryStorage
    {
    public:
        QueryStorage(
            ice::Allocator& alloc,
            ice::ecs::QueryProvider const& query_provider
        ) noexcept;

        virtual ~QueryStorage() noexcept = default;

        template<ice::ecs::QueryType... Types>
        auto get(
            ice::ecs::QueryDefinition<Types...> const& = { }
        ) noexcept -> ice::ecs::Query<Types...> const&;

        template<ice::ecs::QueryType... Types>
        auto build(
            ice::ecs::QueryDefinition<Types...> const& = { }
        ) noexcept -> ice::ecs::QueryBuilder<QueryPart<0, Types...>>;

    private:
        ice::Allocator& _allocator;
        ice::ecs::QueryProvider const& _provider;
        ice::HashMap<ice::UniquePtr<ice::ecs::QueryStorageEntry>> _queries;
    };

    inline QueryStorage::QueryStorage(
        ice::Allocator& alloc,
        ice::ecs::QueryProvider const& query_provider
    ) noexcept
        : _allocator{ alloc }
        , _provider{ query_provider }
        , _queries{ _allocator }
    {
    }

    template<ice::ecs::QueryType... Types>
    auto QueryStorage::get(
        ice::ecs::QueryDefinition<Types...> const&
    ) noexcept -> ice::ecs::Query<Types...> const&
    {
        using QueryEntry = ice::ecs::QueryEntry<Types...>;
        using QueryDefinition = QueryEntry::QueryDefinition;

        ice::u64 const query_hash = QueryEntry::query_hash();

        if (ice::hashmap::has(_queries, query_hash) == false)
        {
            ice::UniquePtr<ice::ecs::QueryStorageEntry> new_entry = ice::make_unique<QueryEntry>(_allocator, _allocator);
            new_entry->initialize(_provider);
            ice::hashmap::set(_queries, query_hash, ice::move(new_entry));
        }

        ice::UniquePtr<ice::ecs::QueryStorageEntry> const& entry = *ice::hashmap::try_get(_queries, query_hash);
        return static_cast<QueryEntry const*>(entry.get())->query;
    }

    template<ice::ecs::QueryType... Types>
    auto QueryStorage::build(
        ice::ecs::QueryDefinition<Types...> const&
    ) noexcept -> ice::ecs::QueryBuilder<QueryPart<0, Types...>>
    {
        return { _allocator, _queries, _provider };
    }

    namespace query_v2
    {

        template<typename... QueryParts>
        inline auto for_each_entity(
            ice::ecs::QueryBuilder<QueryParts...>&& query_builder
        ) noexcept -> ice::Generator<typename ice::ecs::Query_v2<QueryParts...>::QueryTupleResult>
        {
            return ice::ecs::query_v2::for_each_entity(
                static_cast<ice::ecs::Query_v2<QueryParts...> const&>(query_builder)
            );
        }

    }

    namespace query
    {

        template<ice::ecs::QueryType... QueryComponents>
        inline auto entity_data(
            ice::ecs::QueryStorage& queries,
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>
        {
            return ice::ecs::query::entity_data(queries.get<QueryComponents...>(), entity);
        }

#if 0
        template<ice::ecs::QueryType... QueryComponents, ice::ecs::QueryType... SubQueryComponents>
        inline auto entity_data(
            ice::ecs::Query<QueryComponents...> const& query,
            ice::ecs::Query<SubQueryComponents...> const& sub_query,
            ice::ecs::Entity entity
        ) noexcept -> ice::ecs::detail::QueryEntityTupleResult<QueryComponents..., SubQueryComponents...>
        {
            auto first_tuple = ice::ecs::query::entity_data(query, entity);
            auto sub_entity = std::get<sizeof...(QueryComponents) - 1>(first_tuple)->entity;
            return std::tuple_cat(ice::move(first_tuple), ice::ecs::query::entity_data(sub_query, sub_entity));
        }
#endif

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_entity(
            ice::ecs::QueryStorage& queries
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryEntityTupleResult<QueryComponents...>>
        {
            return ice::ecs::query::for_each_entity(queries.get<QueryComponents...>());
        }

        template<ice::ecs::QueryType... QueryComponents>
        inline auto for_each_block(
            ice::ecs::QueryStorage& queries
        ) noexcept -> ice::Generator<ice::ecs::detail::QueryBlockTupleResult<QueryComponents...>>
        {
            return ice::ecs::query::for_each_block(queries.get<QueryComponents...>());
        }

    } // namespace query

} // namespace ice::ecs
