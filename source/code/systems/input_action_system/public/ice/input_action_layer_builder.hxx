#pragma once
#include <ice/mem_unique_ptr.hxx>
#include <ice/input_action_definitions.hxx>

namespace ice
{

    class InputActionLayerBuilder
    {
    public:
        class SourceBuilder;
        class ActionBuilder;

        virtual ~InputActionLayerBuilder() noexcept = default;

        virtual auto define_source(
            ice::String name,
            ice::InputActionSourceType type
        ) noexcept -> SourceBuilder = 0;

        virtual auto define_action(
            ice::String name,
            ice::InputActionDataType type
        ) noexcept -> ActionBuilder = 0;

        virtual auto finalize(
            ice::Allocator& alloc
        ) noexcept -> ice::UniquePtr<ice::InputActionLayer> = 0;
    };

    class InputActionLayerBuilder::SourceBuilder
    {
    public:
        struct Internal;

        SourceBuilder(Internal* internal) noexcept;
        ~SourceBuilder() noexcept = default;

        SourceBuilder(SourceBuilder&&) noexcept = delete;
        SourceBuilder(SourceBuilder const&) noexcept = delete;
        auto operator=(SourceBuilder&&) noexcept -> SourceBuilder& = delete;
        auto operator=(SourceBuilder const&) noexcept -> SourceBuilder& = delete;

        auto add_key(ice::input::KeyboardKey key) noexcept -> SourceBuilder&;
        auto add_keymod(ice::input::KeyboardMod keymod) noexcept -> SourceBuilder&;
        auto add_button(ice::input::MouseInput button) noexcept -> SourceBuilder&;
        auto add_button(ice::input::ControllerInput button) noexcept -> SourceBuilder&;
        auto add_axis(ice::input::MouseInput axisx) noexcept -> SourceBuilder&;
        auto add_axis(ice::input::ControllerInput button) noexcept -> SourceBuilder&;

    private:
        Internal* _internal;
    };

    class InputActionLayerBuilder::ActionBuilder
    {
    public:
        struct Internal;

        ActionBuilder(Internal* internal) noexcept;
        ~ActionBuilder() noexcept = default;

        ActionBuilder(ActionBuilder&&) noexcept = delete;
        ActionBuilder(ActionBuilder const&) noexcept = delete;
        auto operator=(ActionBuilder&&) noexcept -> ActionBuilder& = delete;
        auto operator=(ActionBuilder const&) noexcept -> ActionBuilder& = delete;

        auto set_behavior(ice::InputActionBehavior behavior) noexcept -> ActionBuilder&;

        auto add_condition(
            ice::String source,
            ice::InputActionCondition condition,
            ice::InputActionConditionFlags flags = InputActionConditionFlags::None,
            ice::f32 param = 0.0f,
            bool from_action = false
        ) noexcept -> ActionBuilder&;

        auto add_step(
            ice::String source,
            ice::InputActionStep step,
            ice::String target_axis = ".x"
        ) noexcept -> ActionBuilder&;

        auto add_step(
            ice::InputActionStep step
        ) noexcept -> ActionBuilder&;

        auto add_modifier(
            ice::InputActionModifier modifier,
            ice::f32 param,
            ice::String target_axis = ".x"
        ) noexcept -> ActionBuilder&;

    private:
        Internal* _internal;
    };

    auto create_input_action_layer_builder(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::InputActionLayerBuilder>;

} // namespace ice
