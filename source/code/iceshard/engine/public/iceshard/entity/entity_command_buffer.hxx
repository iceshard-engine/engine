#pragma once
#include <iceshard/entity/entity.hxx>
#include <core/allocator.hxx>
#include <core/data/queue.hxx>
#include <core/pod/array.hxx>
#include <core/cexpr/stringid.hxx>

namespace iceshard
{


    class EntityIndex;

    class EntityManager;

    class ComponentSystem;


    //! \brief A buffer of specific commands which should be run for given entities.
    class EntityCommandBuffer final
    {
    public:
        EntityCommandBuffer(core::allocator& alloc) noexcept;
        ~EntityCommandBuffer() noexcept = default;

        //! \brief Adds a component to the given entity.
        void add_component(
            entity_handle_type entity,
            ComponentSystem* comonent_system,
            core::stringid_arg_type component_name
        ) noexcept;

        //! \brief Removes a component from the given entity.
        void remove_component(
            entity_handle_type entity,
            core::stringid_arg_type component_name
        ) noexcept;

        using component_operation_signature = auto (ComponentSystem*, entity_handle_type) noexcept -> bool;

        //! \brief Runs an operation on the given component.
        void update_component(
            entity_handle_type entity,
            core::stringid_arg_type component_name,
            component_operation_signature* operation_func
        ) noexcept;

        //! \brief Destroys the given entity and all associated components.
        void destroy_entity(entity_handle_type entity) noexcept;

        //! \brief Executes all stored commands.
        void execute(EntityManager* entity_manager, EntityIndex* entity_index) noexcept;

    private:
        core::data_queue _data_queue;

        struct Command
        {
            core::stringid_type command_name;

            core::stringid_type component_name;

            entity_handle_type target_entity;

            void* command_data;
        };
        core::pod::Array<Command> _commands;
    };


} // namespace iceshard
