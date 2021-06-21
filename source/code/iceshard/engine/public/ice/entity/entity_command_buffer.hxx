#pragma once
#include <ice/allocator.hxx>
#include <ice/entity/entity.hxx>
#include <ice/archetype/archetype.hxx>

namespace ice
{

    class EntityCommandBuffer
    {
    public:
        EntityCommandBuffer(
            ice::Allocator& alloc
        ) noexcept;

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
            ice::Span<ComponentInfo const> components
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


        void add_tag(
            ice::Entity entity,
            ice::StringID_Arg tag
        ) noexcept;

        void remove_tag(
            ice::Entity entity,
            ice::StringID_Arg tag
        ) noexcept;


        void erase_data(
            ice::Entity entity
        ) noexcept;
    };

} // namespace ice
