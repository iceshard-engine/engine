#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>

namespace ice::ecs
{

    struct QueryStorageEntry
    {
        virtual ~QueryStorageEntry() noexcept = default;

        virtual void initialize(ice::ecs::QueryProvider const& provider) noexcept = 0;
    };

    template<ice::ecs::QueryType... Types>
    struct QueryEntry : QueryStorageEntry
    {
        using QueryDefinition = ice::ecs::QueryDefinition<Types...>;

        ice::u64 id;
        ice::ecs::Query<QueryDefinition> query;

        QueryEntry(ice::Allocator& alloc) noexcept
            : id{ typeid(QueryDefinition).hash_code() }
            , query{ alloc }
        { }

        void initialize(ice::ecs::QueryProvider const& provider) noexcept override
        {
            provider.initialize_query(query);
        }

        static auto query_hash() noexcept -> ice::u64
        {
            return typeid(QueryDefinition).hash_code();
        }
    };

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
        ) noexcept -> ice::ecs::Query<ice::ecs::QueryDefinition<Types...>> const&;

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
    ) noexcept -> ice::ecs::Query<ice::ecs::QueryDefinition<Types...>> const&
    {
        using QueryEntry = ice::ecs::QueryEntry<Types...>;
        using QueryDefinition = QueryEntry::QueryDefinition;

        ice::u64 const query_hash = QueryEntry::query_hash();
        QueryDefinition constexpr query_definition{ };

        if (ice::hashmap::has(_queries, query_hash) == false)
        {
            ice::UniquePtr<ice::ecs::QueryStorageEntry> new_entry = ice::make_unique<QueryEntry>(_allocator, _allocator);
            new_entry->initialize(_provider);
            ice::hashmap::set(_queries, query_hash, ice::move(new_entry));
        }

        ice::UniquePtr<ice::ecs::QueryStorageEntry> const& entry = *ice::hashmap::try_get(_queries, query_hash);
        return static_cast<QueryEntry const*>(entry.get())->query;
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
