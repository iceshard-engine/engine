#pragma once
#include <ice/expected.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/input_action_types.hxx>
#include <ice/container/hashmap.hxx>

namespace ice
{

    //! \brief An action layer represents a set of actions, grouped by some logic the developer.
    //! \details Each layer may define any number of sources (or none) and any number of actions (or none).
    //!   When evaluated during runtime using an action stack, it will overshadow input source events on layers below the top one,
    //!   which in turn allows to have multiple input contexts depending on the game state. Pushing or popping a layer can be done
    //!   at the end of each frame to prepare input processing for the next frame.
    //!
    //! \note The layer contains only the definitions of it's sources and actions. It does not contain runtime data
    //!   that is used. The runtime values are owned and handled by InputActionStack object, which is also responsible to
    //!   properly resolve references to the same actions or sources between layers.
    class InputActionLayer
    {
    public:
        virtual ~InputActionLayer() noexcept = default;

        //! \return Name of this action layer.
        virtual auto name() const noexcept -> ice::String = 0;

        //! \return List of all sources defined by this layer.
        virtual auto sources() const noexcept -> ice::Span<ice::InputActionSourceInputInfo const> = 0;

        //! \return Fetches the name of the given source, based on it's definition.
        virtual auto source_name(ice::InputActionSourceInputInfo const& source) const noexcept -> ice::String = 0;

        //! \return List of all actions defined by this layer.
        virtual auto actions() const noexcept -> ice::Span<ice::InputActionInfo const> = 0;

        //! \return Fetches the name of the given action, based on it's definition.
        virtual auto action_name(ice::InputActionInfo const& action) const noexcept -> ice::String = 0;

        //! \brief Updates all layer sources based on the input events passed.
        //! \param[in,out] events List of input events to be processed. If an event was processed, the data at that index
        //!   will be replaced with a value of `ice::input::InputEvent{}`.
        //! \param[in,out] source_values List of storage objects for each defined input source.
        //! \return Number of processed event's that have been consumed and reset.
        virtual auto process_inputs(
            ice::Span<ice::input::InputEvent> events,
            ice::Span<ice::InputActionSource* const> source_values
        ) const noexcept -> ice::ucount = 0;

        //! \brief Runs updates on all defined actions by this layer.
        //! \param[in] executor Executor object with context data, used to execute conditions, steps and modifiers.
        //! \param[in] source_values List of storage objects for each defined input source.
        //! \param[in,out] actions List of runtime objects for each action defined by this layer.
        //! \return `true`
        //!
        //! \todo Might need to rethink the return value, currently only returning true.
        virtual bool update_actions(
            ice::InputActionExecutor const& executor,
            ice::Span<ice::InputActionSource* const> source_values,
            ice::HashMap<ice::InputActionRuntime>& actions
        ) const noexcept = 0;
    };

    //! \brief Creates input action layer from binary data.
    //! \param alloc Allocator used to create the object and any other required objects.
    //! \param layer_data Binary definitions of an action layer.
    //! \return `InputActionLayer` object or `nullptr` if failed to parse input data.
    //!
    //! \todo As of now, the function may crash if parsing fails.
    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Data layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>;

    //! \copydoc ice::create_input_action_layer(ice::Allocator&,ice::Memory)
    auto create_input_action_layer(
        ice::Allocator& alloc,
        ice::Memory layer_data
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>;

    //! \brief Creates input action layer from script.
    //! \param alloc Allocator used to create the object and any other required objects.
    //! \param definition Script definitions of an input action layer.
    //! \return `InputActionLayer` object or `nullptr` if failed to parse input data.
    //!
    //! \todo As of now, the function may crash if parsing fails.
    auto parse_input_action_layer(
        ice::Allocator& alloc,
        ice::String definition
    ) noexcept -> ice::UniquePtr<ice::InputActionLayer>;

    //! \brief Creates a binary representation of an InputActionLayer object that can be loaded again later.
    //! \param alloc Allocator used to allocate the final Memory object.
    //! \param action_layer Layer to be saved in binary form.
    //! \return Allocated Memory object if operation was successful, empty if no data was saved.
    auto save_input_action_layer(
        ice::Allocator& alloc,
        ice::InputActionLayer const& action_layer
    ) noexcept -> ice::Expected<ice::Memory>;

} // namespace ice
