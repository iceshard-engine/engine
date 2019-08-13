#pragma once
#include <core/allocator.hxx>
#include <input_system/system.hxx>

namespace iceshard
{


    //! \brief The main class from the engine library.
    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        //! \brief Returns the engine revision.
        virtual auto revision() const noexcept -> uint32_t = 0;

        //! \brief Returns the used input system object.
        virtual auto input_system() const noexcept -> input::InputSystem* = 0;
    };


} // namespace iceshard
