#include "input_actions.hxx"

#include <ice/assert.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/engine.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_runner.hxx>
#include <ice/input_action_info.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/input_action_layer_builder.hxx>
#include <ice/input_action_stack.hxx>
#include <ice/input/input_controller.hxx>
#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/world/world_updater.hxx>
#include <ice/clock.hxx>

namespace ice
{

    // static constexpr ice::ShardID ShardID_ClickAction = "ice/input-action/A:click`ice::InputAction const*"_shardid;

    InputActionsTrait::InputActionsTrait(ice::TraitContext& context, ice::Allocator& alloc) noexcept
        : ice::Trait{ context }
        , ice::TraitDevUI{ {.category="Engine/Traits", .name = trait_name()} }
        , _allocator{ alloc }
        , _layer{ }
    {
        context.bind<&InputActionsTrait::on_update>();
        context.bind<&InputActionsTrait::on_click>("Click`ice::InputAction const*"_shardid);
    }

    auto InputActionsTrait::on_click(ice::InputAction const& action) noexcept -> ice::Task<>
    {
        ice::Tns const timeactive = ice::clock::elapsed(action.timestamp, ice::clock::now());
        ICE_LOG(LogSeverity::Info, LogTag::Engine, "{:.2s} Click | {}x{}", timeactive, action.value.x, action.value.y);

        co_return;
    }

    auto InputActionsTrait::activate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<>
    {
        using namespace ice::input;
        using enum ice::InputActionConditionFlags;

        _stack = ice::addressof(world_update.engine.actions());

        ice::UniquePtr<ice::InputActionLayerBuilder> layerBuilder = ice::create_input_action_layer_builder(_allocator);

        layerBuilder->define_source("S:jump", InputActionSourceType::Key)
            .add_key(KeyboardKey::Space);

        layerBuilder->define_source("S:left", InputActionSourceType::Key)
            .add_key(KeyboardKey::KeyA);
        layerBuilder->define_source("S:right", InputActionSourceType::Key)
            .add_key(KeyboardKey::KeyD);
        layerBuilder->define_source("S:up", InputActionSourceType::Key)
            .add_key(KeyboardKey::KeyW);
        layerBuilder->define_source("S:down", InputActionSourceType::Key)
            .add_key(KeyboardKey::KeyS);

        layerBuilder->define_source("S:pos", InputActionSourceType::Axis2d)
            .add_axis(input::MouseInput::PositionX);
        layerBuilder->define_source("S:click", InputActionSourceType::Button)
            .add_button(input::MouseInput::ButtonLeft);

        layerBuilder->define_action("A:click", InputActionDataType::Float2)
            // .set_behavior(InputActionBehavior::ActiveOnce)
            .add_condition("S:click", InputActionCondition::Released, Final | RunSteps)
                .add_step(InputActionStep::Deactivate)
            .add_condition("S:click", InputActionCondition::Pressed, Final | RunSteps)
                .add_step(InputActionStep::Activate)
                .add_step("S:pos.x", InputActionStep::Set, ".x")
                .add_step("S:pos.y", InputActionStep::Set, ".y");

        layerBuilder->define_action("A:jump", InputActionDataType::Bool)
            .set_behavior(InputActionBehavior::Accumulated)
            .add_condition("S:jump", InputActionCondition::Released, Final | RunSteps)
                .add_step("S:jump", InputActionStep::Set)
                .add_step(InputActionStep::Deactivate)
            .add_condition("S:jump", InputActionCondition::Pressed, Final | RunSteps)
                .add_step("S:jump", InputActionStep::Add)
                .add_step(InputActionStep::Activate)
            .add_modifier(InputActionModifier::Div, 15.f)
            .add_modifier(InputActionModifier::Max, 2.0);

        layerBuilder->define_action("A:move", InputActionDataType::Float2)
            .add_condition("S:left", InputActionCondition::Pressed, RunSteps)
                .add_step("S:left", InputActionStep::Sub, ".x")
            .add_condition("S:right", InputActionCondition::Pressed, RunSteps)
                .add_step("S:right", InputActionStep::Add, ".x")
            .add_condition("S:down", InputActionCondition::Pressed, RunSteps)
                .add_step("S:down", InputActionStep::Sub, ".y")
            .add_condition("S:up", InputActionCondition::Pressed, Final | RunSteps | SeriesCheck)
                .add_step("S:up", InputActionStep::Add, ".y")
                .add_step(InputActionStep::Activate);


        _layer = layerBuilder->finalize(_allocator);
        _layer = ice::parse_input_action_layer(_allocator, R"__(
            layer Test:
                source button Jump: kb.Space

                source button Left: kb.A
                source button Right: kb.D
                source button Up: kb.W
                source button Down: kb.S

                source axis2d Pos: mouse.pos
                source button Click: mouse.lbutton

                // <symbol>[(.pressed)|.released|.active|.inactive|[.[x|y|z] [<|>|<=|>=|==|!=] <param:number>]]

                action Jump: float1
                    when Jump.pressed
                        .time
                        .activate
                    mod .x max 1.0

                action Move: float2
                    when Left.pressed
                        .x - Left.x
                      or Right.pressed
                        .x + Right.x
                      or Up.pressed
                        .y + Up.x
                      or Down.pressed, series
                        .y - Down.x
                        .activate

                    when .true // only executed if the previous condition series are not valid
                        .reset

                action Click: object, once
                    when Click.pressed
                        .x = Pos.x
                        .y = Pos.y
                        .activate
        )__");

        world_update.engine.actions().register_layer(_layer.get());
        co_return;
    }

    auto InputActionsTrait::on_update(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        co_return;
    }

    auto InputActionsTrait::deactivate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<>
    {
        co_return;
    }

    void InputActionsTrait::build_content() noexcept
    {
        ImGui::SeparatorText("Sources");
        for (ice::InputActionSourceInputInfo const& source_info : _layer->sources())
        {
            ice::InputActionSource const& source = _stack->source_runtime(*_layer, source_info);
            if (source_info.type == InputActionSourceType::Axis2d)
            {
                ImGui::TextT("Source: {}, value:{}x{}", _layer->source_name(source_info), source.value, (&source + 1)->value);
            }
            else
            {
                ImGui::TextT("Source: {}, value:{}", _layer->source_name(source_info), source.value);
            }
        }
        ImGui::SeparatorText("Actions");
        for (ice::InputActionInfo const& action_info : _layer->actions())
        {
            ice::InputActionRuntime const& action = _stack->action_runtime(*_layer, action_info);
            ImGui::TextT("Action: {}, active:{}, value:{},{}", _layer->action_name(action_info), action.active, action.value.x, action.value.y);
        }
    }

} // namespace ice
