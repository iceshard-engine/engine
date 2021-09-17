#pragma once
#include <ice/allocator.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/ecs/ecs_archetype.hxx>

namespace ice::ecs
{

    class ArchetypeIndex;

    class EntityStorage
    {
    public:
        EntityStorage(
            ice::Allocator& alloc,
            ice::ecs::ArchetypeIndex const& archetype_index
        ) noexcept;

        ~EntityStorage() noexcept = default;

    private:
        ice::Allocator& _allocator;
        ice::ecs::ArchetypeIndex& _archetype_index;
    };

} // namespace ice::ecs
