#include <iceshard/entity/entity_command_buffer.hxx>
#include <iceshard/entity/entity_manager.hxx>
#include <iceshard/entity/entity_index.hxx>
#include <iceshard/component/component_system.hxx>

namespace iceshard
{
    namespace detail
    {

        static constexpr auto add_component_command_id = core::cexpr::stringid_cexpr("component.add").hash_value;

        static constexpr auto remove_component_command_id = core::cexpr::stringid_cexpr("component.remove").hash_value;

        static constexpr auto update_component_command_id = core::cexpr::stringid_cexpr("component.update").hash_value;

    } // namespace detail

    EntityCommandBuffer::EntityCommandBuffer(core::allocator& alloc) noexcept
        : _data_queue{ alloc }
        , _commands{ alloc }
    {
        core::pod::array::reserve(_commands, 20);
    }

    void EntityCommandBuffer::add_component(entity_handle_type entity, ComponentSystem* comonent_system, core::cexpr::stringid_argument_type component_name) noexcept
    {
        core::pod::array::push_back(_commands, { { detail::add_component_command_id }, component_name, entity, comonent_system });
    }

    void EntityCommandBuffer::remove_component(entity_handle_type entity, core::cexpr::stringid_argument_type component_name) noexcept
    {
        core::pod::array::push_back(_commands, { { detail::remove_component_command_id }, component_name, entity, nullptr });
    }

    void EntityCommandBuffer::update_component(
        entity_handle_type entity,
        core::cexpr::stringid_argument_type component_name,
        component_operation_signature* operation_func
    ) noexcept
    {
        core::pod::array::push_back(_commands, { { detail::update_component_command_id }, component_name, entity, operation_func });
    }

    void EntityCommandBuffer::execute(EntityManager* entity_manager, EntityIndex* entity_index) noexcept
    {
        for (const auto& command : _commands)
        {
            [[unlikely]]
            if (entity_manager->is_alive(command.target_entity) == false)
            {
                continue;
            }

            switch (static_cast<uint64_t>(command.command_name.hash_value))
            {
            case static_cast<uint64_t>(detail::add_component_command_id):
            {
                auto* system = reinterpret_cast<ComponentSystem*>(command.command_data);
                system->create(command.target_entity, command.component_name);
                entity_index->register_component(command.target_entity, command.component_name, system);
                break;
            }
            case static_cast<uint64_t>(detail::remove_component_command_id) :
            {
                if (auto* system = entity_index->find_component_system(command.target_entity, command.component_name); system != nullptr)
                {
                    system->remove(command.target_entity, command.component_name);
                    entity_index->remove_component(command.target_entity, command.component_name);
                }
                break;
            }
            case static_cast<uint64_t>(detail::update_component_command_id) :
            {
                if (auto* system = entity_index->find_component_system(command.target_entity, command.component_name); system != nullptr)
                {
                    reinterpret_cast<component_operation_signature*>(command.command_data)(system, command.target_entity);
                }
                break;
            }
            }
        }

        core::pod::array::clear(_commands);
    }

} // namespace iceshard
