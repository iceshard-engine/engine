#pragma once
#include <device/device.hxx>

namespace device
{


    //! \brief Display device backed system.
    enum class DisplayBackend
    {
        Unknown
        , Vulkan
        , OpenGL
        , DirectX
        , Metal
    };


    //! \brief A single display device.
    class DisplayDevice : public Device
    {
    public:
        ~DisplayDevice() noexcept override = default;

        //! \brief Display backed system.
        virtual auto backend_system() const noexcept -> DisplayBackend = 0;
    };


    /// OLD CODE ///

    //class IOSystem;

    //struct Window;

    //Window* render_window(IOSystem* system);

    //const char* window_title(Window* window);

    //void set_window_title(Window* window, const char* title);

    //void window_position(Window* window, int& x, int &y);

    //void window_drawable_size(Window* window, int& width, int& height);

    //void window_size(Window* window, int& width, int& height);

    //bool window_has_focus(Window* window);

    //void window_show(Window* window);

    //void window_hide(Window* window);

    //void* window_handle(Window* window);

    //void* window_native_handle(Window* window);

    /// OLD CODE ///


} // namespace device
