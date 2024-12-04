#include "box2d_trait.hxx"

#include <ice/assert.hxx>
#include <ice/devui_imgui.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/engine_shards.hxx>
#include <ice/engine_runner.hxx>
#include <ice/clock.hxx>

namespace ice
{

    static constexpr ice::ShardID ShardID_ClickAction = "ice/input-action/A:click`ice::InputAction const*"_shardid;

    namespace detail::box2d
    {

        static ice::HostAllocator global_alloc;

        auto allocator_fn(unsigned int size, int align) noexcept -> void*
        {
            return global_alloc.allocate({ ice::usize{ size }, ice::ualign(align) }).memory;
        }

        void free_fn(void* data) noexcept
        {
            global_alloc.deallocate(data);
        }

        auto assert_fn(char const* condition, char const* filename, int linenumber) noexcept -> ice::i32
        {
            ice::detail::assert(condition, "Box2D assertion failed!", {}, {.file=filename,.line=ice::u32(linenumber)});
            return 1;
        }

        auto enqueue_task_callback(
            b2TaskCallback* task,
            int32_t item_count,
            int32_t min_range,
            void* task_context,
            void* user_context
        ) noexcept -> void*
        {
            task(0, item_count, 0, task_context);
            return nullptr;
        }

        void finish_task_callback(void* userTask, void* userContext) noexcept
        {
        }

    } // namespace detail::box2d

    Box2DTrait::Box2DTrait(ice::TraitContext& context, ice::Allocator& alloc) noexcept
        : ice::Trait{ context }
        , ice::TraitDevUI{ {.category="Engine/Traits", .name = trait_name()} }
    {
        b2SetAssertFcn(ice::detail::box2d::assert_fn);
        b2SetAllocator(
            ice::detail::box2d::allocator_fn,
            ice::detail::box2d::free_fn
        );

        context.bind<&Box2DTrait::on_update>();
        context.bind<&Box2DTrait::on_click>(ice::ShardID_ClickAction);
    }

    auto Box2DTrait::on_click(ice::InputAction const& action) noexcept -> ice::Task<>
    {
        ice::Tns const timeactive = ice::clock::elapsed(action.timestamp, ice::clock::now());
        ICE_LOG(LogSeverity::Info, LogTag::Engine, "{:.2s} Click | {}x{}", timeactive, action.value.x, action.value.y);

        b2BodyDef body = b2DefaultBodyDef();
        body.type = b2_dynamicBody;
        body.position.x = action.value.x;
        body.position.y = action.value.y;
        b2BodyId bodyid = b2CreateBody(_worldid, &body);
        b2Body_SetMassData(bodyid, {.mass=200.f});
        b2ShapeDef shape = b2DefaultShapeDef();
        b2Polygon poly = b2MakeBox(0.5f, 0.5f);
        b2ShapeId shapeid = b2CreatePolygonShape(bodyid, &shape, &poly);
        co_return;
    }

    auto Box2DTrait::activate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<>
    {
        b2WorldDef world_definition = b2DefaultWorldDef();
        world_definition.workerCount = 1;
        world_definition.userTaskContext = nullptr;
        world_definition.enqueueTask = ice::detail::box2d::enqueue_task_callback;
        world_definition.finishTask = nullptr;
        world_definition.gravity.y = 10.0f;

        _worldid = b2CreateWorld(&world_definition);

        b2BodyDef body = b2DefaultBodyDef();
        body.type = b2_dynamicBody;
        body.position.x = 50;
        body.position.y = 50;
        b2BodyId bodyid = b2CreateBody(_worldid, &body);
        b2Body_SetMassData(bodyid, {.mass=4.f});
        b2ShapeDef shape = b2DefaultShapeDef();
        b2Polygon poly = b2MakeBox(0.5f, 0.5f);
        b2ShapeId shapeid = b2CreatePolygonShape(bodyid, &shape, &poly);
        co_return;
    }

    auto Box2DTrait::on_update(ice::EngineFrameUpdate const& update) noexcept -> ice::Task<>
    {
        b2World_Step(_worldid, 1.0f/60.f, 4);
        co_return;
    }

    auto Box2DTrait::deactivate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<>
    {
        b2DestroyWorld(_worldid);
        co_return;
    }

    void Box2DTrait::build_content() noexcept
    {
        ImGui::TextT("Allocated bytes: {:p}", ice::usize{ice::u32(b2GetByteCount())});

        b2DebugDraw debug_draw = b2DefaultDebugDraw();
        debug_draw.drawShapes = true;
        debug_draw.DrawPolygon = [](b2Vec2 const* vertices, int vertex_count, b2HexColor color, void* context) noexcept
        {
            static ImVec2 im_vertices[b2_maxPolygonVertices];
            for (ice::i32 idx = 0; idx < vertex_count; ++idx)
            {
                im_vertices[idx].x = vertices[idx].x;
                im_vertices[idx].y = vertices[idx].y;
            }

            ImDrawList* background = ImGui::GetForegroundDrawList();
            background->AddPolyline(im_vertices, vertex_count, color, ImDrawFlags_Closed, 1.0f);
        };
        debug_draw.DrawSolidPolygon = [](b2Transform xform, b2Vec2 const* vertices, int vertex_count, float radius, b2HexColor color, void* context) noexcept
        {
            static ImVec2 im_vertices[b2_maxPolygonVertices];
            for (ice::i32 idx = 0; idx < vertex_count; ++idx)
            {
                im_vertices[idx].x = vertices[idx].x * 10.0f + xform.p.x;
                im_vertices[idx].y = vertices[idx].y * 10.0f + xform.p.y;
            }

            ImDrawList* background = ImGui::GetBackgroundDrawList();
            background->AddPolyline(im_vertices, vertex_count, ImGui::ToColor(0xFFFF6666_argb), ImDrawFlags_Closed, 1.0f);
        };
        b2World_Draw(_worldid, &debug_draw);
    }

} // namespace ice
