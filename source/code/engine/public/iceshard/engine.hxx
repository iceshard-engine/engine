#pragma once
#include <core/allocator.hxx>
#include <core/pointer.hxx>
#include <input_system/system.hxx>

#include <cppcoro/task.hpp>
#include <functional>

namespace iceshard
{


    class Frame;


    //! \brief The main class from the engine library.
    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        //! \brief Returns the engine revision.
        virtual auto revision() const noexcept -> uint32_t = 0;

    public:
        //! \brief Returns the used input system object.
        virtual auto input_system() const noexcept -> input::InputSystem* = 0;

    public:
        //! \brief Returns the previous frame object.
        virtual auto previous_frame() const noexcept -> const Frame& = 0;

        //! \brief Returns the next frame object with all systems updated and waiting for their task completion.
        virtual auto current_frame() noexcept -> Frame& = 0;

        //! \brief Updates the engine internal state for the next frame.
        virtual void next_frame() noexcept = 0;

        //! \brief Creates a task in the current frame.
        void create_task(std::function<cppcoro::task<>(core::allocator&)> task) noexcept;

        //! \brief Creates a task in the current frame.
        void create_task(std::function<cppcoro::task<>(iceshard::Frame&)> task) noexcept;

    protected:
        //! \brief Adds a task to the current frame.
        virtual void add_task(cppcoro::task<> task) noexcept = 0;
    };


} // namespace iceshard
