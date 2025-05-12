/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_query_object.hxx>
#include <ice/ecs/ecs_query_provider.hxx>

namespace ice::ecs
{

    class QueryStorageEntry
    {
    public:
        virtual ~QueryStorageEntry() noexcept = default;

        virtual void initialize(ice::ecs::QueryProvider const& provider) noexcept = 0;
    };

    template<typename... T>
    class QueryObjectEntry;

    template<typename... Parts, ice::ecs::QueryTagType... Tags>
    class QueryObjectEntry<ice::ecs::QueryObject<Parts...>, Tags...> : public QueryStorageEntry
    {
    public:
        using QueryObjectType = ice::ecs::QueryObject<Parts...>;
        using QueryObjectEntryType = ice::ecs::QueryObjectEntry<QueryObject<Parts...>, Tags...>;

    public:
        static auto hash_value() noexcept -> ice::u64;

        QueryObjectEntry(ice::Allocator& alloc) noexcept;
        ~QueryObjectEntry() noexcept override = default;

        void initialize(ice::ecs::QueryProvider const& provider) noexcept override;

        auto object_query() const noexcept -> ice::ecs::QueryObject<Parts...> const& { return _query_object; }

    private:
        QueryObjectType _query_object;
    };

    template<typename... Parts, ice::ecs::QueryTagType... Tags>
    auto QueryObjectEntry<ice::ecs::QueryObject<Parts...>, Tags...>::hash_value() noexcept -> ice::u64
    {
        return typeid(QueryObjectEntryType).hash_code();
    }

    template<typename... Parts, ice::ecs::QueryTagType... Tags>
    QueryObjectEntry<ice::ecs::QueryObject<Parts...>, Tags...>::QueryObjectEntry(ice::Allocator& alloc) noexcept
        : _query_object{ alloc }
    {
    }

    template<typename... Parts, ice::ecs::QueryTagType... Tags>
    void QueryObjectEntry<ice::ecs::QueryObject<Parts...>, Tags...>::initialize(ice::ecs::QueryProvider const& provider) noexcept
    {
        if constexpr (sizeof...(Tags) > 0)
        {
            provider.initialize_query_object(_query_object, ice::span::from_std_const(ice::ecs::QueryTagsDefinition<Tags...>::Constant_Tags));
        }
        else
        {
            provider.initialize_query_object(_query_object);
        }
    }

} // namespace ice::ecs
