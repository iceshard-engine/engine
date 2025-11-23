/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/input_action_definitions.hxx>
#include <ice/shard_container.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/clock_types.hxx>
#include <ice/expected.hxx>

namespace ice
{

    static constexpr ice::ErrorCode S_LayerAlreadyRegistered{ "S.2300:InputAction:The passed input action layer is already registered."};
    static constexpr ice::ErrorCode E_UnknownInputAction{ "E.2300:InputAction:The requested input action does not exist." };
    static constexpr ice::ErrorCode E_InputActionDisabled{ "E.2301:InputAction:The requested input action is disabled and cannot be accessed." };
    static constexpr ice::ErrorCode E_InputActionInactive{ "E.2302:InputAction:The requested input action is inactive and cannot be accessed." };

    //! \brief Represents an stack of currently registered input layers.
    //! \details Layers can be stacked on each other with a priority value, which starts at '0' and grows.
    //!   If the stack contains a layer, which priority is lower than a layer processed previously, that layer will not be processed.
    class InputActionStack
    {
    public:
        virtual ~InputActionStack() noexcept = default;

        virtual auto registered_layers(
            ice::Array<ice::InputActionLayer const*>& out_layers
        ) const noexcept -> ice::ucount = 0;

        virtual auto register_layer(
            ice::InputActionLayer const* layer
        ) noexcept -> ice::Result = 0;


        virtual auto active_layers(
            ice::Array<ice::InputActionLayer const*>& out_layers
        ) const noexcept -> ice::ucount = 0;

        virtual void push_layer(
            ice::InputActionLayer const* layer
        ) noexcept = 0;

        virtual void pop_layer(
            ice::InputActionLayer const* layer
        ) noexcept = 0;


        virtual auto action(
            ice::String action
        ) const noexcept -> ice::Expected<ice::InputAction const*> = 0;

        virtual bool action_check(
            ice::String action,
            ice::InputActionCheck check = InputActionCheck::Active
        ) const noexcept = 0;

        virtual auto action_time(
            ice::String action
        ) const noexcept -> ice::Tms = 0;

        virtual bool action_value(
            ice::String action,
            ice::vec2f& out_value
        ) const noexcept = 0;

        virtual bool action_value(
            ice::String action,
            ice::vec2f& out_value,
            ice::Tns& out_timeactive
        ) const noexcept = 0;


        virtual void process_inputs(
            ice::Span<ice::input::InputEvent const> events
        ) noexcept = 0;

        virtual auto publish_shards(
            ice::ShardContainer& out_shards
        ) const noexcept -> void = 0;


        virtual auto action_runtime(
            ice::InputActionLayer const& layer,
            ice::InputActionInfo const& action_info
        ) const noexcept -> ice::InputActionRuntime const& = 0;

        virtual auto source_runtime(
            ice::InputActionLayer const& layer,
            ice::InputActionSourceInputInfo const& source_info
        ) const noexcept -> ice::InputActionSource const& = 0;
    };

    auto create_input_action_stack(
        ice::Allocator& alloc,
        ice::String actionid_prefix
    ) noexcept -> ice::UniquePtr<ice::InputActionStack>;

} // namespace ice
