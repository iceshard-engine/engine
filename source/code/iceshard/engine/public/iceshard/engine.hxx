#pragma once
#include <core/allocator.hxx>
#include <asset_system/asset_system.hxx>
#include <input_system/system.hxx>

#include <iceshard/input/device/input_device_queue.hxx>
#include <iceshard/entity/entity_manager.hxx>
#include <iceshard/world/world_manager.hxx>
#include <iceshard/renderer/render_module.hxx>

#include <cppcoro/task.hpp>
#include <cppcoro/static_thread_pool.hpp>

namespace iceshard
{

    class Frame;

    class WorldManager;

    class EntityManager;

    class ExecutionInstance;

    class ServiceProvider;

    //! \brief The main class from the engine library.
    class Engine
    {
    public:
        virtual ~Engine() noexcept = default;

        auto render_system() noexcept -> iceshard::renderer::RenderSystem&;

        virtual auto revision() const noexcept -> uint32_t = 0;

        virtual auto asset_system() noexcept -> asset::AssetSystem& = 0;

        virtual auto input_system() noexcept -> ::input::InputSystem& = 0;

        virtual auto entity_manager() noexcept -> iceshard::EntityManager& = 0;

        virtual auto world_manager() noexcept -> iceshard::WorldManager& = 0;

        virtual auto worker_threads() noexcept -> cppcoro::static_thread_pool& = 0;

        virtual auto execution_instance() noexcept -> core::memory::unique_pointer<iceshard::ExecutionInstance> = 0;

        virtual auto services() noexcept -> iceshard::ServiceProvider& = 0;

    //public:
    //    virtual void add_task(cppcoro::task<> task) noexcept = 0;

    //    virtual auto previous_frame() const noexcept -> Frame const& = 0;

    //    virtual auto current_frame() noexcept -> Frame& = 0;

    //    void add_long_task(cppcoro::task<> task) noexcept;

    //protected:
        virtual auto render_module() noexcept -> iceshard::renderer::RenderModule& = 0;
    };

} // namespace iceshard
