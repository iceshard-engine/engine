#pragma once
#include <device/device.hxx>
#include <tuple>

namespace device
{


    //! \brief Supported mouse buttons.
    enum class MouseButton
    {
        Unknown

        , Left
        , Right
        , Middle

        , Custom0
    };


    //! \brief A simple mouse device.
    class MouseDevice : public Device
    {
    public:
        ~MouseDevice() noexcept override = default;

        //! \brief Checks the mouse device for scroll capability.
        virtual bool has_scroll() const noexcept = 0;

        //! \brief Checks the mouse device for custom buttons.
        virtual bool has_custom_buttons() const noexcept = 0;

        //! \brief Returns the current mouse position.
        virtual auto position() const noexcept -> std::tuple<uint32_t, uint32_t>;
    };


    /// OLD CODE ///

    //bool check_button(MouseButton button);

    //void mouse_position(int& x, int& y);

    //void set_relative_mouse_mode(bool relative);

    //bool relative_mouse_mode();

    //void show_cursor(bool show);

    /// OLD CODE ///


}
