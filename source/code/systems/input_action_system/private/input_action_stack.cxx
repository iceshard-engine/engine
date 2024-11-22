#include <ice/input_action_stack.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/input_action_definitions.hxx>
#include <ice/input_action_executor.hxx>
#include <ice/string/static_string.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/container/array.hxx>
#include <ice/profiler.hxx>
#include <ice/clock.hxx>

namespace ice
{

    class SimpleInputActionStack final : public InputActionStack
    {
    public:
        SimpleInputActionStack(ice::Allocator& alloc) noexcept
            : _allocator{ alloc }
            , _layers{ _allocator }
            , _sources{ _allocator }
            , _actions{ _allocator }
            , _sources_values{ _allocator }
            , _sources_indices{ _allocator }
        {
        }

        auto get_layers(
            ice::Array<ice::InputActionLayer const*> out_layers
        ) const noexcept -> ice::ucount override;

        void register_layer(
            ice::InputActionLayer const* layer
        ) noexcept override;

        auto action(
            ice::String action
        ) const noexcept -> ice::Expected<ice::InputAction const*> override;

        bool action_check(
            ice::String action,
            ice::InputActionCheck check = InputActionCheck::Active
        ) const noexcept override;

        bool action_value(
            ice::String action,
            ice::vec2f& out_value
        ) const noexcept override;

        bool action_value(
            ice::String action,
            ice::vec2f& out_value,
            ice::Tns& out_timeactive
        ) const noexcept override;

        void process_inputs(ice::Span<ice::input::InputEvent const> events) noexcept override;
        auto publish_shards(ice::ShardContainer& out_shards) const noexcept -> void override;


        auto action_runtime(
            ice::InputActionLayer const& layer,
            ice::InputActionInfo const& action_info
        ) const noexcept -> ice::InputActionRuntime const& override;

        auto source_runtime(
            ice::InputActionLayer const& layer,
            ice::InputActionSourceInfo const& source_info
        ) const noexcept -> ice::InputActionSource const& override;

    private:
        struct StackLayer
        {
            ice::InputActionLayer const* layer;
            ice::u32 offsets_offset;
        };

        struct StackSource
        {
            ice::String name;
            ice::u32 values_index;
        };

    private:
        ice::Allocator& _allocator;
        ice::HashMap<StackLayer> _layers;
        ice::HashMap<StackSource> _sources;
        ice::HashMap<InputActionRuntime> _actions;

        ice::Array<ice::InputActionSource> _sources_values;
        ice::Array<ice::u32> _sources_indices;
    };

    auto SimpleInputActionStack::get_layers(
        ice::Array<ice::InputActionLayer const*> out_layers
    ) const noexcept -> ice::ucount
    {
        for (StackLayer const& layer : _layers)
        {
            ice::array::push_back(out_layers, layer.layer);
        }
        return ice::hashmap::count(_layers);
    }

    void SimpleInputActionStack::register_layer(ice::InputActionLayer const* layer) noexcept
    {
        IPT_ZONE_SCOPED;

        ice::u64 const name_hash = ice::hash(layer->name());
        if (layer == nullptr || ice::hashmap::has(_layers, name_hash))
        {
            return;
        }

        // Resize the array for the necessary amount of pointers
        ice::u32 const count_sources = ice::count(layer->sources());
        ice::u32 const offsets_offset = ice::count(_sources_indices);
        ice::array::reserve(_sources_indices, offsets_offset + count_sources);

        // Go through each resource and assing the pointers.
        for (ice::InputActionSourceInfo const& source : layer->sources())
        {
            ice::String const source_name = layer->source_name(source);

            ice::u32 values_index = 0;
            ice::u64 const source_hash = ice::hash(source_name);
            if (ice::hashmap::has(_sources, source_hash))
            {
                values_index = ice::hashmap::try_get(_sources, source_hash)->values_index;
            }
            else
            {
                values_index = ice::array::count(_sources_values);

                ice::array::push_back(_sources_values, InputActionSource{});
                if (source.type == InputActionSourceType::Axis2d)
                {
                    // Add another one
                    ice::array::push_back(_sources_values, InputActionSource{});
                }
                ICE_ASSERT_CORE(source.storage < ice::count(_sources_values));

                // Save the pointer where the values are stored
                ice::hashmap::set(_sources, source_hash, { source_name, values_index });
            }

            // Stores the index for the source
            ice::array::push_back(_sources_indices, values_index);
        }

        // Go through each resource and assing the pointers.
        for (ice::InputActionInfo const& action : layer->actions())
        {
            ice::String const action_name = layer->action_name(action);
            ice::u64 const action_hash = ice::hash(action_name);
            if (ice::hashmap::has(_actions, action_hash) == false)
            {
                // Save the pointer where the values are stored
                ice::hashmap::set(_actions, action_hash, { .name = action_name, .enabled = true });
            }
        }

        // 'OFFSET' that locates the start where 'indexes' for values can be found.
        ice::hashmap::set(_layers, ice::hashmap::has(_layers, name_hash), { layer, offsets_offset });
    }

    auto SimpleInputActionStack::action(ice::String action_name) const noexcept -> ice::Expected<ice::InputAction const*>
    {
        ice::InputActionRuntime const* action = ice::hashmap::try_get(_actions, ice::hash(action_name));
        if (action != nullptr)
        {
            if (action->enabled == false)
            {
                return E_InputActionDisabled;
            }
            else if (action->active == false)
            {
                return E_InputActionInactive;
            }
            else
            {
                return action;
            }
        }
        return E_UnknownInputAction;
    }

    bool SimpleInputActionStack::action_check(
        ice::String action_name,
        ice::InputActionCheck check
    ) const noexcept
    {
        // We check for validity not for success. NOTE: 'operator bool()' checks for success.
        if (ice::Expected const action = this->action(action_name); action.valid())
        {
            switch (check)
            {
                using enum InputActionCheck;
            case Exists: return true;
            case Enabled: return action != E_InputActionDisabled;
            case Disabled: return action == E_InputActionDisabled;
            case Active: return action.succeeded();
            case Inactive: return action == E_InputActionInactive;
            default: ICE_ASSERT_CORE(false); break;
            }
        }
        return false;
    }

    bool SimpleInputActionStack::action_value(
        ice::String action_name,
        ice::vec2f& out_value
    ) const noexcept
    {
        // Check for success
        if (ice::Expected const action = this->action(action_name); action)
        {
            out_value = action.value()->value;
            return true;
        }
        return false;
    }

    bool SimpleInputActionStack::action_value(
        ice::String action_name,
        ice::vec2f& out_value,
        ice::Tns& out_timeactive
    ) const noexcept
    {
        // Check for success
        if (ice::Expected const action = this->action(action_name); action)
        {
            out_value = action.value()->value;
            out_timeactive = ice::clock::elapsed(action.value()->timestamp, ice::clock::now());
            return true;
        }
        return false;
    }

    void SimpleInputActionStack::process_inputs(
        ice::Span<ice::input::InputEvent const> events
    ) noexcept
    {
        IPT_ZONE_SCOPED;

        ice::Array<ice::input::InputEvent> events_copy{ _allocator, events };
        ice::Array<ice::InputActionSource*> source_values{ _allocator };

        // Reset all source values changed flag.
        for (ice::InputActionSource& source : _sources_values)
        {
            source.changed = false;
        }

        // TODO: Implement priority and order logic
        for (StackLayer const& layer : ice::hashmap::values(_layers))
        {
            for (ice::u32 offset : ice::array::slice(_sources_indices, layer.offsets_offset, ice::count(layer.layer->sources())))
            {
                ice::array::push_back(source_values, ice::addressof(_sources_values[offset]));
            }

            layer.layer->process_inputs(events_copy, source_values);
        }

        ice::InputActionExecutor ex{};
        for (StackLayer const& layer : ice::hashmap::values(_layers))
        {
            for (ice::u32 offset : ice::array::slice(_sources_indices, layer.offsets_offset, ice::count(layer.layer->sources())))
            {
                ice::array::push_back(source_values, ice::addressof(_sources_values[offset]));
            }

            layer.layer->update_actions(ex, source_values, _actions);
        }
    }

    auto SimpleInputActionStack::publish_shards(
        ice::ShardContainer& out_shards
    ) const noexcept -> void
    {
        ice::StaticString<64> temp{ "ice/input-action/" };
        ice::ucount const temp_prefix = ice::size(temp);

        for (ice::InputActionRuntime const& action : _actions)
        {
            if (action.active)
            {
                ice::string::resize(temp, temp_prefix);
                ice::string::push_back(temp, action.name);
                ice::shards::push_back(out_shards, ice::shardid(ice::String{temp}) | (ice::InputAction const*)ice::addressof(action));
            }
        }
    }

    auto SimpleInputActionStack::action_runtime(
        ice::InputActionLayer const& layer,
        ice::InputActionInfo const& action_info
    ) const noexcept -> ice::InputActionRuntime const&
    {
        static ice::InputActionRuntime invalid{.name="<invalid-action-info>"};
        ice::String const action_name = layer.action_name(action_info);
        return ice::hashmap::get(_actions, ice::hash(action_name), invalid);
    }

    auto SimpleInputActionStack::source_runtime(
        ice::InputActionLayer const& layer,
        ice::InputActionSourceInfo const& source_info
    ) const noexcept -> ice::InputActionSource const&
    {
        static ice::InputActionSource invalid{};
        ice::String const source_name = layer.source_name(source_info);

        ice::u32 const values_idx = ice::hashmap::get(
            _sources,
            ice::hash(source_name),
            {.values_index=ice::u32_max}
        ).values_index;

        if (values_idx == ice::u32_max)
        {
            return invalid;
        }
        return _sources_values[values_idx];
    }

    auto create_input_action_stack(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::InputActionStack>
    {
        return ice::make_unique<ice::SimpleInputActionStack>(alloc, alloc);
    }

} // namespace ice
