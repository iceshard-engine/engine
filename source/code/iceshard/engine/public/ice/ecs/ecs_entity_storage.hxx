#pragma once
#include <ice/allocator.hxx>
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_query.hxx>

namespace ice::ecs
{

    class ArchetypeIndex;

    class EntityOperations;

    class EntityStorage
    {
    public:
        EntityStorage(
            ice::Allocator& alloc,
            ice::ecs::ArchetypeIndex const& archetype_index
        ) noexcept;

        ~EntityStorage() noexcept;

        void execute_operations(
            ice::ecs::EntityOperations const& operations,
            ice::ShardContainer& out_shards
        ) noexcept;

        // #todo This might result in a query with nullptr to blocks.
        //  This needs still more development time.
        template<ice::ecs::QueryType... Types>
        auto create_query(
            ice::Allocator& alloc,
            ice::ecs::QueryDefinition<Types...> const& = { }
        ) noexcept -> ice::ecs::Query<ice::ecs::QueryDefinition<Types...>>
        {
            using Definition = ice::ecs::QueryDefinition<Types...>;
            using Query = typename Definition::Query;

            static constexpr Definition definition{ };

            ice::pod::Array<ice::ecs::Archetype> archetypes{ _allocator };
            _archetype_index.find_archetypes(definition.requirements, archetypes);

            ice::pod::Array<ice::ecs::ArchetypeInstanceInfo const*> instance_infos{ alloc };
            ice::pod::Array<ice::ecs::DataBlock const*> data_blocks{ alloc };
            ice::pod::Array<ice::StaticArray<ice::u32, definition.component_count>> argument_idx_map{ alloc };

            ice::u32 const archetype_count = ice::size(archetypes);
            ice::pod::array::resize(instance_infos, archetype_count);
            ice::pod::array::reserve(data_blocks, archetype_count);
            ice::pod::array::reserve(argument_idx_map, archetype_count);

            _archetype_index.fetch_archetype_instance_infos(archetypes, instance_infos);

            for (ice::ecs::ArchetypeInstanceInfo const* instance : instance_infos)
            {
                ice::u32 const instance_idx = static_cast<ice::u32>(instance->archetype_instance);
                ice::pod::array::push_back(data_blocks, _data_blocks[instance_idx]);
                ice::pod::array::push_back(argument_idx_map, ice::ecs::detail::argument_idx_map<Types...>(*instance));
            }

            return Query{
                .archetype_instances = ice::move(instance_infos),
                .archetype_data_blocks = ice::move(data_blocks),
                .archetype_argument_idx_map = ice::move(argument_idx_map),
            };
        }

    private:
        ice::Allocator& _allocator;
        ice::ecs::ArchetypeIndex const& _archetype_index;

        ice::pod::Array<ice::ecs::DataBlock> _head_blocks;
        ice::pod::Array<ice::ecs::DataBlock*> _data_blocks;
    };

} // namespace ice::ecs
