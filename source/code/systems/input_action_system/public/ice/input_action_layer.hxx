#pragma once
#include <ice/expected.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/input_action_types.hxx>
#include <ice/container/hashmap.hxx>

namespace ice
{

    class InputActionLayer
    {
    public:
        virtual ~InputActionLayer() noexcept = default;

        virtual auto name() const noexcept -> ice::String = 0;

        virtual auto sources() const noexcept -> ice::Span<ice::InputActionSourceInfo const> = 0;
        virtual auto source_name(ice::InputActionSourceInfo const& source) const noexcept -> ice::String = 0;

        virtual auto actions() const noexcept -> ice::Span<ice::InputActionInfo const> = 0;
        virtual auto action_name(ice::InputActionInfo const& action) const noexcept -> ice::String = 0;

        virtual bool process_inputs(
            ice::Span<ice::input::InputEvent const> events,
            ice::Span<ice::InputActionSource* const> source_values
        ) const noexcept = 0;

        virtual bool update_actions(
            ice::InputActionExecutor const& executor,
            ice::Span<ice::InputActionSource* const> source_values,
            ice::HashMap<ice::InputActionRuntime>& actions
        ) const noexcept = 0;
    };

    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Data layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>;

    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Memory layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>;

    auto save_input_action_layer(
        ice::Allocator& alloc,
        ice::InputActionLayer const& action_layer
    ) noexcept -> ice::Expected<ice::Memory>;

} // namespace ice
