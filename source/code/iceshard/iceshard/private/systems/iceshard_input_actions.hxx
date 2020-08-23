#pragma once
#include <core/allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/pod/collections.hxx>
#include <core/collections.hxx>
#include <core/pointer.hxx>
#include <core/clock.hxx>

#include <iceshard/action/action_input.hxx>

namespace iceshard
{

    class Frame;

    struct InputActionTrigger
    {
        ActionTriggerEvent event;
        ActionTriggerFunc* func;
        void* user_data;
    };

    struct InputActionStage
    {
        uint32_t initial_success_trigger;
        uint32_t final_success_trigger;

        uint32_t initial_fail_trigger;
        uint32_t final_fail_trigger;

        uint32_t reset_trigger;
    };

    struct InputAction
    {
        core::stringid_type name;

        core::pod::Array<InputActionStage> stages;
        core::pod::Array<InputActionTrigger> triggers;
        //core::pod::Array<InputActionTrigger> fail_triggers;
        //core::pod::Array<InputActionTrigger> reset_triggers;
    };

    struct InputActionState
    {
        core::stringid_type action_name;
        InputAction const* action_info;

        core::Timeline action_timeline;

        uint32_t current_stage;
        uint32_t current_success_trigger;
        uint32_t current_fail_trigger;
        uint32_t current_reset_trigger;

        bool is_success;
        bool is_fail;
    };

    class InputActionsTracker final : public ActionSystem, public ActionTriggerDatabase
    {
    public:
        InputActionsTracker(core::allocator& alloc, core::Clock& clock) noexcept;
        ~InputActionsTracker() noexcept override;

        auto trigger_database() noexcept -> ActionTriggerDatabase* override
        {
            return this;
        }

        auto trigger_database() const noexcept -> ActionTriggerDatabase const* override
        {
            return this;
        }

        void create_action(
            core::stringid_arg_type name,
            ActionDefinition action_definition
        ) noexcept override;

        void define_action(InputAction action) noexcept;
        //void define_quick_action(core::stringid_type name, iceshard::input::InputID input) noexcept override;

        void remove_action(core::stringid_arg_type name) noexcept;

        void update_actions(Frame const& frame, core::pod::Array<core::stringid_type>& out_actions) noexcept;


        void add_trigger_definition(
            core::stringid_arg_type name,
            ActionTriggerDefinition definition
        ) noexcept override;

        auto get_trigger_definition(
            core::stringid_arg_type name
        ) const noexcept -> ActionTriggerDefinition override;

    private:
        core::allocator& _allocator;
        core::Clock& _clock;

        core::Map<core::stringid_hash_type, core::memory::unique_pointer<InputAction>> _defined_actions;
        core::pod::Hash<ActionTriggerDefinition> _trigger_definitions;

        core::pod::Array<InputActionState> _action_states;
    };

} // namespace iceshard
