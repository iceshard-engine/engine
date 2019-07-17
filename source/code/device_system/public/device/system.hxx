#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <core/string_types.hxx>
#include <core/pod/collections.hxx>
#include <device/device.hxx>

namespace device
{

    struct ProviderAPI
    {
        void(*query_devices_func)(core::pod::Array<device::Device*>& device_list);
        void(*reserved[16])() { nullptr };
    };


    //! \brief This class represents a driver object which is used to manage devices.
    class DeviceProvider
    {
    public:
        virtual ~DeviceProvider() noexcept = default;

        //! \brief Queries the current device driver for supported devices.
        virtual void query_devices(core::pod::Array<device::Device*>& device_list) noexcept = 0;
    };


    /// OLD CODE ///

    ////! \brief A interface for querying system inputs.
    //class InputProvider
    //{
    //public:
    //    virtual ~InputProvider() noexcept = default;

    //    //! \brief Called to gather all input events between calls.
    //    virtual void read_messages(core::MessageBuffer& message_queue) noexcept = 0;

    //    //! \brief Clears the internal input queue.
    //    virtual void clear_messages() noexcept = 0;
    //};


    //enum class Backend
    //{
    //    // Uses the SDL2 library as the backed with the target machine (windows, linux, macos)
    //    SDL2
    //};


    //// IO system related functions
    //class IOSystem;

    //IOSystem* initialize(core::allocator& alloc, Backend backend);

    //bool initialized(IOSystem* system);

    //void shutdown(IOSystem* system);

    //bool process_events(IOSystem* system);

    //void process_inputs(IOSystem* system);

    //// Clipboard API
    //const char* get_clipboard_text(IOSystem* system);

    //void set_clipboard_text(IOSystem* system, const char* text);

    //// Debug API
    //void send_debug_message(core::cexpr::stringid_argument_type name, int64_t value);

    //void send_tick_message();

    /// OLD CODE ///


} // namespace input
