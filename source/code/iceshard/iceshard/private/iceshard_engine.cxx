#include "iceshard_engine.hxx"

#include <resource/resource_system.hxx>
#include <asset_system/assets/asset_config.hxx>
#include <input_system/system.hxx>

#include <core/string.hxx>
#include <core/memory.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>

#include <atomic>
#include <vector>

#include <cppcoro/task.hpp>
#include <cppcoro/static_thread_pool.hpp>

#include "systems/iceshard_camera_system.hxx"
#include "systems/iceshard_lights_system.hxx"
#include "systems/iceshard_static_mesh_renderer.hxx"

#include "rendering/ice_render_system.hxx"

namespace iceshard
{
    namespace detail
    {

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
        , _render_module{ nullptr, { core::memory::globals::null_allocator() } }
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

            _input_module = ::input::load_driver_module(_allocator, sdl_driver_module_location->location().path);
            IS_ASSERT(_input_module != nullptr, "Invalid SDL2 driver module! Unable to load!");
        }

        if (core::string::empty(config.render_driver_location) == false)
        {
            auto* render_driver_location = resources.find(resource::URN{ config.render_driver_location });
            IS_ASSERT(render_driver_location != nullptr, "Missing driver for rendering module!");

            _render_module = iceshard::renderer::load_render_system_module(_allocator, render_driver_location->location().path);
            IS_ASSERT(_render_module != nullptr, "Invalid Vulkan driver module! Unable to load!");
        }

        _material_system = core::memory::make_unique<iceshard::IceshardMaterialSystem>(
            _allocator,
            _allocator,
            _asset_system,
            *render_module().render_system()
        );

        _entity_manager = core::memory::make_unique<iceshard::EntityManager>(_allocator, _allocator);
        _serivce_provider = core::memory::make_unique<iceshard::IceshardServiceProvider>(
            _allocator,
            _allocator,
            _entity_manager.get()
        );

        _world_manager = core::memory::make_unique<iceshard::IceshardWorldManager>(
            _allocator,
            _allocator,
            *_serivce_provider
        );
    }

    IceShardEngine::~IceShardEngine() noexcept
    {
        _world_manager = nullptr;

        _serivce_provider = nullptr;
        _entity_manager = nullptr;
        _material_system = nullptr;
        _input_module = nullptr;
    }

    auto IceShardEngine::asset_system() noexcept -> asset::AssetSystem&
    {
        return _asset_system;
    }

    auto IceShardEngine::input_system() noexcept -> ::input::InputSystem&
    {
        return *_input_module->input_system();
    }

    auto IceShardEngine::material_system() noexcept -> iceshard::MaterialSystem&
    {
        return *_material_system;
    }

    auto IceShardEngine::entity_manager() noexcept -> iceshard::EntityManager&
    {
        return *_entity_manager;
    }

    auto IceShardEngine::world_manager() noexcept -> iceshard::WorldManager&
    {
        return *_world_manager;
    }

    auto IceShardEngine::worker_threads() noexcept -> cppcoro::static_thread_pool&
    {
        return _worker_pool;
    }

    auto IceShardEngine::execution_instance() noexcept -> core::memory::unique_pointer<iceshard::ExecutionInstance>
    {
        core::memory::unique_pointer<iceshard::ExecutionInstance> result{ nullptr, { _allocator } };
        if (_execution_instance_lock == nullptr)
        {
            result = core::memory::make_unique<iceshard::ExecutionInstance, iceshard::IceshardExecutionInstance>(
                _allocator,
                _allocator,
                ExecutionLock(&_execution_instance_lock),
                *this,
                *_serivce_provider
            );

            _execution_instance_lock = result.get();
        }

        return result;
    }

    auto IceShardEngine::services() noexcept -> iceshard::ServiceProvider&
    {
        return *_serivce_provider;
    }

    auto IceShardEngine::render_module() noexcept -> iceshard::renderer::RenderModule&
    {
        return *_render_module->render_module();
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
