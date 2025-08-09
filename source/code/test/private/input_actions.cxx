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
        , _allocator{ alloc, "trait :: input-actions" }
        , _layers{ _allocator }
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

    auto InputActionsTrait::activate(
        ice::WorldStateParams const& world_update
    ) noexcept -> ice::Task<>
    {
        using namespace ice::input;
        using enum ice::InputActionConditionFlags;
        _stack = ice::addressof(world_update.engine.actions());

        ice::AssetStorage& assets = world_update.assets;
        ice::Asset script = assets.bind(ice::AssetCategory_InputActionsScript, "core/example_input_actions");
        if (script.valid() == false)
        {
            co_return;
        }

        ice::Data const data = co_await script[AssetState::Raw];
        if (data.location != nullptr)
        {
            _layers = ice::parse_input_action_layer(_allocator, ice::string::from_data(data));
        }



        for (ice::UniquePtr<ice::InputActionLayer> const& layer : _layers)
        {
            _stack->register_layer(layer.get());
            _stack->push_layer(layer.get());
        }
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

    auto evname(InputActionSourceEvent ev) -> char const*
    {
        switch(ev)
        {
            using enum InputActionSourceEvent;
        case None: return "-";
        case Trigger: return "trigger";
        case ButtonPress: return "pressed";
        case ButtonRelease: return "released";
        case Axis: return "axis";
        case AxisDeadzone: return "deadzone";
        default: return "?";
        }
    }

    void InputActionsTrait::build_content() noexcept
    {
        for (ice::UniquePtr<ice::InputActionLayer> const& layer : _layers)
        {
            ImGui::Separator();
            ImGui::TextT("{}", layer->name());
            ImGui::SeparatorText("Sources");

            ice::u32 last_storage = ice::u32_max;
            for (ice::InputActionSourceInputInfo const& source_info : layer->sources())
            {
                ice::InputActionSource const& source = _stack->source_runtime(*layer, source_info);
                if (ice::exchange(last_storage, source_info.storage_offset) == source_info.storage_offset)
                {
                    continue;
                }

                if (source_info.type == InputActionSourceType::Axis2d)
                {
                    ImGui::TextT("Source: {}, value:{}x{}", layer->source_name(source_info), source.value, (&source + 1)->value);
                }
                else
                {
                    ImGui::TextT("Source: {}, value:{} ({})", layer->source_name(source_info), source.value, evname(source.event));
                }
            }
            ImGui::SeparatorText("Actions");
            for (ice::InputActionInfo const& action_info : layer->actions())
            {
                ice::InputActionRuntime const& action = _stack->action_runtime(*layer, action_info);
                ImGui::TextT("Action: {}, active:{}, value:{},{}", layer->action_name(action_info), action.active, action.value.x, action.value.y);
            }
        }
    }

} // namespace ice
