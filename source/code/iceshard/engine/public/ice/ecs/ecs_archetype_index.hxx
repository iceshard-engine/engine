#pragma once
#include <ice/allocator.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    class DataBlockPool;

    class ArchetypeIndex
    {
    public:
        ArchetypeIndex(
            ice::Allocator& alloc
        ) noexcept;

        ~ArchetypeIndex() noexcept = default;

        auto register_archetype(
            ice::ecs::ArchetypeComponentsInfo const& archetype_components_info,
            ice::ecs::DataBlockPool* component_block_pool = nullptr
        ) noexcept -> ice::ecs::Archetype;

        // #todo Create a archetype component query
        auto find_archetype(
            ice::ecs::ArchetypeComponentsInfo const& components_info
        ) noexcept -> ice::ecs::Archetype;

        auto get_component_block_pool(
            ice::ecs::Archetype archetype
        ) noexcept -> ice::ecs::DataBlockPool&;

    private:
        ice::Allocator& _allocator;
    };

} // namespace ice::ecs
