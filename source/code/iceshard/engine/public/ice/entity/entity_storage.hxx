#pragma once
#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_block_allocator.hxx>
#include <ice/entity/entity.hxx>

namespace ice
{

    enum class EntityInstance : ice::u64
    {
        Invalid = 0
    };

    class EntityStorage
    {
    public:
        EntityStorage(
            ice::Allocator& alloc,
            ice::ArchetypeIndex& archetype_index,
            ice::ArchetypeBlockAllocator& archetype_block_alloc
        ) noexcept;
        ~EntityStorage() noexcept;

        void set_archetype(
            ice::Entity entity,
            ice::ArchetypeHandle archetype
        ) noexcept;

        void change_archetype(
            ice::Entity entity,
            ice::ArchetypeHandle archetype
        ) noexcept;


        void set_components(
            ice::Entity entity,
            ice::Span<ice::ArchetypeComponent const> components
        ) noexcept;

        void add_component(
            ice::Entity entity,
            ice::ArchetypeComponent component
        ) noexcept;

        void remove_component(
            ice::Entity entity,
            ice::StringID_Arg component_name
        ) noexcept;

        template<typename... Components>
        void set_components(
            ice::Entity entity
        ) noexcept;

        template<typename Component>
        void add_component(
            ice::Entity entity
        ) noexcept;

        template<typename Component>
        void remove_component(
            ice::Entity entity
        ) noexcept;


        void erase_data(
            ice::Entity entity
        ) noexcept;


        void query_blocks(
            ice::Span<ice::ArchetypeHandle const> archetypes,
            ice::pod::Array<ice::u32>& archetype_block_count,
            ice::pod::Array<ice::ArchetypeBlock*>& archetype_blocks
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::ArchetypeIndex& _archetype_index;
        ice::pod::Hash<ice::ArchetypeBlock*> _archetype_blocks;

        ice::pod::Hash<ice::EntityInstance> _instances;
    };

    template<typename... Components>
    void EntityStorage::set_components(
        ice::Entity entity
    ) noexcept
    {
        static constexpr ice::Archetype<Components...> temp_archetype{ };
        set_components(entity, temp_archetype.components);
    }

    template<typename Component>
    void EntityStorage::add_component(
        ice::Entity entity
    ) noexcept
    {
        static constexpr ArchetypeComponent component_info{
            .name = ice::stringid_hash(ComponentIdentifier<Component>),
            .size = sizeof(Component),
            .alignment = alignof(Component),
        };

        add_component(entity, component_info);
    }

    template<typename Component>
    void EntityStorage::remove_component(
        ice::Entity entity
    ) noexcept
    {
        remove_component(entity, ComponentIdentifier<Component>);
    }

} // namespace ice
