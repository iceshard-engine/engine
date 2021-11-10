#pragma once
#include <ice/allocator.hxx>
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_query.hxx>
#include <ice/ecs/ecs_query_provider.hxx>

namespace ice::ecs
{

    class ArchetypeIndex;

    class EntityOperations;

    class EntityStorage : public ice::ecs::QueryProvider
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

    protected:
        void query_internal(
            ice::Span<ice::ecs::detail::QueryTypeInfo const> query_info,
            ice::pod::Array<ice::ecs::ArchetypeInstanceInfo const*>& out_instance_infos,
            ice::pod::Array<ice::ecs::DataBlock const*>& out_data_blocks
        ) const noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::ecs::ArchetypeIndex const& _archetype_index;

        ice::pod::Array<ice::ecs::DataBlock> _head_blocks;
        ice::pod::Array<ice::ecs::DataBlock*> _data_blocks;
    };

} // namespace ice::ecs
