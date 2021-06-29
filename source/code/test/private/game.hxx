#pragma once
#include <ice/game_framework.hxx>
#include <ice/game_render_traits.hxx>
#include <ice/memory/proxy_allocator.hxx>

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>

#include <ice/entity/entity_storage.hxx>
#include <ice/archetype/archetype_index.hxx>
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


        _game_gfx_pass->add_stages(_trait_render_gfx->gfx_stage_infos());
        _game_gfx_pass->add_stages(_trait_render_clear->gfx_stage_infos());
        //_game_gfx_pass->add_stage("frame.draw"_sid, { _trait_render_clear->gfx_stage_name() });
        //_game_gfx_pass->add_stage("frame.postprocess"_sid, { "frame.draw"_sid });
        //_game_gfx_pass->add_stage("frame.present"_sid, { "frame.postprocess"_sid });


        ice::WorldManager& world_manager = engine.world_manager();
        _test_world = world_manager.create_world("game.test_world"_sid, &_entity_storage);
        _test_world->add_trait("ice.render_gfx"_sid, _trait_render_gfx.get());
        _test_world->add_trait("ice.render_clear"_sid, _trait_render_clear.get());
        _test_world->add_trait("game"_sid, this);

        //add_world_traits(engine, world);

        //_archetype_index->register_archetype<ice::Obj2dTransform>(&_archetype_alloc);
        //auto tilemap_arch = _archetype_index->register_archetype<ice::TileMapComponent>(&_archetype_alloc);



        //auto a = _archetype_index->register_archetype<ice::Obj2dShape, ice::Obj2dTransform>(&_archetype_alloc);
        //auto a2 = _archetype_index->register_archetype<ice::Obj2dShape, ice::Obj2dTransform, ice::Obj2dMaterial>(&_archetype_alloc);

        //ice::EntityIndex& idx = engine.entity_index();

        //_entity_storage.set_archetype(idx.create(), a);
        //_entity_storage.set_archetype(idx.create(), a);
        //_entity_storage.set_archetype(idx.create(), a2);
        //_entity_storage.set_archetype(idx.create(), a2);
        //_entity_storage.set_archetype(idx.create(), a);
        //_entity_storage.set_archetype(idx.create(), tilemap_arch);


        //using ObjectQuery = ice::ComponentQuery<ice::Obj2dShape&, ice::Obj2dTransform&>;
        //ObjectQuery obj_query{ _allocator, *_archetype_index };
        //auto obj_result = obj_query.result_by_entity(_allocator, _entity_storage);

        //ice::rad angle{ 0 };
        //ice::vec3f initial_pos{ 32.f, 32.f, 1.f };
        //obj_result.for_each([&](ice::Obj2dShape& shape, ice::Obj2dTransform& xform)
        //    {
        //        shape.shape_definition = nullptr;
        //        xform.position = initial_pos;
        //        xform.rotation = angle.value;
        //        initial_pos.x += 32.f;
        //        initial_pos.y += 32.f;
        //        initial_pos.z += 1.f;
        //        angle.value += 0.1;
        //    });

        //ice::ComponentQuery<ice::TileMapComponent&> query{ _allocator, *_archetype_index };
        //auto result = query.result_by_entity(_allocator, _entity_storage);

        //result.for_each([&](ice::TileMapComponent& tilemap)
        //    {
        //        tilemap.name = "test"_sid;
        //    });
    }

    void on_app_shutdown(ice::Engine& engine) noexcept
    {
        _test_world->remove_trait("game"_sid);
        _test_world->remove_trait("ice.render_clear"_sid);
        _test_world->remove_trait("ice.render_gfx"_sid);

        ice::WorldManager& world_manager = engine.world_manager();
        world_manager.destroy_world("game.test_world"_sid);
        //remove_world_traits(world_manager.find_world("default"_sid));

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

    ice::World* _test_world;
};

ICE_REGISTER_GAMEAPP(MyGame);
