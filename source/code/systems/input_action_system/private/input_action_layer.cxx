/// Copyright 2025 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


#include "input_action_internal_types.hxx"
#include "input_action_script_parser.hxx"

#include <ice/input/input_keyboard.hxx>
#include <ice/input/input_mouse.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/input_action_layer_builder.hxx>
#include <ice/input_action_definitions.hxx>
#include <ice/input_action_executor.hxx>
#include <ice/array.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/profiler.hxx>
#include <ice/clock.hxx>
#include <ice/sort.hxx>
#include <ice/log.hxx>

namespace ice
{

    struct InputActionLayerInfo
    {
        ice::String name;
        ice::Span<ice::InputActionSourceInputInfo const> sources;
        ice::Span<ice::InputActionInfo const> actions;
        ice::Span<ice::InputActionConditionData const> conditions;
        ice::Span<ice::InputActionStepData const> steps;
        ice::Span<ice::InputActionModifierData const> modifiers;
        ice::Span<ice::InputActionConstantInfo const> constants;
        ice::Span<ice::f32 const> constant_values;
        ice::String strings;
    };

    template<typename T>
    auto load_field_from_data(
        ice::Span<T const>& out_span,
        ice::Data data,
        ice::usize offset,
        ice::ncount count
    ) noexcept -> ice::usize
    {
        ice::data::read_span(ice::ptr_add(data, offset), count, out_span);
        return out_span.size();
    }

    auto load_from_data(ice::Data data) noexcept -> ice::Expected<ice::InputActionLayerInfo>
    {
        if (data.location == nullptr)
        {
            return E_NullPointerData;
        }

        ice::InputActionLayerInfoHeader const& header = *reinterpret_cast<ice::InputActionLayerInfoHeader const*>(data.location);
        ice::usize offset = ice::size_of<ice::InputActionLayerInfoHeader>;

        ice::InputActionLayerInfo result{};
        offset += load_field_from_data(result.sources, data, offset, header.count_sources);
        offset += load_field_from_data(result.actions, data, offset, header.count_actions);
        offset += load_field_from_data(result.conditions, data, offset, header.count_conditions);
        offset += load_field_from_data(result.steps, data, offset, header.count_steps);
        offset += load_field_from_data(result.modifiers, data, offset, header.count_modifiers);
        offset += load_field_from_data(result.constant_values, data, offset, header.count_constants);
        offset += load_field_from_data(result.constants, data, offset, header.count_constants);

        ICE_ASSERT_CORE(offset == ice::usize{ header.offset_strings });
        result.strings = ice::string_from_data<char>(
            data,
            header.offset_strings,
            data.size.value - header.offset_strings
        );
        result.name = result.strings.substr(0, header.size_name);
        return result;
    }

    class StandardInputActionLayer final : public ice::InputActionLayer
    {
    public:
        StandardInputActionLayer(
            ice::Allocator& alloc,
            ice::Memory memory,
            ice::InputActionLayerInfo const& info
        ) noexcept
            : _allocator{ alloc }
            , _rawdata{ memory }
            , _name{ info.name }
            , _sources{ info.sources }
            , _actions{ info.actions }
            , _conditions{ info.conditions }
            , _steps{ info.steps }
            , _modifiers{ info.modifiers }
            , _constants{ info.constants }
            , _constant_values{ info.constant_values }
            , _strings{ info.strings }
        { }

        ~StandardInputActionLayer() noexcept override
        {
            _allocator.deallocate(_rawdata);
        }

        auto name() const noexcept -> ice::String override
        {
            return _name;
        }

        auto sources() const noexcept -> ice::Span<ice::InputActionSourceInputInfo const> override
        {
            return _sources;
        }

        auto source_name(ice::InputActionSourceInputInfo const& source) const noexcept -> ice::String override
        {
            return _strings.substr(source.name);
        }

        auto actions() const noexcept -> ice::Span<ice::InputActionInfo const> override
        {
            return _actions;
        }

        auto action_name(ice::InputActionInfo const& action) const noexcept -> ice::String override
        {
            return _strings.substr(action.name);
        }

        auto load_constants(ice::Span<ice::f32> constants_span) const noexcept -> ice::u32 override
        {
            ICE_ASSERT_CORE(constants_span.size() >= Constant_CountInputActionConstants);
            for (ice::InputActionConstantInfo const constant : _constants)
            {
                ice::u32 const idx = ice::u32(constant.identifier);
                constants_span[idx] = _constant_values[constant.offset];
            }
            return _constants.size().u32();
        }

        auto process_inputs(
            ice::Span<ice::input::InputEvent> input_events,
            ice::Span<ice::InputActionSource* const> source_values
        ) const noexcept -> ice::u32 override
        {
            IPT_ZONE_SCOPED;

            // helpers
            static auto comp_event_id = [](ice::input::InputEvent ev, ice::input::InputID id) noexcept -> bool
                {
                    return ev.identifier == id;
                };

            ice::u32 count_processed = 0;
            ice::u32 const count_events = input_events.size().u32();

            // Reset the temporary events.
            for (ice::InputActionSourceInputInfo const& src : _sources)
            {
                ice::InputActionSource* const values = source_values[src.storage_offset];
                ice::u32 const count_values = 1 + ice::u32(src.type == InputActionSourceType::Axis2d);

                for (ice::u32 idx = 0; idx < count_values; ++idx)
                {
                    values[idx].temp_event = InputActionSourceEvent::None;
                }
            }

            for (ice::InputActionSourceInputInfo const& src : _sources)
            {
                ice::InputActionSource* const values = source_values[src.storage_offset];
                ice::u32 const count_values = 1 + ice::u32(src.type == InputActionSourceType::Axis2d);

                ice::u32 event_index = 0;
                if (ice::search(input_events, src.input, comp_event_id, event_index) == false)
                {
                    // Reset any event that was a key-release event previously
                    for (ice::u32 idx = 0; idx < count_values; ++idx)
                    {
                        if (values[idx].event == InputActionSourceEvent::KeyRelease)
                        {
                            values[idx].event = InputActionSourceEvent::None;
                        }
                    }
                    continue;
                }

                // We consume the event by placing the current last event at it's index.
                count_processed += 1;
                ice::input::InputEvent const ev = ice::exchange(
                    input_events[event_index],
                    input_events[count_events - count_processed]
                );

                // The actual value that is being updated
                ICE_ASSERT_CORE(ev.axis_idx < count_values);
                InputActionSource& value = values[ev.axis_idx];

                if (ev.value_type == ice::input::InputValueType::Trigger)
                {
                    value = { ev.value.axis.value_f32, InputActionSourceEvent::Trigger };
                }
                else if (ev.value_type == ice::input::InputValueType::AxisInt)
                {
                    value = { ice::f32(ev.value.axis.value_i32), InputActionSourceEvent::Axis };
                }
                else if (ev.value_type == ice::input::InputValueType::AxisFloat)
                {
                    // Check for deadzone values
                    if (src.param < ev.value.axis.value_f32)
                    {
                        value = { ev.value.axis.value_f32, InputActionSourceEvent::Axis };
                    }
                    else
                    {
                        value = { ev.value.axis.value_f32, InputActionSourceEvent::AxisDeadzone };
                    }
                }
                else if (value.temp_event != InputActionSourceEvent::KeyPress)
                {
                    value = {
                        ev.value.button.state.released
                            ? 0.0f : 1.0f,
                        ev.value.button.state.released
                            ? InputActionSourceEvent::KeyRelease
                            : InputActionSourceEvent::KeyPress,
                    };
                }
            }

            // Select the final events.
            for (ice::InputActionSourceInputInfo const& src : _sources)
            {
                ice::InputActionSource* const values = source_values[src.storage_offset];
                ice::u32 const count_values = 1 + ice::u32(src.type == InputActionSourceType::Axis2d);

                for (ice::u32 idx = 0; idx < count_values; ++idx)
                {
                    values[idx].event = values[idx].temp_event;
                }
            }

            // Clear the end of the events list
            for (ice::u32 idx = 1; idx <= count_processed; ++idx)
            {
                input_events[count_events - idx] = ice::input::InputEvent{};
            }
            return count_processed;
        }

        bool update_actions(
            ice::InputActionExecutor const& executor,
            ice::Span<ice::InputActionSource* const> source_values,
            ice::HashMap<ice::InputActionRuntime>& actions
        ) const noexcept override
        {
            IPT_ZONE_SCOPED;
            using enum ice::InputActionConditionFlags;

            for (ice::InputActionInfo const& action : _actions)
            {
                ice::String const action_name = _strings.substr(action.name);

                ice::InputActionRuntime* const runtime = ice::hashmap::try_get(actions, ice::hash(action_name));
                // TODO: Check if we need this
                //if (action.behavior != InputActionBehavior::Accumulated)
                {
                    runtime->raw_value = {};
                }

                bool series_success = false;
                ice::Span const conditions = _conditions.subspan(action.conditions);
                for (ice::InputActionConditionData const& cond : conditions)
                {
                    bool cond_result = false;
                    if (cond.id >= ice::InputActionCondition::ActionEnabled)
                    {
                        ice::InputActionRuntime const* checked_action = runtime; // Seft (by-default)
                        if (cond.source.source_index != InputActionIndex::SelfIndex)
                        {
                            ice::InputActionInfo const checked_action_info = _actions[cond.source.source_index];
                            ice::String const checked_action_name = _strings.substr(checked_action_info.name);
                            checked_action = ice::hashmap::try_get(actions, ice::hash(checked_action_name));
                        }
                        ICE_ASSERT_CORE(checked_action != nullptr);
                        cond_result = executor.execute_condition(
                            cond.id,
                            *checked_action,
                            cond.param
                        );
                    }
                    else
                    {
                        ICE_ASSERT_CORE(source_values[cond.source.source_index] != nullptr);
                        ice::InputActionSource& value = source_values[cond.source.source_index][0];

                        if (value.event != InputActionSourceEvent::None)
                        {
                            cond_result = executor.execute_condition(cond.id, value, cond.param);
                        }
                    }

                    // Can't check for 'SeriesOr' since it's just a zero value
                    if (ice::has_all(cond.flags, InputActionConditionFlags::SeriesAnd))
                    {
                        series_success &= cond_result;
                    }
                    else
                    {
                        series_success |= cond_result;
                    }

                    bool const check_success = ice::has_all(cond.flags, SeriesCheck)
                        ? series_success
                        : cond_result;

                    if (ice::has_all(cond.flags, RunSteps) && check_success)
                    {
                        ice::Span const steps = _steps.subspan(cond.steps);
                        for (ice::InputActionStepData const& step : steps)
                        {
                            if (step.id < InputActionStep::Set)
                            {
                                executor.execute_step(step.id, *runtime);
                            }
                            else
                            {
                                ICE_ASSERT_CORE(source_values[step.source.source_index] != nullptr);
                                ice::InputActionSource& value = source_values[step.source.source_index][step.source.source_axis];

                                executor.execute_step(step.id, value, runtime->raw_value.v[0][step.dst_axis]);
                            }
                        }
                    }

                    // If this is not 'series finish', continue
                    if (ice::has_none(cond.flags, SeriesFinish))
                    {
                        continue;
                    }

                    // // If the condition should activate/deactivate implicitly
                    // // TODO: Move before the final check to allow activating on regular checks?
                    // if (ice::has_all(cond.flags, Activate) && runtime->enabled)
                    // {
                    //     // This small operation allows use to implement 'Once' logic.
                    //     // Initial state == 1, unless deactivated from now on we will always be above 1
                    //     //   since we purposefully loosing bits due to overflow but always set the MSB again.
                    //     runtime->state = runtime->state * 2 + 1;
                    //     runtime->active = check_success;
                    // }
                    // else if (ice::has_all(cond.flags, Deactivate))
                    // {
                    //     runtime->state = 0;
                    //     runtime->active = false;
                    // }

                    // If the series is not successful, just continue
                    if (series_success == false || runtime->enabled == false)
                    {
                        // If the action finaly fails, we reset the state so we can again start counting from 0
                        runtime->state = 0;
                        runtime->active = false;
                        continue;
                    }

                    series_success = false;

                    // continue, if it's not a final condition
                    if (ice::has_none(cond.flags, Final))
                    {
                        continue;
                    }

                    // Stop checking futher conditions, we are in the final  condition
                    break;
                }
            }

            for (ice::InputActionInfo const& action : _actions)
            {
                ice::String const action_name = _strings.substr(action.name);
                ice::InputActionRuntime* const runtime = ice::hashmap::try_get(actions, ice::hash(action_name));

                // Handles 'Toggle'. We only activate of the first press, which is `state == 1`.
                if (action.behavior == InputActionBehavior::Toggled)
                {
                    runtime->active |= runtime->toggle_enabled;
                    //if (runtime->state == 1)
                    //{
                    //    runtime->toggle_enabled = !runtime->toggle_enabled;
                    //}
                    //else
                    //{
                    //}
                }
                // Handles 'Once'. Once state is bigger than `1` we always deactivate the action.
                else if (action.behavior == InputActionBehavior::ActiveOnce && runtime->state > 1)
                {
                    runtime->active = false;
                }

                if (runtime->active == false)
                {
                    runtime->was_active = false;
                    continue;
                }

                if (runtime->was_active == false)
                {
                    runtime->was_active = true;
                    runtime->timestamp = ice::clock::now();
                }

                // Update the final value and run modifiers over it.
                runtime->value = { runtime->raw_value.x, runtime->raw_value.y };

                ice::Span const mods = _modifiers.subspan(action.mods);
                for (ice::InputActionModifierData const& mod : mods)
                {
                    executor.execute_modifier(mod.id, runtime->value.v[0][mod.axis], mod.param);
                }
            }
            return false;
        }

    private:
        ice::Allocator& _allocator;
        ice::Memory _rawdata;

        ice::String _name;
        ice::Span<ice::InputActionSourceInputInfo const> _sources;
        ice::Span<ice::InputActionInfo const> _actions;
        ice::Span<ice::InputActionConditionData const> _conditions;
        ice::Span<ice::InputActionStepData const> _steps;
        ice::Span<ice::InputActionModifierData const> _modifiers;
        ice::Span<ice::InputActionConstantInfo const> _constants;
        ice::Span<ice::f32 const> _constant_values;
        ice::String _strings;
    };

    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Data layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>
    {
        ice::Memory const data_copy = alloc.allocate({ layer_data.size, layer_data.alignment });
        ice::memcpy(data_copy, layer_data);

        ice::Expected<ice::InputActionLayerInfo> params = ice::load_from_data(ice::data_view(data_copy));
        if (params.succeeded() == false)
        {
            return {};
        }

        return ice::make_unique<ice::StandardInputActionLayer>(alloc, alloc, data_copy, params.value());
    }

    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Memory layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>
    {
        ice::Expected<ice::InputActionLayerInfo> params = ice::load_from_data(ice::data_view(layer_data));
        if (params.succeeded() == false)
        {
            return {};
        }

        return ice::make_unique<ice::StandardInputActionLayer>(alloc, alloc, layer_data, params.value());
    }

    auto parse_input_action_layer(
        ice::Allocator& alloc,
        ice::String definition
    ) noexcept -> ice::Array<UniquePtr<ice::InputActionLayer>>
    {
        struct MultiLayerScriptParser : InputActionScriptParser
        {
            ice::Array<UniquePtr<ice::InputActionLayer>> results;

            MultiLayerScriptParser(ice::Allocator& alloc) noexcept
                : InputActionScriptParser{ alloc }
                , results{ alloc }
            {
            }

            void on_layer_parsed(ice::UniquePtr<ice::InputActionLayer> layer) noexcept override
            {
                results.push_back(ice::move(layer));
            }
        } parser{ alloc };

        bool const parse_success = ice::asl::parse_action_input_definition(alloc, definition, parser);
        ICE_LOG_IF(parse_success == false, LogSeverity::Warning, LogTag::Engine, "Failed to parse input action layer script!");
        return ice::move(parser.results);
    }

    auto save_input_action_layer(
        ice::Allocator& alloc,
        ice::InputActionLayer const& action_layer
    ) noexcept -> ice::Expected<ice::Memory>
    {
        return {};
    }

} // namespace ice
