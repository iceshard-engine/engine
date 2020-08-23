#pragma once
#include <core/pod/collections.hxx>
#include <iceshard/action/action_trigger.hxx>

namespace iceshard
{

    struct ActionTrigger
    {
        core::stringid_type trigger_name;
        void* trigger_userdata;
    };

    struct ActionStage
    {
        uint32_t initial_success_trigger;
        uint32_t initial_failure_trigger;

        uint32_t num_success_triggers;
        uint32_t num_failure_triggers;

        uint32_t reset_trigger;
    };

    //! \brief Allocates an action definition object.
    //!
    //! \detail This object will be capable of storing a predefined number of stages and triggers.
    struct ActionDefinition
    {
        core::pod::Array<ActionStage> stages;
        core::pod::Array<ActionTrigger> success_triggers;
        core::pod::Array<ActionTrigger> failure_triggers;
        core::pod::Array<ActionTrigger> reset_triggers;
    };

    //enum class ActionHandle : uint32_t
    //{
    //    Invalid = 0x0
    //};

    class ActionSystem
    {
    public:
        virtual ~ActionSystem() noexcept = default;

        virtual auto trigger_database() noexcept -> ActionTriggerDatabase* = 0;

        virtual auto trigger_database() const noexcept -> ActionTriggerDatabase const* = 0;

        virtual void create_action(
            core::stringid_arg_type name,
            ActionDefinition action_definition
        ) noexcept = 0;
    };

} // namespace iceshard
