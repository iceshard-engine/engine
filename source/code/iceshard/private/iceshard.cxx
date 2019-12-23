#include <iceshard/engine.hxx>

#include <resource/resource_system.hxx>
#include <asset_system/assets/asset_config.hxx>
#include <input_system/system.hxx>
#include <input_system/module.hxx>
#include <render_system/render_module.hxx>
#include <render_system/render_system.hxx>

#include <core/string.hxx>
#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>

#include <cppcoro/task.hpp>
#include <cppcoro/sync_wait.hpp>
#include <cppcoro/when_all_ready.hpp>
#include <cppcoro/static_thread_pool.hpp>

#include "frame.hxx"
#include "world/iceshard_world_manager.hxx"
#include "iceshard_service_provider.hxx"

#include <atomic>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace iceshard
{
    namespace detail
    {

        static constexpr auto MiB = 1024u * 1024u;

        static constexpr auto FrameAllocatorCapacity = 256u * detail::MiB;

        struct FrameTask
        {
            cppcoro::task<> object;
            FrameTask* next_task = nullptr;
        };

        class NoneRenderSystem : public render::RenderSystem
        {
        public:
            auto command_buffer() noexcept -> render::CommandBufferHandle override { return render::CommandBufferHandle{ 0 }; }
            void swap() noexcept override { }
        };

        class NoneRenderSystemModule : public render::RenderSystemModule
        {
        public:
            //! \brief Returns the engine object from the loaded module.
            [[nodiscard]]
            auto render_system() noexcept -> render::RenderSystem* override { return &_render_system; }

            //! \brief Returns the engine object from the loaded module.
            [[nodiscard]]
            auto render_system() const noexcept -> render::RenderSystem const* override { return &_render_system; }

        private:
            NoneRenderSystem _render_system;
        };

        struct Config
        {
            core::allocator& allocator;
            core::String<> render_driver_location{ core::memory::globals::null_allocator() };
        };

        void load_config_file(asset::AssetConfigObject const& config_object, Config& config) noexcept
        {
            if (config_object.Has("render_drivers") && config_object.Has("render_drivers"))
            {
                auto const& drivers = config_object.ObjectValue("render_drivers");
                if (drivers.Has(config_object.StringValue("render_driver")))
                {
                    auto const& selected_driver = drivers.ObjectValue(config_object.StringValue("render_driver"));
                    IS_ASSERT(selected_driver.Has("name"), "Missing render driver `name`!");

                    fmt::print("Selected render driver: {}\n", selected_driver.StringValue("name"));
                    if (selected_driver.Has("resource"))
                    {
                        config.render_driver_location = core::String{ config.allocator, selected_driver.StringValue("resource") };
                    }
                }
            }
        }

    } // namespace detail

    class IceShardEngine final : public iceshard::Engine
    {
    public:
        IceShardEngine(core::allocator& alloc, resource::ResourceSystem& resources) noexcept
            : _allocator{ "iceshard-engine", alloc }
            , _resources{ resources }
            , _asset_system{ _allocator, resources }
            , _input_module{ nullptr, { _allocator } }
            , _render_module{ core::memory::make_unique<render::RenderSystemModule, detail::NoneRenderSystemModule>(_allocator) }
            // Managers
            , _entity_manager{ nullptr, { _allocator } }
            , _serivce_provider{ nullptr, { _allocator } }
            , _world_manager{ nullptr, { _allocator } }
            // Task allocator
            , _mutable_task_list{ &_frame_tasks[_task_list_index] }
            // Frames allocators
            , _frame_allocator{ _allocator, sizeof(MemoryFrame) * 5 }
            , _frame_data_allocator{ { _allocator, detail::FrameAllocatorCapacity }, { _allocator, detail::FrameAllocatorCapacity } }
            // Frames
            , _previous_frame{ core::memory::make_unique<MemoryFrame>(_frame_allocator, _frame_data_allocator[0]) }
            , _current_frame{ core::memory::make_unique<MemoryFrame>(_frame_allocator, _frame_data_allocator[1]) }
        {
            _asset_system.update();

            detail::Config config{ _allocator, core::String{ _allocator, "vulkan_driver.dll" } };
            asset::AssetData config_data;
            if (_asset_system.load(asset::AssetConfig{ "config" }, config_data) == asset::AssetStatus::Loaded)
            {
                detail::load_config_file(asset::AssetConfigObject{ _allocator, config_data }, config);
            }

            {
                auto* sdl_driver_module_location = resources.find({ "sdl2_driver.dll" });
                IS_ASSERT(sdl_driver_module_location != nullptr, "Missing SDL2 driver module!");

                _input_module = input::load_driver_module(_allocator, sdl_driver_module_location->location().path);
                IS_ASSERT(_input_module != nullptr, "Invalid SDL2 driver module! Unable to load!");
            }

            if (core::string::empty(config.render_driver_location) == false)
            {
                auto* render_driver_location = resources.find({ config.render_driver_location });
                IS_ASSERT(render_driver_location != nullptr, "Missing driver for rendering module!");

                _render_module = render::load_render_system_module(_allocator, render_driver_location->location().path);
                IS_ASSERT(_render_module != nullptr, "Invalid Vulkan driver module! Unable to load!");
            }

            _entity_manager = core::memory::make_unique<iceshard::EntityManager>(_allocator, _allocator);
            _serivce_provider = core::memory::make_unique<iceshard::IceshardServiceProvider>(_allocator, _allocator, _entity_manager.get());

            _world_manager = core::memory::make_unique<iceshard::IceshardWorldManager>(_allocator, _allocator, _serivce_provider.get());
        }

        ~IceShardEngine() noexcept
        {
            _world_manager = nullptr;

            _serivce_provider = nullptr;
            _entity_manager = nullptr;
            _input_module = nullptr;

            _current_frame = nullptr;
            _previous_frame = nullptr;
        }

        auto revision() const noexcept -> uint32_t override
        {
            return 1;
        }

        auto asset_system() noexcept -> asset::AssetSystem*
        {
            return &_asset_system;
        }

        auto input_system() noexcept -> input::InputSystem*
        {
            return _input_module->input_system();
        }

        auto entity_manager() noexcept -> iceshard::EntityManager* override
        {
            return _entity_manager.get();
        }

        auto world_manager() noexcept -> iceshard::WorldManager* override
        {
            return _world_manager.get();
        }

        auto previous_frame() const noexcept -> const Frame& override
        {
            return *_previous_frame;
        }

        auto current_frame() noexcept -> Frame& override
        {
            return *_current_frame;
        }

        void next_frame() noexcept override
        {
            {
                _task_list_index += 1;
                std::vector<cppcoro::task<>>* expected_list = &_frame_tasks[(_task_list_index - 1) % 2];
                std::vector<cppcoro::task<>>* exchange_list = &_frame_tasks[_task_list_index % 2];

                while (_mutable_task_list.compare_exchange_weak(expected_list, exchange_list) == false)
                {
                    expected_list = &_frame_tasks[(_task_list_index - 1) % 2];
                }

                cppcoro::sync_wait(cppcoro::when_all_ready(std::move(*expected_list)));
            }

            _render_module->render_system()->swap();

            // Move the current frame to the 'previous' slot.
            _previous_frame = std::move(_current_frame);

            // Reset the frame allocator inner pointers.
            [[maybe_unused]]
            const bool successful_reset = _frame_data_allocator[_next_free_allocator].reset();
            IS_ASSERT(successful_reset == true, "Memory was discarded during frame allocator reset!");

            _current_frame = core::memory::make_unique<MemoryFrame>(_frame_allocator, _frame_data_allocator[_next_free_allocator]);

            // We need to update the allocator index
            _next_free_allocator += 1;
            _next_free_allocator %= std::size(_frame_data_allocator);

            // Now we want to get all messages for the current frame.
            auto* inputs = input_system();
            inputs->query_messages(_current_frame->messages());
        }

        auto worker_threads() noexcept -> cppcoro::static_thread_pool& override
        {
            return _worker_pool;
        }

        void add_task(cppcoro::task<> task) noexcept override
        {
            std::vector<cppcoro::task<>>* expected_list = &_frame_tasks[_task_list_index % 2];
            {
                while (_mutable_task_list.compare_exchange_weak(expected_list, nullptr) == false)
                {
                    expected_list = &_frame_tasks[_task_list_index % 2];
                }
            }

            expected_list->push_back(std::move(task));

            _mutable_task_list.store(expected_list);
        }

    private:
        core::memory::proxy_allocator _allocator;

        // Resource systems.
        resource::ResourceSystem& _resources;

        asset::AssetSystem _asset_system;

        core::memory::unique_pointer<input::InputModule> _input_module;
        core::memory::unique_pointer<render::RenderSystemModule> _render_module;

        // Managers and service provider
        core::memory::unique_pointer<iceshard::EntityManager> _entity_manager;
        core::memory::unique_pointer<iceshard::IceshardServiceProvider> _serivce_provider;

        core::memory::unique_pointer<iceshard::IceshardWorldManager> _world_manager;

        // Thread pool of the engine.
        cppcoro::static_thread_pool _worker_pool{};

        // Tasks to be run this frame.
        size_t _task_list_index = 0;
        std::vector<cppcoro::task<>> _frame_tasks[2]{ {}, {} };

        std::atomic<std::vector<cppcoro::task<>>*> _mutable_task_list = nullptr;

        // Frame allocators.
        uint32_t _next_free_allocator = 0;

        core::memory::scratch_allocator _frame_allocator;
        core::memory::scratch_allocator _frame_data_allocator[2];

        // Frames.
        core::memory::unique_pointer<MemoryFrame> _previous_frame;
        core::memory::unique_pointer<MemoryFrame> _current_frame;
    };

} // namespace iceshard

extern "C"
{
    __declspec(dllexport) auto create_engine(core::allocator& alloc, resource::ResourceSystem& resources) -> iceshard::Engine*
    {
        return alloc.make<iceshard::IceShardEngine>(alloc, resources);
    }

    __declspec(dllexport) void release_engine(core::allocator& alloc, iceshard::Engine* engine)
    {
        alloc.destroy(engine);
    }
}
