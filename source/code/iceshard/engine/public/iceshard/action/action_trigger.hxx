#pragma once
#include <core/base.hxx>
#include <core/clock.hxx>
#include <core/cexpr/stringid.hxx>
#include <resource/uri.hxx>

namespace iceshard
{

    enum class ActionTriggerEvent
    {
        Invalid = 0x0,
        InputEvent,
        ActionEvent,
        MessageEvent,
        FrameEvent,
    };

    using ActionTriggerFunc = bool(void* userdata, float elapsed_time, void const* event_data) noexcept;

    struct ActionTriggerDefinition
    {
        ActionTriggerEvent event;
        ActionTriggerFunc* func;
    };

    class ActionTriggerDatabase
    {
    public:
        virtual ~ActionTriggerDatabase() noexcept = default;

        virtual void add_trigger_definition(
            core::stringid_arg_type name,
            ActionTriggerDefinition definition
        ) noexcept = 0;

        virtual auto get_trigger_definition(
            core::stringid_arg_type name
        ) const noexcept -> ActionTriggerDefinition = 0;
    };

    void register_common_triggers(ActionTriggerDatabase& database) noexcept;

} // namespace iceshard