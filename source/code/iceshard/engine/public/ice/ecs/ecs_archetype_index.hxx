#pragma once
#include <ice/allocator.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/ecs/ecs_data_block_pool.hxx>
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
            ice::ecs::ArchetypeInfo const& archetype_info,
            ice::ecs::DataBlockPool* data_block_pool = nullptr
        ) noexcept -> ice::ecs::Archetype;

        // #todo Create a archetype component query
        void find_archetypes(
            ice::ecs::ArchetypeInfo const& components_info,
            ice::pod::Array<ice::ecs::Archetype>& out_archetypes
        ) noexcept;

        auto get_component_block_pool(
            ice::ecs::Archetype archetype
        ) noexcept -> ice::ecs::DataBlockPool&;

    private:
        ice::Allocator& _allocator;
        ice::ecs::DataBlockPool _default_block_pool;

        ice::pod::Hash<ice::u32> _archetype_index;

        struct ArchetypeDataHeader;
        ice::pod::Array<ArchetypeDataHeader*> _archetype_data;
    };

} // namespace ice::ecs
