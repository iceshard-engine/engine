#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <iceshard/entity/entity.hxx>
#include <iceshard/component/component_archetype_index.hxx>

namespace iceshard::ecs
{

    template<typename... Components>
    struct ComponentQuery
    {
        static inline core::stringid_type ComponentIdentifiers[] = {
            std::remove_pointer_t<std::remove_cv_t<Components>>::identifier...
        };
        static constexpr uint64_t ComponentIdentifierHashes[] = {
            core::hash(std::remove_pointer_t<std::remove_cv_t<Components>>::identifier)...
        };

        ComponentQuery(core::allocator& alloc) noexcept
            : _allocator{ alloc }
        {
            core::pod::array::create_view(_components_view, &ComponentIdentifiers[0], core::size(ComponentIdentifiers));
        }
        ~ComponentQuery() noexcept = default;

        auto allocator() const noexcept -> core::allocator& { return _allocator; }

        auto components() const noexcept -> core::pod::Array<core::stringid_type> const& { return _components_view; }

        template<typename Fn>
        static void apply(Fn&& fn, uint32_t count, core::pod::Hash<void*> const& pointers) noexcept
        {
            std::forward<Fn>(fn)(
                count,
                reinterpret_cast<Components>(
                    core::pod::hash::get(
                        pointers,
                        core::hash(std::remove_pointer_t<Components>::identifier),
                        nullptr
                    )
                )...
            );
        }

        template<typename Fn>
        static void apply_for_each(Fn&& fn, uint32_t count, core::pod::Hash<void*> const& pointers) noexcept
        {
            for (uint32_t idx = 0; idx < count; ++idx)
            {
                std::forward<Fn>(fn)(
                    (reinterpret_cast<Components>(
                        core::pod::hash::get(
                            pointers,
                            core::hash(std::remove_pointer_t<Components>::identifier),
                            nullptr
                        )
                    ) + idx)...
                );
            }
        }

        enum class EOperation
        {
            QueryFull,
            EntityAddSingle,
            EntityAddMany,
        };

        class Result;
    private:
        core::allocator& _allocator;
        core::pod::Array<core::stringid_type> _components_view{ core::memory::globals::null_allocator() };
    };

    template<typename... Components>
    class ComponentQuery<Components...>::Result
    {
    public:
        using QueryType = iceshard::ecs::ComponentQuery<Components...>;

        Result(ComponentQuery const& qry, iceshard::ecs::ArchetypeIndex& index) noexcept
            : _query{ qry }
            , _index{ index }
            , _archetype_block_count{ _query.allocator() }
            , _archetype_component_offsets{ _query.allocator() }
            , _blocks{ _query.allocator() }
        {
            _index.query_instances(
                _query.components(),
                _archetype_block_count,
                _archetype_component_offsets,
                _blocks
            );
        }
        ~Result() noexcept = default;

        Result(Result&&) noexcept = delete;
        Result(Result const&) noexcept = delete;

        auto operator=(Result&&) noexcept -> Result& = delete;
        auto operator=(Result const&) noexcept -> Result& = delete;

        auto query() const noexcept -> QueryType const&
        {
            return _query;
        }

        auto archetype_block_count() const noexcept -> core::pod::Array<uint32_t> const&
        {
            return _archetype_block_count;
        }

        auto archetype_component_offsets() const noexcept -> core::pod::Array<uint32_t> const&
        {
            return _archetype_block_count;
        }

        auto blocks() const noexcept -> core::pod::Array<uint32_t> const&
        {
            return _archetype_block_count;
        }

        template<typename Fn>
        auto for_each_archetype(Fn&& fn) noexcept
        {
            uint32_t const component_count = core::pod::array::size(_query.components());
            uint32_t const archetype_count = core::pod::array::size(_archetype_block_count);
            uint32_t block_base_idx = 0;

            core::pod::Array<iceshard::ComponentBlock*> blocks_view{ core::memory::globals::null_allocator() };
            core::pod::Array<uint32_t> offsets_view{ core::memory::globals::null_allocator() };

            for (uint32_t idx = 0; idx < archetype_count; ++idx)
            {
                uint32_t const block_count = _archetype_block_count[idx];

                core::pod::array::create_view(blocks_view, &_blocks[block_base_idx], block_count);
                core::pod::array::create_view(offsets_view, &_archetype_component_offsets[idx * component_count], component_count);

                std::forward<Fn>(fn)(blocks_view, offsets_view);

                block_base_idx += block_count;
            }
        }

    private:
        QueryType const& _query;
        iceshard::ecs::ArchetypeIndex& _index;

        core::pod::Array<uint32_t> _archetype_block_count;
        core::pod::Array<uint32_t> _archetype_component_offsets;
        core::pod::Array<iceshard::ComponentBlock*> _blocks{ alloc };
    };

    template<typename Q>
    auto query_index(Q const& qry, iceshard::ecs::ArchetypeIndex& index) noexcept -> typename Q::Result
    {
        return typename Q::Result{ qry, index };
    }

    template<typename Qr, typename Fn>
    void for_each_block(Qr&& query_result, Fn&& fn) noexcept
    {
        using QueryType = typename std::remove_reference_t<Qr>::QueryType;

        constexpr uint32_t component_count = core::size(QueryType::ComponentIdentifiers);

        core::memory::stack_allocator_512 temp_alloc;
        core::pod::Hash<void*> pointers{ temp_alloc };

        query_result.for_each_archetype([&pointers, &fn](core::pod::Array<iceshard::ComponentBlock*> const& blocks, core::pod::Array<uint32_t> const& offsets) noexcept
            {
                for (iceshard::ComponentBlock* block : blocks)
                {
                    for (uint32_t idx = 0; idx < component_count; ++idx)
                    {
                        core::pod::hash::set(
                            pointers,
                            QueryType::ComponentIdentifierHashes[idx],
                            core::memory::utils::pointer_add(block, offsets[idx])
                        );
                    }

                    QueryType::apply(std::forward<Fn>(fn), block->_entity_count, pointers);
                }
            });
    }

    template<typename Qr, typename Fn>
    void for_each_entity(Qr&& query_result, Fn&& fn) noexcept
    {
        using QueryType = typename std::remove_reference_t<Qr>::QueryType;

        constexpr uint32_t component_count = core::size(QueryType::ComponentIdentifiers);

        core::memory::stack_allocator_512 temp_alloc;
        core::pod::Hash<void*> pointers{ temp_alloc };

        query_result.for_each_archetype([&pointers, &fn](core::pod::Array<iceshard::ComponentBlock*> const& blocks, core::pod::Array<uint32_t> const& offsets) noexcept
            {
                for (iceshard::ComponentBlock* block : blocks)
                {
                    for (uint32_t idx = 0; idx < component_count; ++idx)
                    {
                        core::pod::hash::set(
                            pointers,
                            QueryType::ComponentIdentifierHashes[idx],
                            core::memory::utils::pointer_add(block, offsets[idx])
                        );
                    }

                    QueryType::apply_for_each(std::forward<Fn>(fn), block->_entity_count, pointers);
                }
            });
    }

    template<typename Q, typename Fn>
    void for_entity(Q const& qry, iceshard::ecs::ArchetypeIndex& index, iceshard::Entity entity, Fn && fn) noexcept
    {
        using QueryType = Q;

        constexpr uint32_t component_count = core::size(QueryType::ComponentIdentifiers);

        core::pod::Array<uint32_t> offsets{ qry.allocator() };
        iceshard::ComponentBlock* block = nullptr;
        uint32_t block_index = 0;

        bool const query_successful = index.query_instance(
            entity,
            qry.components(),
            offsets,
            block,
            block_index
        );

        if (query_successful)
        {
            core::memory::stack_allocator_512 temp_alloc;
            core::pod::Hash<void*> pointers{ temp_alloc };

            for (uint32_t idx = 0; idx < component_count; ++idx)
            {
                core::pod::hash::set(
                    pointers,
                    QueryType::ComponentIdentifierHashes[idx],
                    core::memory::utils::pointer_add(block, offsets[idx])
                );
            }

            QueryType::apply_for_each(std::forward<Fn>(fn), 1, pointers);
        }
    }

} // namespace iceshard::ecs
