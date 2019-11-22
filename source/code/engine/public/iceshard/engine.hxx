#pragma once
#include <iceshard/world/world_manager.hxx>
#include <iceshard/entity/entity_manager.hxx>
#include <iceshard/service_provider.hxx>

#include <input_system/system.hxx>
#include <core/allocator.hxx>
#include <core/pointer.hxx>

#include <functional>
#include <memory>

#include <cppcoro/task.hpp>
#include <cppcoro/static_thread_pool.hpp>

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
        virtual auto input_system() noexcept -> input::InputSystem* = 0;

    public:
        virtual auto entity_manager() noexcept -> iceshard::EntityManager* = 0;

        virtual auto world_manager() noexcept -> iceshard::WorldManager* = 0;

    public:
        //! \brief Returns the previous frame object.
        virtual auto previous_frame() const noexcept -> const Frame& = 0;

        //! \brief Returns the next frame object with all systems updated and waiting for their task completion.
        virtual auto current_frame() noexcept -> Frame& = 0;

        //! \brief Updates the engine internal state for the next frame.
        virtual void next_frame() noexcept = 0;

    public:
        virtual auto worker_threads() noexcept -> cppcoro::static_thread_pool& = 0;

    public:
        //! \brief Creates a task in the current frame.
        void create_task(std::function<cppcoro::task<>(core::allocator&)>&& task) noexcept;

        //! \brief Creates a task in the current frame.
        void create_task(std::function<cppcoro::task<>(iceshard::Frame&)>&& task) noexcept;

        //! \brief Creates a task in the current frame.
        void create_task(std::function<cppcoro::task<>(iceshard::Engine&)>&& task) noexcept;

        //! \brief Creates a task in the current frame.
        void create_long_task(std::function<cppcoro::task<>(iceshard::Engine&)>&& task) noexcept;

    protected:
        //! \brief Adds a task to the current frame.
        virtual void add_task(cppcoro::task<> task) noexcept = 0;
    };

} // namespace iceshard
