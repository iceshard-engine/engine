#pragma once
#include <core/cexpr/stringid.hxx>
#include <core/pod/collections.hxx>

namespace iceshard
{

    struct ActionStage
    {
        uint32_t initial_success_trigger;
        uint32_t initial_failure_trigger;

        uint32_t num_success_triggers;
        uint32_t num_failure_triggers;

        uint32_t reset_trigger;
    };

    struct ActionTrigger
    {
        core::stringid_type trigger_name;
        void* trigger_userdata;
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

} // namespace iceshard
