#pragma once
#include <ice/stringid.hxx>
#include <ice/pod/hash.hxx>
#include <ice/entity/entity_index.hxx>

namespace ice
{

    class EntityTracker final
    {
    public:
        EntityTracker(
            ice::Allocator& alloc,
            ice::EntityIndex& index
        ) noexcept;
        ~EntityTracker() noexcept;

        bool create_entity(
            ice::StringID_Arg name,
            ice::Entity& entity_out
        ) noexcept;

        bool find_entity(
            ice::StringID_Arg name,
            ice::Entity& entity_out
        ) noexcept;

        bool destroy_entity(
            ice::StringID_Arg name
        ) noexcept;

    protected:
        ice::EntityIndex& _entity_index;
        ice::pod::Hash<ice::Entity> _named_entities;
    };

} // namespace ice
