/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>
#include <ice/ecs/ecs_query_builder.hxx>

namespace ice::ecs
{

    class QueryStorage
    {
    public:
        QueryStorage(
            ice::Allocator& alloc,
            ice::ecs::QueryProvider const& query_provider
        ) noexcept;
        ~QueryStorage() noexcept;

        auto query_provider() const noexcept -> ice::ecs::QueryProvider const&;

        template<ice::ecs::QueryType... Types>
        auto build(
            ice::ecs::QueryDefinition<Types...> const& = { }
        ) noexcept -> ice::ecs::QueryBuilder<QueryObject<ice::ecs::detail::QueryObjectPart<0, Types...>>>;

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
    { }

    inline QueryStorage::~QueryStorage() noexcept
    {
        ice::hashmap::clear(_queries);
    }

    inline auto QueryStorage::query_provider() const noexcept -> ice::ecs::QueryProvider const&
    {
        return _provider;
    }

    template<ice::ecs::QueryType... Types>
    inline auto QueryStorage::build(
        ice::ecs::QueryDefinition<Types...> const&
    ) noexcept -> ice::ecs::QueryBuilder<QueryObject<ice::ecs::detail::QueryObjectPart<0, Types...>>>
    {
        return { _allocator, _queries, _provider };
    }

} // namespace ice::ecs
