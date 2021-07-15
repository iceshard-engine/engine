#include "game.hxx"

#include <ice/engine.hxx>
#include <ice/engine_runner.hxx>

#include <ice/entity/entity_index.hxx>
#include <ice/entity/entity_storage.hxx>
#include <ice/archetype/archetype_index.hxx>
#include <ice/archetype/archetype_block_allocator.hxx>

#include <ice/engine_devui.hxx>
#include <ice/devui/devui_render_trait.hxx>

#include <ice/world/world.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/world/world_manager.hxx>

#include <ice/gfx/gfx_pass.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_pass.hxx>
#include <ice/render/render_swapchain.hxx>

#include <ice/module_register.hxx>
#include <ice/resource_system.hxx>
#include <ice/resource.hxx>
#include <ice/assert.hxx>

MyGame::MyGame(ice::Allocator& alloc, ice::Clock const& clock) noexcept
    : _allocator{ alloc, "MyGame-alloc" }
    , _clock{ clock }
    , _current_engine{ nullptr }
    , _archetype_alloc{ _allocator }
    , _archetype_index{ ice::create_archetype_index(_allocator) }
    , _entity_storage{ _allocator, *_archetype_index, _archetype_alloc }
    , _game_gfx_pass{ ice::gfx::create_dynamic_pass(_allocator) }
    , _test_world{ nullptr }
    , _anim_timer{ ice::timer::create_timer(_clock, 1.f / 10) }
    , _anim_query{ nullptr }
{
}

void MyGame::on_load_modules(ice::GameServices& sercies) noexcept
{
    ice::ModuleRegister& mod = sercies.module_registry();
    ice::ResourceSystem& res = sercies.resource_system();

    ice::Resource* const pipelines_module = res.request("res://iceshard_pipelines.dll"_uri);
    ice::Resource* const engine_module = res.request("res://iceshard.dll"_uri);
    ice::Resource* const vulkan_module = res.request("res://vulkan_renderer.dll"_uri);
    ice::Resource* const imgui_module = res.request("res://imgui_module.dll"_uri);

    ICE_ASSERT(pipelines_module != nullptr, "Missing `iceshard_pipelines.dll` module!");
    ICE_ASSERT(engine_module != nullptr, "Missing `iceshard.dll` module!");
    ICE_ASSERT(vulkan_module != nullptr, "Missing `vulkan_renderer.dll` module!");

    mod.load_module(_allocator, pipelines_module->location().path);
    mod.load_module(_allocator, engine_module->location().path);
    mod.load_module(_allocator, vulkan_module->location().path);

    if (imgui_module != nullptr)
    {
        mod.load_module(_allocator, imgui_module->location().path);
    }
}

void MyGame::on_app_startup(ice::Engine& engine) noexcept
{
    _current_engine = &engine;
    ICE_LOG(
        ice::LogSeverity::Debug, ice::LogTag::Game,
        "Hello, world!"
    );

    ice::EngineDevUI& devui = engine.developer_ui();

    _trait_render_gfx = ice::create_trait_render_gfx(_allocator);
    _trait_render_clear = ice::create_trait_render_clear(_allocator);
    _trait_render_finish = ice::create_trait_render_finish(_allocator);
    _trait_render_postprocess = ice::create_trait_render_postprocess(_allocator);
    _trait_render_sprites = ice::create_trait_render_sprites(_allocator);
    _trait_render_camera = ice::create_trait_camera(_allocator);


    _game_gfx_pass->add_stages(_trait_render_gfx->gfx_stage_infos());
    _game_gfx_pass->add_stages(_trait_render_clear->gfx_stage_infos());
    _game_gfx_pass->add_stages(_trait_render_sprites->gfx_stage_infos());
    _game_gfx_pass->add_stages(_trait_render_postprocess->gfx_stage_infos());

    if (devui.world_trait() != nullptr)
    {
        ice::gfx::GfxStageInfo devui_stage = devui.world_trait()->gfx_stage_info();

        ice::pod::Array<ice::StringID> dependencies{ _allocator };
        ice::pod::array::push_back(dependencies, devui_stage.dependencies);
        for (ice::gfx::GfxStageInfo const& stage_info : _trait_render_postprocess->gfx_stage_infos())
        {
            ice::pod::array::push_back(dependencies, stage_info.name);
        }

        devui_stage.dependencies = dependencies;
        _game_gfx_pass->add_stage(devui_stage);
    }

    {
        ice::gfx::GfxStageInfo devui_stage = devui.world_trait()->gfx_stage_info();
        for (ice::gfx::GfxStageInfo stage_info : _trait_render_finish->gfx_stage_infos())
        {
            ice::pod::Array<ice::StringID> dependencies{ _allocator };
            ice::pod::array::push_back(dependencies, stage_info.dependencies);
            ice::pod::array::push_back(dependencies, devui_stage.name);

            stage_info.dependencies = dependencies;
            _game_gfx_pass->add_stage(stage_info);
        }
    }


    ice::WorldManager& world_manager = engine.world_manager();
    _test_world = world_manager.create_world("game.test_world"_sid, &_entity_storage);
    _test_world->add_trait("ice.render_gfx"_sid, _trait_render_gfx.get());
    _test_world->add_trait("ice.render_clear"_sid, _trait_render_clear.get());
    _test_world->add_trait("ice.render_finish"_sid, _trait_render_finish.get());
    _test_world->add_trait("ice.camera"_sid, _trait_render_camera.get());
    _test_world->add_trait("ice.render_postprocess"_sid, _trait_render_postprocess.get());
    _test_world->add_trait("ice.render_sprites"_sid, _trait_render_sprites.get());
    _test_world->add_trait("ice.devui"_sid, devui.world_trait());
    _test_world->add_trait("game"_sid, this);

    ice::ArchetypeHandle ortho_arch = _archetype_index->register_archetype<ice::Camera, ice::CameraOrtho>(&_archetype_alloc);
    ice::ArchetypeHandle persp_arch = _archetype_index->register_archetype<ice::Camera, ice::CameraPerspective>(&_archetype_alloc);
    _archetype_index->register_archetype<ice::Transform2DStatic, ice::Sprite>(&_archetype_alloc);
    _archetype_index->register_archetype<ice::Transform2DStatic, ice::Sprite, ice::SpriteTile, AnimComponent>(&_archetype_alloc);

    _anim_query = _allocator.make<ice::ComponentQuery<AnimComponent const&, ice::SpriteTile&>>(_allocator, *_archetype_index);
}

void MyGame::on_app_shutdown(ice::Engine& engine) noexcept
{
    _allocator.destroy(_anim_query);

    _test_world->remove_trait("game"_sid);
    _test_world->remove_trait("ice.devui"_sid);
    _test_world->remove_trait("ice.camera"_sid);
    _test_world->remove_trait("ice.render_sprites"_sid);
    _test_world->remove_trait("ice.render_postprocess"_sid);
    _test_world->remove_trait("ice.render_finish"_sid);
    _test_world->remove_trait("ice.render_clear"_sid);
    _test_world->remove_trait("ice.render_gfx"_sid);

    ice::WorldManager& world_manager = engine.world_manager();
    world_manager.destroy_world("game.test_world"_sid);

    _trait_render_camera = nullptr;
    _trait_render_postprocess = nullptr;
    _trait_render_sprites = nullptr;
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

void MyGame::on_game_begin(ice::EngineRunner& runner) noexcept
{
    ice::vec2u extent = runner.graphics_device().swapchain().extent();

    constexpr ice::StringID components[]{ ice::Camera::Identifier, ice::CameraOrtho::Identifier };
    constexpr ice::ArchetypeQueryCriteria query_criteria{
        .components = components
    };

    constexpr ice::StringID sprite_components[]{ ice::Sprite::Identifier, ice::SpriteTile::Identifier };
    constexpr ice::ArchetypeQueryCriteria sprite_query_criteria{
        .components = sprite_components
    };

    ice::ArchetypeHandle ortho_arch = _archetype_index->find_archetype(query_criteria);
    ice::ArchetypeHandle sprite_arch = _archetype_index->find_archetype(sprite_query_criteria);

    ice::Entity camera_entity = _current_engine->entity_index().create();
    ice::Entity sprite_entity = _current_engine->entity_index().create();
    ice::Entity sprite_entity2 = _current_engine->entity_index().create();
    ice::Entity sprite_entity3 = _current_engine->entity_index().create();

    ice::Camera const camera{
        .name = "camera.default"_sid,
        .position = { 0.f, 0.f, 0.f },
        .front = { 0.f, 0.f, -1.f }
    };
    ice::CameraOrtho const orto_values{
        .left_right = { 0.f, (ice::f32)extent.x },
        .top_bottom = { (ice::f32)extent.y, 0.f },
        .near_far = { 0.1f, 100.f }
    };
    _entity_storage.set_archetype_with_data(camera_entity, ortho_arch, camera, orto_values);

    AnimComponent anim{
        .steps = 18,
    };
    ice::Transform2DStatic sprite_pos{
        .position = { 48.f, 48.f, 1.f },
        .scale = { 3.f, 3.f }
    };
    ice::Sprite sprite{
        .material = "/cotm/cotm_hero"_sid,
    };
    ice::SpriteTile sprite_tile{
        .material_tile = { 0, 0 }
    };
    _entity_storage.set_archetype_with_data(sprite_entity, sprite_arch, anim, sprite_pos, sprite, sprite_tile);

    anim.steps = 4;
    sprite_pos.position = { 120.f, 48.f, 1.f };
    sprite_tile.material_tile = { 0, 1 };
    _entity_storage.set_archetype_with_data(sprite_entity2, sprite_arch, anim, sprite_pos, sprite, sprite_tile);

    anim.steps = 7;
    sprite_pos.position = { 180.f, 48.f, 1.f };
    sprite_tile.material_tile = { 0, 2 };
    _entity_storage.set_archetype_with_data(sprite_entity3, sprite_arch, anim, sprite_pos, sprite, sprite_tile);

    _trait_render_sprites->set_camera("camera.default"_sid);

    ice::EngineRequest requests[]{
        ice::create_request(ice::Request_ActivateWorld, _test_world)
    };
    runner.current_frame().push_requests(requests);
}

void MyGame::on_game_end() noexcept
{
}

void MyGame::on_update(ice::EngineFrame& frame, ice::EngineRunner& runner, ice::WorldPortal& portal) noexcept
{
    auto result = _anim_query->result_by_entity(runner.current_frame().allocator(), _entity_storage);

    if (ice::timer::update_by_step(_anim_timer))
    {
        result.for_each([&](AnimComponent const& anim, ice::SpriteTile& tile) noexcept
            {
                tile.material_tile.x += 1;
                if (anim.steps == tile.material_tile.x)
                {
                    tile.material_tile.x = 0;
                }
            }
        );
    }

    runner.graphics_frame().enqueue_pass("default"_sid, _game_gfx_pass.get());
}
