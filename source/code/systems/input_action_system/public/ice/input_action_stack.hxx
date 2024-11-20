#pragma once
#include <ice/input_action_types.hxx>
#include <ice/mem_unique_ptr.hxx>

namespace ice
{

    class InputActionStack
    {
    public:
        virtual ~InputActionStack() noexcept = default;

        virtual bool check_action(
            ice::String action,
            ice::vec3f& out_val,
            ice::Tns& out_timeactive
        ) const noexcept = 0;

        virtual void register_layer(
            ice::InputActionLayer const* layer
        ) noexcept = 0;

        virtual void process_inputs(
            ice::Span<ice::input::InputEvent const> events
        ) noexcept = 0;
    };

    auto create_input_action_stack(ice::Allocator& alloc) noexcept -> ice::UniquePtr<ice::InputActionStack>;

} // namespace ice
