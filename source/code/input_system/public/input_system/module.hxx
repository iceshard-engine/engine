#pragma once
#include <input_system/system.hxx>
#include <core/string_types.hxx>
#include <core/pointer.hxx>

namespace input
{


    //! \brief Describes a driver module object.
    class InputModule
    {
    public:
        virtual ~InputModule() noexcept = default;

        //! \brief Returns the associated media driver.
        [[nodiscard]]
        virtual auto input_system() noexcept -> InputSystem* = 0;

        //! \brief Returns the associated media driver.
        [[nodiscard]]
        virtual auto input_system() const noexcept -> const InputSystem* = 0;
    };


    //! \brief Tries to load a device driver from the given module path.
    [[nodiscard]]
    auto load_driver_module(core::allocator& alloc, core::StringView driver_module_path) noexcept -> core::memory::unique_pointer<InputModule>;


} // namespace input