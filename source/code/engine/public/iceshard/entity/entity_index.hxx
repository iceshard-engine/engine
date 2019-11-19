#pragma once
#include <iceshard/entity/entity.hxx>
#include <core/allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/collections.hxx>

namespace iceshard
{


    class ComponentSystem;


    //! \brief This class is used to store information about components an entity uses.
    class EntityIndex final
    {
    public:
        EntityIndex(core::allocator& alloc) noexcept;
        ~EntityIndex() noexcept = default;

        void register_component(
            entity_handle_type entity,
            core::cexpr::stringid_argument_type component_name,
            ComponentSystem* component_system
        ) noexcept;

        auto find_component_system(
            entity_handle_type entity,
            core::cexpr::stringid_argument_type component_name
        ) noexcept -> ComponentSystem*;

        void remove_component(
            entity_handle_type entity,
            core::cexpr::stringid_argument_type component_name
        ) noexcept;

        void remove_entity(entity_handle_type entity) noexcept;

    private:
        core::memory::scratch_allocator _temp_allocator;

        struct PrototypeInfo
        {
            core::cexpr::stringid_type base_prototype;

            core::cexpr::stringid_type component_name;

            ComponentSystem* component_system;
        };

        core::pod::Hash<core::cexpr::stringid_type> _entity_index;
        core::pod::Hash<PrototypeInfo> _prototype_map;
    };


} // namespace iceshard
