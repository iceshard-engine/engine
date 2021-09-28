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
    };

} // namespace ice::ecs
