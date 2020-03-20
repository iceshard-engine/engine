#include "iceshard_engine.hxx"

#include <resource/resource_system.hxx>
#include <asset_system/assets/asset_config.hxx>
#include <input_system/system.hxx>
#include <render_system/render_system.hxx>
#include <render_system/render_commands.hxx>

#include <core/string.hxx>
#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>

#include <atomic>
#include <vector>

#include <cppcoro/task.hpp>
#include <cppcoro/static_thread_pool.hpp>
#include "intellisense_vsbug_workaround.hxx"

namespace iceshard
{
    namespace detail
    {

        static constexpr auto FrameAllocatorCapacity = 256u * 1024u * 1024u;

        struct FrameTask
        {
            cppcoro::task<> object;
            FrameTask* next_task = nullptr;
        };

        using iceshard::renderer::RenderPass;
        using iceshard::renderer::RenderPassStage;

        class NoneRenderSystem : public render::RenderSystem
        {
        public:

            auto create_buffer(iceshard::renderer::api::BufferType, uint32_t) noexcept -> iceshard::renderer::api::Buffer override { return iceshard::renderer::api::Buffer{ 0 }; }

            void initialize_render_interface(iceshard::renderer::api::RenderInterface**) noexcept override { }

            auto create_resource_set(
                core::stringid_arg_type /*name*/,
                iceshard::renderer::RenderPipelineLayout /*layout*/,
                core::pod::Array<iceshard::renderer::RenderResource> const& /*resources*/
            ) noexcept -> iceshard::renderer::ResourceSet override
            {
                return iceshard::renderer::ResourceSet::Invalid;
            }

            void update_resource_set(
                core::stringid_arg_type /*name*/,
                core::pod::Array<iceshard::renderer::RenderResource> const& /*resources*/
            ) noexcept override
            {
            }

            void destroy_resource_set(
                core::stringid_arg_type /*name*/
            ) noexcept override
            {
            }

            auto load_texture(asset::AssetData) noexcept -> iceshard::renderer::api::Texture override { return iceshard::renderer::api::Texture{ 0 }; }

            auto acquire_command_buffer(RenderPassStage) noexcept -> iceshard::renderer::CommandBuffer
            {
                return iceshard::renderer::CommandBuffer::Invalid;
            }

            void submit_command_buffer(iceshard::renderer::CommandBuffer) noexcept { }

            auto create_pipeline(
                [[maybe_unused]] core::stringid_arg_type name,
                [[maybe_unused]] iceshard::renderer::RenderPipelineLayout layout,
                [[maybe_unused]] core::pod::Array<asset::AssetData> const& shader_assets
            ) noexcept -> iceshard::renderer::Pipeline override
            {
                return iceshard::renderer::Pipeline{ 0 };
            }

            void destroy_pipeline(
                [[maybe_unused]] core::stringid_arg_type name
            ) noexcept override
            {
            }
        };

        class NoneRenderSystemModule : public iceshard::renderer::RenderSystemModule
        {
        public:
            //! \brief Returns the engine object from the loaded module.
            [[nodiscard]]
            auto render_system() noexcept -> render::RenderSystem* override { return &_render_system; }

            //! \brief Returns the engine object from the loaded module.
            [[nodiscard]]
            auto render_system() const noexcept -> render::RenderSystem const* override { return &_render_system; }

            //! \brief Returns the render api interface from the loaded module.
            [[nodiscard]]
            auto render_api() noexcept -> iceshard::renderer::api::RenderInterface* override { return iceshard::renderer::api::render_api_instance; }

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

    IceShardEngine::IceShardEngine(core::allocator& alloc, resource::ResourceSystem& resources) noexcept
        : _allocator{ "iceshard-engine", alloc }
        , _resources{ resources }
        , _asset_system{ _allocator, resources }
        , _render_module{ core::memory::make_unique<iceshard::renderer::RenderSystemModule, detail::NoneRenderSystemModule>(_allocator) }
        // Task list
        , _mutable_task_list{ &_frame_tasks[_task_list_index] }
        // Frame related fields
        , _frame_allocator{ _allocator, sizeof(MemoryFrame) * 5 }
        , _frame_data_allocator{ { _allocator, detail::FrameAllocatorCapacity }, { _allocator, detail::FrameAllocatorCapacity } }
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
            auto* render_driver_location = resources.find(resource::URN{ config.render_driver_location });
            IS_ASSERT(render_driver_location != nullptr, "Missing driver for rendering module!");

            _render_module = iceshard::renderer::load_render_system_module(_allocator, render_driver_location->location().path);
            IS_ASSERT(_render_module != nullptr, "Invalid Vulkan driver module! Unable to load!");

            iceshard::renderer::api::render_api_instance = _render_module->render_api();
        }

        _entity_manager = core::memory::make_unique<iceshard::EntityManager>(_allocator, _allocator);
        _serivce_provider = core::memory::make_unique<iceshard::IceshardServiceProvider>(_allocator, _allocator, _entity_manager.get());

        _world_manager = core::memory::make_unique<iceshard::IceshardWorldManager>(_allocator, _allocator, _serivce_provider.get());
    }

    IceShardEngine::~IceShardEngine() noexcept
    {
        _world_manager = nullptr;

        _serivce_provider = nullptr;
        _entity_manager = nullptr;
        _input_module = nullptr;

        _current_frame = nullptr;
        _previous_frame = nullptr;
    }

    auto IceShardEngine::asset_system() noexcept -> asset::AssetSystem*
    {
        return &_asset_system;
    }

    auto IceShardEngine::input_system() noexcept -> input::InputSystem*
    {
        return _input_module->input_system();
    }

    auto IceShardEngine::entity_manager() noexcept -> iceshard::EntityManager*
    {
        return _entity_manager.get();
    }

    auto IceShardEngine::world_manager() noexcept -> iceshard::WorldManager*
    {
        return _world_manager.get();
    }

    auto IceShardEngine::previous_frame() const noexcept -> const Frame&
    {
        return *_previous_frame;
    }

    auto IceShardEngine::current_frame() noexcept -> Frame&
    {
        return *_current_frame;
    }

    void IceShardEngine::next_frame() noexcept
    {
        {
            _task_list_index += 1;
            std::vector<cppcoro::task<>>* expected_list = &_frame_tasks[(_task_list_index - 1) % 2];
            std::vector<cppcoro::task<>>* exchange_list = &_frame_tasks[_task_list_index % 2];

            while (_mutable_task_list.compare_exchange_weak(expected_list, exchange_list) == false)
            {
                expected_list = &_frame_tasks[(_task_list_index - 1) % 2];
            }

            vs_hacks::cppcoro_sync_all_workaround(std::move(*expected_list));
        }

        _render_module->render_system()->end_frame();

        // Move the current frame to the 'previous' slot.
        _previous_frame = std::move(_current_frame);

        // Reset the frame allocator inner pointers.
        [[maybe_unused]] const bool successful_reset = _frame_data_allocator[_next_free_allocator].reset();
        IS_ASSERT(successful_reset == true, "Memory was discarded during frame allocator reset!");

        _current_frame = core::memory::make_unique<MemoryFrame>(_frame_allocator, _frame_data_allocator[_next_free_allocator]);

        // We need to update the allocator index
        _next_free_allocator += 1;
        _next_free_allocator %= std::size(_frame_data_allocator);

        // Now we want to get all messages for the current frame.
        auto* inputs = input_system();
        inputs->query_messages(_current_frame->messages());

        _render_module->render_system()->begin_frame();
    }

    auto IceShardEngine::worker_threads() noexcept -> cppcoro::static_thread_pool&
    {
        return _worker_pool;
    }

    void IceShardEngine::add_task(cppcoro::task<> task) noexcept
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

    auto IceShardEngine::render_system(iceshard::renderer::api::RenderInterface*& render_api) noexcept -> render::RenderSystem*
    {
        render_api = iceshard::renderer::api::render_api_instance;
        return _render_module->render_system();
    }

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
