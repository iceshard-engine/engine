#pragma once
#include <ice/game_framework.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/game_camera.hxx>

#include <ice/memory/proxy_allocator.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>

#include <ice/entity/entity_index.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_query.hxx>
#include <ice/archetype/archetype_block_allocator.hxx>

#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_manager.hxx>

#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_frame.hxx>

#include <ice/module_register.hxx>
#include <ice/resource_system.hxx>
#include <ice/resource.hxx>
#include <ice/assert.hxx>


using ice::operator""_sid;
using ice::operator""_uri;

struct TestComponent
{
    static constexpr ice::StringID Identifier = "test.component"_sid;

    ice::i32 entity_index;
};
struct TestComponent2
{
    static constexpr ice::StringID Identifier = "test.component2"_sid;

    ice::i32 a;
};
struct TestComponent3
{
    static constexpr ice::StringID Identifier = "test.component3"_sid;

    ice::i32 b;
};

class MyGame : public ice::WorldTrait
{
public:
    static constexpr ice::URI ConfigFile = "file://../source/data/config.json"_uri;

    MyGame(ice::Allocator& alloc, ice::Clock const& clock) noexcept
        : _allocator{ alloc, "MyGame-alloc" }
        , _clock{ clock }
        , _current_engine{ nullptr }
        , _archetype_alloc{ _allocator }
        , _archetype_index{ ice::create_archetype_index(_allocator) }
        , _entity_storage{ _allocator, *_archetype_index, _archetype_alloc }
        , _game_gfx_pass{ ice::gfx::create_dynamic_pass(_allocator) }
        , _army{ _allocator }
    {
    }

    void on_load_modules(ice::GameServices& sercies) noexcept
    {
        ice::ModuleRegister& mod = sercies.module_registry();
        ice::ResourceSystem& res = sercies.resource_system();

        ice::Resource* const pipelines_module = res.request("res://iceshard_pipelines.dll"_uri);
        ice::Resource* const engine_module = res.request("res://iceshard.dll"_uri);
        ice::Resource* const vulkan_module = res.request("res://vulkan_renderer.dll"_uri);

        ICE_ASSERT(pipelines_module != nullptr, "Missing `iceshard_pipelines.dll` module!");
        ICE_ASSERT(engine_module != nullptr, "Missing `iceshard.dll` module!");
        ICE_ASSERT(vulkan_module != nullptr, "Missing `vulkan_renderer.dll` module!");

        mod.load_module(_allocator, pipelines_module->location().path);
        mod.load_module(_allocator, engine_module->location().path);
        mod.load_module(_allocator, vulkan_module->location().path);
    }

    void on_app_startup(ice::Engine& engine) noexcept
    {
        _current_engine = &engine;
        ICE_LOG(
            ice::LogSeverity::Debug, ice::LogTag::Game,
            "Hello, world!"
        );

        _trait_render_gfx = ice::create_trait_render_gfx(_allocator);
        _trait_render_clear = ice::create_trait_render_clear(_allocator);
        _trait_render_finish = ice::create_trait_render_finish(_allocator);
        _trait_render_camera = ice::create_trait_camera(_allocator);


        _game_gfx_pass->add_stages(_trait_render_gfx->gfx_stage_infos());
        _game_gfx_pass->add_stages(_trait_render_clear->gfx_stage_infos());
        _game_gfx_pass->add_stages(_trait_render_finish->gfx_stage_infos());


        ice::WorldManager& world_manager = engine.world_manager();
        _test_world = world_manager.create_world("game.test_world"_sid, &_entity_storage);
        _test_world->add_trait("ice.render_gfx"_sid, _trait_render_gfx.get());
        _test_world->add_trait("ice.render_clear"_sid, _trait_render_clear.get());
        _test_world->add_trait("ice.render_finish"_sid, _trait_render_finish.get());
        _test_world->add_trait("ice.camera"_sid, _trait_render_camera.get());
        _test_world->add_trait("game"_sid, this);

        ice::ArchetypeHandle ortho_arch = _archetype_index->register_archetype<ice::Camera, ice::CameraOrtho>(&_archetype_alloc);
        ice::ArchetypeHandle persp_arch = _archetype_index->register_archetype<ice::Camera, ice::CameraPerspective>(&_archetype_alloc);

        //ice::ArchetypeHandle test_arch = _archetype_index->register_archetype<TestComponent2, TestComponent3, TestComponent>(&_archetype_alloc);

        //ice::ComponentQuery<TestComponent const&, TestComponent2 const&, TestComponent3 const&> test_query{ _allocator, *_archetype_index };

        //ice::pod::array::resize(_army, 300'000);
        //engine.entity_index().create_many(_army);


        //ice::StringID const components[3]{ TestComponent::Identifier, TestComponent2::Identifier, TestComponent3::Identifier };
        //ice::u32 const sizes[3]{ sizeof(TestComponent), sizeof(TestComponent2), sizeof(TestComponent3) };
        //ice::u32 const alignments[3]{ alignof(TestComponent), alignof(TestComponent2), alignof(TestComponent3) };

        //ice::u32 const total_count = ice::pod::array::size(_army);
        //ice::u32 const total_data_size = sizes[0] * total_count * 2;
        //ice::u32 const total_data_size2 = sizes[1] * total_count * 2;
        //ice::u32 const total_data_size3 = sizes[2] * total_count * 2;

        //void* data = _allocator.allocate(total_data_size + total_data_size2 + total_data_size3, alignments[0]);
        //TestComponent* component_data = reinterpret_cast<TestComponent*>(data);
        //TestComponent2* component_data2 = reinterpret_cast<TestComponent2*>(component_data + total_count);
        //TestComponent3* component_data3 = reinterpret_cast<TestComponent3*>(component_data2 + total_count);

        //ice::u32 const offsets[3]{ 0, total_data_size, total_data_size + total_data_size2 };

        //for (ice::i32 idx = 0; idx < total_count * 2; ++idx)
        //{
        //    component_data[idx].entity_index = idx;
        //    component_data2[idx].a = -1;
        //    component_data3[idx].b = 100;
        //}

        //ice::ArchetypeInfo const data_archetype{
        //    .block_allocator = nullptr,
        //    .block_base_alignment = 0,
        //    .block_max_entity_count = total_count,
        //    .components = components,
        //    .sizes = sizes,
        //    .alignments = alignments,
        //    .offsets = offsets,
        //};

        //ice::ArchetypeOperation operation{
        //    .source_archetype = &data_archetype,
        //    .source_data = ice::data_view(data, total_data_size, alignments[0]),
        //    .source_data_offset = 0,
        //    .source_entity_count = total_count
        //};


        //_entity_storage.set_archetypes(test_arch, _army, operation);

        //engine.entity_index().create_many(_army);
        //operation.source_data_offset = total_count,
        //_entity_storage.set_archetypes(test_arch, _army, operation);

        //auto test_result = test_query.result_by_block(_allocator, _entity_storage);
        //ICE_LOG(
        //    ice::LogSeverity::Debug, ice::LogTag::Game,
        //    "Query found {} entities.",
        //    test_result.entity_count()
        //);

        //ice::u32 block_count = 0;
        //test_result.for_each([&block_count](ice::u32 entity_count, TestComponent const* test_array, TestComponent2 const* test2, TestComponent3 const* test3) noexcept
        //    {
        //        ICE_LOG(
        //            ice::LogSeverity::Debug, ice::LogTag::Game,
        //            "Block starts with entity index {} and ends with index {}.",
        //            test_array[0].entity_index + test2[0].a + test3[0].b,
        //            test_array[entity_count - 1].entity_index + test2[entity_count - 1].a + test3[entity_count - 1].b
        //        );

        //        block_count += 1;
        //    }
        //);

        //ICE_LOG(
        //    ice::LogSeverity::Debug, ice::LogTag::Game,
        //    "Total of {} blocks where created!",
        //    block_count
        //);

        //_allocator.deallocate(data);
    }

    void on_app_shutdown(ice::Engine& engine) noexcept
    {
        _test_world->remove_trait("game"_sid);
        _test_world->remove_trait("ice.camera"_sid);
        _test_world->remove_trait("ice.render_finish"_sid);
        _test_world->remove_trait("ice.render_clear"_sid);
        _test_world->remove_trait("ice.render_gfx"_sid);

        ice::WorldManager& world_manager = engine.world_manager();
        world_manager.destroy_world("game.test_world"_sid);

        _trait_render_camera = nullptr;
        _trait_render_finish = nullptr;
        _trait_render_clear = nullptr;
        _trait_render_gfx = nullptr;
        _game_gfx_pass = nullptr;

        ICE_LOG(
            ice::LogSeverity::Debug, ice::LogTag::Game,
            "Goodbye, world!"
        );
        _current_engine = nullptr;
    }

    void on_game_begin(ice::EngineRunner& runner) noexcept;

    void on_game_end() noexcept;


    void on_update(
        ice::EngineFrame& frame,
        ice::EngineRunner& runner,
        ice::WorldPortal& portal
    ) noexcept
    {
        runner.graphics_frame().enqueue_pass("default"_sid, _game_gfx_pass.get());
    }

protected:
    struct TraitContainer;
    TraitContainer* _traits = nullptr;
    void add_world_traits(
        ice::Engine& engine,
        ice::World* world
    ) noexcept;
    void remove_world_traits(ice::World* world) noexcept;

public:
    ice::memory::ProxyAllocator _allocator;
    ice::Clock const& _clock;

    ice::Engine* _current_engine;

    ice::ArchetypeBlockAllocator _archetype_alloc;
    ice::UniquePtr<ice::ArchetypeIndex> _archetype_index;
    ice::EntityStorage _entity_storage;

    ice::UniquePtr<ice::gfx::GfxDynamicPass> _game_gfx_pass;
    ice::UniquePtr<ice::GameWorldTrait_Render> _trait_render_gfx{ ice::make_unique_null<ice::GameWorldTrait_Render>() };
    ice::UniquePtr<ice::GameWorldTrait_Render> _trait_render_clear{ ice::make_unique_null<ice::GameWorldTrait_Render>() };
    ice::UniquePtr<ice::GameWorldTrait_Render> _trait_render_finish{ ice::make_unique_null<ice::GameWorldTrait_Render>() };
    ice::UniquePtr<ice::WorldTrait> _trait_render_camera{ ice::make_unique_null<ice::WorldTrait>() };

    ice::World* _test_world;

    ice::pod::Array<ice::Entity> _army;
};

ICE_REGISTER_GAMEAPP(MyGame);
