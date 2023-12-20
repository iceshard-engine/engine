/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/mem_data.hxx>
#include <ice/clock.hxx>
#include <ice/container/array.hxx>
#include <ice/input/device_event.hxx>
#include <ice/input/input_event.hxx>

namespace ice::input
{

    class InputDevice
    {
    public:
        virtual ~InputDevice() noexcept = default;

        //! \return Number of maximum supported devices.
        //!
        //! \note In some cases, like a touch screen, a single touch pointer is seen as a single device.
        //!     Multi touch is implemented using this value.
        //! \note A total of 15 different devices of the same type can be handled based on engine limits.
        virtual auto max_count() const noexcept -> ice::ucount { return 1u; }

        //! \return Number of currently connected devices.
        virtual auto count() const noexcept -> ice::ucount { return 1u; }

        //! \return Handle to a connected device.
        //! \note After accessing a handle, the user is still required to check if the handle valid.
        //! \note Use count() to check how many device handles are available. This values shouldn't be cached as it can be invalidated at any time.
        virtual auto handle(ice::u32 device_index = 0) const noexcept -> ice::input::DeviceHandle = 0;

        virtual void on_tick(ice::Timer const& timer) noexcept = 0;
        virtual void on_event(ice::input::DeviceEvent event) noexcept = 0;
        virtual void on_publish(ice::Array<ice::input::InputEvent>& events_out) noexcept = 0;
    };

} // ice::input
