#include <ice/input_action_stack.hxx>
#include <ice/input_action_layer.hxx>
#include <ice/input_action_definitions.hxx>
#include <ice/input_action_executor.hxx>
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

        bool check_action(ice::String action, ice::vec3f& out_val, ice::Tns& out_timeactive) const noexcept override;

        void register_layer(ice::InputActionLayer const* layer) noexcept override;
        void process_inputs(ice::Span<ice::input::InputEvent const> events) noexcept override;

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


    bool SimpleInputActionStack::check_action(ice::String action, ice::vec3f& out_val, ice::Tns& out_timeactive) const noexcept
    {
        ice::InputActionRuntime const* runtime = ice::hashmap::try_get(_actions, ice::hash(action));
        bool const action_active = runtime != nullptr && runtime->active;
        if (action_active)
        {
            out_timeactive = ice::clock::elapsed(runtime->activation_ts, ice::clock::now());
            out_val = runtime->final_value;
        }
        return action_active;
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
                ice::hashmap::set(_actions, action_hash, { .enabled = true });
            }
        }

        // 'OFFSET' that locates the start where 'indexes' for values can be found.
        ice::hashmap::set(_layers, ice::hashmap::has(_layers, name_hash), { layer, offsets_offset });
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

    auto create_input_action_stack(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::InputActionStack>
    {
        return ice::make_unique<ice::SimpleInputActionStack>(alloc, alloc);
    }

} // namespace ice
