#pragma once
#include <ice/memory/stack_allocator.hxx>
#include <ice/entity/entity_component.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/entity/entity_query_utils.hxx>
#include <ice/archetype/archetype_info.hxx>

namespace ice
{

    template<typename T>
    concept ComponentQueryType = (EntityComponent<T> || std::is_same_v<ice::Entity, T>)
        && (std::is_pointer_v<T> || std::is_reference_v<T> || std::is_same_v<ice::Entity, T>);

    namespace detail
    {

        template<ComponentQueryType T>
        using ComponentTypeFromQueryType = std::remove_const_t<std::remove_reference_t<std::remove_pointer_t<T>>>;

        template<typename T>
        struct QueryComponentOptional { };

        template<typename T>
        struct QueryComponentOptional<T&>
        {
            static constexpr bool IsOptional = false;
        };

        template<typename T>
        struct QueryComponentOptional<T*>
        {
            static constexpr bool IsOptional = true;
        };

        template<>
        struct QueryComponentOptional<ice::Entity>
        {
            static constexpr bool IsOptional = false;
        };

        template<typename T>
        struct QueryComponentReadOnly
        {
            static constexpr bool IsReadOnly = false;
        };

        template<typename T>
        struct QueryComponentReadOnly<T const>
        {
            static constexpr bool IsReadOnly = true;
        };

        template<>
        struct QueryComponentReadOnly<ice::Entity>
        {
            static constexpr bool IsReadOnly = true;
        };

        template<typename T>
        struct QueryInvokeHelper { };

        template<typename T>
        struct QueryInvokeHelper<T*>
        {
            using PointerType = T*;

            static constexpr auto ToFinalObject(T* ptr, ice::u32 idx) noexcept -> T*
            {
                return ptr == nullptr ? nullptr : ptr + idx;
            }
        };

        template<typename T>
        struct QueryInvokeHelper<T&>
        {
            using PointerType = T*;

            static constexpr auto ToFinalObject(T* ptr, ice::u32 idx) noexcept -> T&
            {
                return *(ptr + idx);
            }
        };

        template<>
        struct QueryInvokeHelper<ice::Entity>
        {
            using PointerType = ice::Entity*;

            static constexpr auto ToFinalObject(ice::Entity* ptr, ice::u32 idx) noexcept -> ice::Entity
            {
                return *(ptr + idx);
            }
        };

        template<typename Fn, typename... Components>
        void query_invoke(
            Fn&& fn,
            ice::u32 entity_count,
            typename QueryInvokeHelper<Components>::PointerType... component_ptrs
        )
        {
            for (ice::u32 entity_idx = 0; entity_idx < entity_count; ++entity_idx)
            {
                ice::forward<Fn>(fn)(
                    QueryInvokeHelper<Components>::ToFinalObject(component_ptrs, entity_idx)...
                );
            }
        };

        template<typename T>
        struct query_cast { };

        template<>
        struct query_cast<ice::Entity>
        {
            static auto to(
                void** ptr,
                ice::u32& component_idx
            ) noexcept -> ice::Entity*
            {
                component_idx -= 1;
                return reinterpret_cast<ice::Entity*>(ptr[component_idx]);
            }
        };

        template<typename T>
        struct query_cast<T*>
        {
            static constexpr auto to(
                void** ptr,
                ice::u32& component_idx
            ) noexcept -> T*
            {
                component_idx -= 1;
                return reinterpret_cast<T*>(ptr[component_idx]);
            }
        };

        template<typename T>
        struct query_cast<T&>
        {
            static constexpr auto to(
                void** ptr,
                ice::u32& component_idx
            ) noexcept -> T*
            {
                component_idx -= 1;
                return reinterpret_cast<T*>(ptr[component_idx]);
            }
        };

    } // namespace detail

    template<ComponentQueryType... Components>
    struct ComponentQueryInfo
    {
        static constexpr ice::u32 Const_ComponentCount = sizeof...(Components);
        static constexpr ice::StringID Const_Identifiers[Const_ComponentCount]{
            ComponentIdentifier<std::remove_pointer_t<std::remove_reference_t<Components>>>...
        };
        static constexpr ice::StringID_Hash Const_IdentifierHashes[Const_ComponentCount]{
            ice::stringid_hash(ComponentIdentifier<std::remove_pointer_t<std::remove_reference_t<Components>>>)...
        };
        static constexpr bool Const_ComponentOptional[Const_ComponentCount]{
            detail::QueryComponentOptional<Components>::IsOptional...
        };
        static constexpr bool Const_ComponentReadOnly[Const_ComponentCount]{
            detail::QueryComponentReadOnly<
                std::remove_pointer_t<std::remove_reference_t<Components>>
            >::IsReadOnly...
        };
        ice::ArchetypeIndexQuery const index_query{
            .components = ice::Span<ice::StringID const>{ Const_Identifiers, ice::size(Const_Identifiers) },
            .optional = ice::Span<bool const>{ Const_ComponentOptional, ice::size(Const_ComponentOptional) }
        };
    };

    template<ComponentQueryType... Components>
    struct ComponentQuery
    {
        static constexpr ice::ComponentQueryInfo<Components...> Constant_QueryInfo{ };

        struct ResultByEntity;
        struct ResultByBlock;

        inline ComponentQuery(
            ice::Allocator& alloc,
            ice::ArchetypeIndex& index
        ) noexcept;

        inline ComponentQuery(
            ice::ComponentQueryInfo<Components...>,
            ice::Allocator& alloc,
            ice::ArchetypeIndex& index
        ) noexcept;

        inline auto result_by_entity(
            ice::Allocator& alloc,
            ice::EntityStorage& storage
        ) const noexcept -> ResultByEntity;

        inline auto result_by_block(
            ice::Allocator& alloc,
            ice::EntityStorage& storage
        ) const noexcept -> ResultByBlock;

        ice::ArchetypeIndex& _archetype_index;
        ice::pod::Array<ice::ArchetypeHandle> _archetypes;
        ice::pod::Array<ice::ArchetypeInfo> _archetype_infos;
    };

    template<ComponentQueryType... Components>
    ComponentQuery(ice::ComponentQueryInfo<Components...>, ice::Allocator&, ice::ArchetypeIndex&)
        ->ComponentQuery<Components...>;

    template<ComponentQueryType... Components>
    struct ComponentQuery<Components...>::ResultByEntity
    {
        static constexpr ice::ComponentQueryInfo<Components...> Constant_QueryInfo;
        static constexpr ice::u32 Constant_ComponentCount = sizeof...(Components);

        inline ResultByEntity(
            ice::Allocator& alloc,
            ice::pod::Array<ice::ArchetypeInfo> const& infos
        ) noexcept;

        inline auto entity_count() const noexcept -> ice::u32;

        template<typename Fn>
        inline void for_each(Fn&& fn) noexcept;

        ice::pod::Array<ice::ArchetypeInfo> const& _archetype_infos;
        ice::pod::Array<ice::u32> _archetype_block_count;
        ice::pod::Array<ice::ArchetypeBlock*> _archetype_blocks;
    };

    template<ComponentQueryType... Components>
    struct ComponentQuery<Components...>::ResultByBlock
    {
        static constexpr ice::ComponentQueryInfo<Components...> Constant_QueryInfo;
        static constexpr ice::u32 Constant_ComponentCount = sizeof...(Components);

        inline ResultByBlock(
            ice::Allocator& alloc,
            ice::pod::Array<ice::ArchetypeInfo> const& infos
        ) noexcept;

        inline auto entity_count() const noexcept -> ice::u32;

        template<typename Fn>
        inline void for_each(Fn&& fn) noexcept;

        ice::pod::Array<ice::ArchetypeInfo> const& _archetype_infos;
        ice::pod::Array<ice::u32> _archetype_block_count;
        ice::pod::Array<ice::ArchetypeBlock*> _archetype_blocks;

    };

    template<ComponentQueryType... Components>
    inline ComponentQuery<Components...>::ComponentQuery(
        ice::Allocator& alloc,
        ice::ArchetypeIndex& index
    ) noexcept
        : _archetype_index{ index }
        , _archetypes{ alloc }
        , _archetype_infos{ alloc }
    {
        if (index.find_archetypes(Constant_QueryInfo.index_query, _archetypes))
        {
            ice::pod::array::resize(_archetype_infos, ice::size(_archetypes));
            index.archetype_info(
                _archetypes,
                _archetype_infos
            );
        }
    }

    template<ComponentQueryType... Components>
    inline ComponentQuery<Components...>::ComponentQuery(
        ice::ComponentQueryInfo<Components...>,
        ice::Allocator& alloc,
        ice::ArchetypeIndex& index
    ) noexcept
        : ComponentQuery{ alloc, index }
    { }

    template<ComponentQueryType... Components>
    inline auto ComponentQuery<Components...>::result_by_entity(
        ice::Allocator& alloc,
        ice::EntityStorage& storage
    ) const noexcept -> ResultByEntity
    {
        ResultByEntity result{ alloc, _archetype_infos };
        storage.query_blocks(
            _archetypes,
            result._archetype_block_count,
            result._archetype_blocks
        );
        return result;
    }

    template<ComponentQueryType... Components>
    inline auto ComponentQuery<Components...>::result_by_block(
        ice::Allocator& alloc,
        ice::EntityStorage& storage
    ) const noexcept -> ResultByBlock
    {
        ResultByBlock result{ alloc, _archetype_infos };
        storage.query_blocks(
            _archetypes,
            result._archetype_block_count,
            result._archetype_blocks
        );
        return result;
    }

    template<ComponentQueryType... Components>
    inline ComponentQuery<Components...>::ResultByEntity::ResultByEntity(
        ice::Allocator& alloc,
        ice::pod::Array<ice::ArchetypeInfo> const& infos
    ) noexcept
        : _archetype_infos{ infos }
        , _archetype_block_count{ alloc }
        , _archetype_blocks{ alloc }
    { }

    template<ComponentQueryType... Components>
    inline auto ComponentQuery<Components...>::ResultByEntity::entity_count() const noexcept -> ice::u32
    {
        ice::ArchetypeBlock* const* block_it = ice::pod::begin(_archetype_blocks);
        ice::ArchetypeBlock* const* const block_end = ice::pod::end(_archetype_blocks);

        ice::u32 result = 0;
        while (block_it != block_end)
        {
            result += (*block_it)->entity_count;
            block_it += 1;
        }
        return result;
    }

    template<ComponentQueryType... Components>
    template<typename Fn>
    inline void ComponentQuery<Components...>::ResultByEntity::for_each(Fn&& fn) noexcept
    {
        void* block_pointers[Constant_ComponentCount]{};

        ice::ArchetypeBlock** block_it = ice::pod::begin(_archetype_blocks);

        ice::u32 archetype_idx = 0;
        for (ice::ArchetypeInfo const& archetype : _archetype_infos)
        {
            auto const idx_map = ice::detail::map_arguments_to_archetype_index<detail::ComponentTypeFromQueryType<Components>...>(archetype);

            ice::u32 block_count = _archetype_block_count[archetype_idx];
            while (block_count > 0)
            {
                ice::ArchetypeBlock* block = *block_it;
                for (ice::u32 idx = 0; idx < Constant_ComponentCount; ++idx)
                {
                    if (idx_map[idx] == std::numeric_limits<ice::u32>::max())
                    {
                        block_pointers[idx] = nullptr;
                    }
                    else
                    {
                        block_pointers[idx] = ice::memory::ptr_add(
                            block->block_data,
                            archetype.offsets[idx_map[idx]]
                        );
                    }
                }

                ice::u32 component_index = Constant_ComponentCount;
                detail::query_invoke<Fn, Components...>(
                    std::forward<Fn>(fn),
                    block->entity_count,
                    detail::query_cast<Components>::to(
                        block_pointers, component_index
                    )...
                );

                block_it += 1;
                block_count -= 1;
            }
            archetype_idx += 1;
        }
    }

    template<ComponentQueryType... Components>
    inline ComponentQuery<Components...>::ResultByBlock::ResultByBlock(
        ice::Allocator& alloc,
        ice::pod::Array<ice::ArchetypeInfo> const& infos
    ) noexcept
        : _archetype_infos{ infos }
        , _archetype_block_count{ alloc }
        , _archetype_blocks{ alloc }
    { }

    template<ComponentQueryType... Components>
    inline auto ComponentQuery<Components...>::ResultByBlock::entity_count() const noexcept -> ice::u32
    {
        ice::ArchetypeBlock* const* block_it = ice::pod::begin(_archetype_blocks);
        ice::ArchetypeBlock* const* const block_end = ice::pod::end(_archetype_blocks);

        ice::u32 result = 0;
        while (block_it != block_end)
        {
            result += (*block_it)->entity_count;
            block_it += 1;
        }
        return result;
    }

    template<ComponentQueryType... Components>
    template<typename Fn>
    inline void ComponentQuery<Components...>::ResultByBlock::for_each(Fn&& fn) noexcept
    {
        void* block_pointers[Constant_ComponentCount]{};

        ice::ArchetypeBlock** block_it = ice::pod::begin(_archetype_blocks);

        ice::u32 archetype_idx = 0;
        for (ice::ArchetypeInfo const& archetype : _archetype_infos)
        {
            auto const idx_map = ice::detail::map_arguments_to_archetype_index<detail::ComponentTypeFromQueryType<Components>...>(archetype);

            ice::u32 block_count = _archetype_block_count[archetype_idx];
            while (block_count > 0)
            {
                ice::ArchetypeBlock* block = *block_it;
                for (ice::u32 idx = 0; idx < Constant_ComponentCount; ++idx)
                {
                    if (idx_map[idx] == std::numeric_limits<ice::u32>::max())
                    {
                        block_pointers[idx] = nullptr;
                    }
                    else
                    {
                        block_pointers[idx] = ice::memory::ptr_add(
                            block->block_data,
                            archetype.offsets[idx_map[idx]]
                        );
                    }
                }

                ice::u32 component_index = Constant_ComponentCount;

                std::forward<Fn>(fn)(
                    block->entity_count,
                    detail::query_cast<Components>::to(
                        block_pointers, component_index
                    )...
                );

                block_it += 1;
                block_count -= 1;
            }
            archetype_idx += 1;
        }
    }

} // namespace ice
