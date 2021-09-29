#pragma once
#include <ice/allocator.hxx>
#include <ice/shard_container.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>

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

        ~EntityStorage() noexcept = default;

        void execute_operations(
            ice::ecs::EntityOperations const& operations
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::ecs::ArchetypeIndex const& _archetype_index;

        ice::pod::Array<ice::ecs::DataBlock*> _data_blocks;

        //struct ArchetypeBlockHead
        //{
        //    ice::ecs::ArchetypeInstanceInfo const* archetype_instance;
        //    ice::ecs::DataBlockPool* archetype_data_pool;
        //    ice::ecs::DataBlock* archetype_data_head;
        //};
        //ice::pod::Hash<ArchetypeBlockHead> _archetype_heads;
    };

} // namespace ice::ecs
