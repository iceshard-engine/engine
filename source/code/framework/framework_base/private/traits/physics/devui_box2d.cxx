#include "devui_box2d.hxx"

#include <ice/engine_frame.hxx>
#include <ice/game_render_traits.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice
{

    namespace detail
    {

        inline auto as_debug_color(b2Color const& color, ice::f32 replace_a) noexcept -> ice::vec1u
        {
            ice::u32 const r = static_cast<ice::u8>(color.r * 255.f);
            ice::u32 const g = static_cast<ice::u8>(color.g * 255.f) << 8;
            ice::u32 const b = static_cast<ice::u8>(color.b * 255.f) << 16;
            ice::u32 const a = static_cast<ice::u8>(replace_a * 255.f) << 24;
            return ice::vec1u{ r | g | b | a };
        }

        inline auto as_debug_color(b2Color const& color) noexcept -> ice::vec1u
        {
            return as_debug_color(color, color.a);
        }

        class DevUI_DebugDrawCommandRecorder : public b2Draw
        {
        public:
            DevUI_DebugDrawCommandRecorder(ice::u32 flags) noexcept
            {
                SetFlags(flags);
            }

            void SetLists(
                ice::DebugDrawCommand* command_list,
                ice::vec3f* vertex_list,
                ice::vec1u* color_list
            ) noexcept
            {
                draw_commands = 0;
                draw_vertex_count = 0;

                _vertex_list = vertex_list;
                _color_list = color_list;
                _command_list = command_list;
                _polygon_draw_command = nullptr;
                _transform_draw_command = nullptr;
            }

            /// Draw a closed polygon provided in CCW order.
            void DrawPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) noexcept override;

            /// Draw a solid closed polygon provided in CCW order.
            void DrawSolidPolygon(const b2Vec2* vertices, int32 vertexCount, const b2Color& color) noexcept override;

            /// Draw a circle.
            void DrawCircle(const b2Vec2& center, float radius, const b2Color& color) noexcept override;

            /// Draw a solid circle.
            void DrawSolidCircle(const b2Vec2& center, float radius, const b2Vec2& axis, const b2Color& color) noexcept override;

            /// Draw a line segment.
            void DrawSegment(const b2Vec2& p1, const b2Vec2& p2, const b2Color& color) override { }

            /// Draw a transform. Choose your own length scale.
            /// @param xf a transform.
            void DrawTransform(const b2Transform& xf) override;

            /// Draw a point.
            void DrawPoint(const b2Vec2& p, float size, const b2Color& color) override;

            ice::u32 draw_commands = 0;
            ice::u32 draw_vertex_count = 0;

        private:
            ice::vec3f* _vertex_list = nullptr;
            ice::vec1u* _color_list = nullptr;

            ice::DebugDrawCommand* _command_list = nullptr;
            ice::DebugDrawCommand* _polygon_draw_command = nullptr;
            ice::DebugDrawCommand* _transform_draw_command = nullptr;

            ice::u32 available_polygon_commands = 1;
            ice::u32 available_transform_commands = 1;
        };
    }

    DevUI_Box2D::DevUI_Box2D(b2World& box2d_world) noexcept
        : _world{ box2d_world }
        , _debug_draw_flags{ 0 }
        , _visible{ true }
    {
    }

    void DevUI_Box2D::on_prepare(void* context) noexcept
    {
        ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(context));
    }

    void DevUI_Box2D::on_draw() noexcept
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("Debug Windows"))
            {
                ImGui::MenuItem("Physics2D", nullptr, &_visible);
                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();

        if (_visible)
        {
            if (ImGui::Begin("Physics2D (Box2D)", &_visible))
            {
                ImGui::CheckboxFlags("Draw Shapes", &_debug_draw_flags, b2Draw::e_shapeBit);
                ImGui::CheckboxFlags("Draw Bounding Boxes", &_debug_draw_flags, b2Draw::e_aabbBit);
                ImGui::CheckboxFlags("Draw Transforms", &_debug_draw_flags, b2Draw::e_centerOfMassBit);
            }
            ImGui::End();
        }
    }

    void DevUI_Box2D::on_frame(ice::EngineFrame& frame) noexcept
    {
        if (_visible && _debug_draw_flags != 0)
        {
            detail::DevUI_DebugDrawCommandRecorder recorder{ _debug_draw_flags };

            _world.SetDebugDraw(&recorder);
            _world.DebugDraw();

            if (recorder.draw_commands == 0)
            {
                _world.SetDebugDraw(nullptr);
                return;
            }

            ice::DebugDrawCommand* const commands = (ice::DebugDrawCommand*) frame.allocate_named_data(
                "physics2d.debug-draw.commands"_sid,
                recorder.draw_commands * sizeof(ice::DebugDrawCommand),
                alignof(ice::DebugDrawCommand)
            );

            ice::vec3f* const vertex_list = (ice::vec3f*)frame.allocate_named_data(
                "physics2d.debug-draw.vertex-list"_sid,
                recorder.draw_vertex_count * sizeof(ice::vec3f),
                alignof(ice::vec3f)
            );

            ice::vec1u* const color_list = (ice::vec1u*)frame.allocate_named_data(
                "physics2d.debug-draw.color-list"_sid,
                recorder.draw_vertex_count * sizeof(ice::vec1u),
                alignof(ice::vec1u)
            );

            recorder.SetLists(commands, vertex_list, color_list);

            _world.DebugDraw();
            _world.SetDebugDraw(nullptr);

            ice::DebugDrawCommandList const* const command_list = frame.create_named_object<ice::DebugDrawCommandList>(
                "physics2d.debug-draw.command-list"_sid,
                recorder.draw_commands,
                commands
            );

            ice::shards::push_back(
                frame.shards(),
                ice::Shard_DebugDrawCommand | command_list
            );
        }
    }

    /// Draw a closed polygon provided in CCW order.

    void detail::DevUI_DebugDrawCommandRecorder::DrawPolygon(b2Vec2 const* vertices, int32 vertex_count, b2Color const& color) noexcept
    {
        if (_command_list != nullptr)
        {
            ice::vec3f* vertex = _vertex_list + draw_vertex_count;
            ice::vec1u* vertex_color = _color_list + draw_vertex_count;

            *vertex = ice::vec3f{ vertices[vertex_count - 1].x, vertices[vertex_count - 1].y, 0.f } *Constant_PixelsInMeter;
            *vertex_color = as_debug_color(color, 0.f);

            vertex += 1;
            vertex_color += 1;

            for (ice::u32 idx = 0; idx < vertex_count; ++idx)
            {
                *vertex = ice::vec3f{ vertices[idx].x, vertices[idx].y, color.r } *Constant_PixelsInMeter;
                *vertex_color = as_debug_color(color);

                vertex += 1;
                vertex_color += 1;
            }

            *vertex = ice::vec3f{ vertices[0].x, vertices[0].y, 0.f } *Constant_PixelsInMeter;
            *vertex_color = as_debug_color(color, 0.f);

            vertex += 1;
            vertex_color += 1;

            if (_polygon_draw_command == nullptr)
            {
                _polygon_draw_command = _command_list + draw_commands;
                *_polygon_draw_command = DebugDrawCommand{
                    .vertex_count = static_cast<ice::u32>(vertex_count) + 2,
                    .vertex_list = _vertex_list + draw_vertex_count,
                    .vertex_color_list = _color_list + draw_vertex_count,
                };

                draw_commands += 1;
            }
            else
            {
                _polygon_draw_command->vertex_count += vertex_count + 2;
            }
        }
        else
        {
            draw_commands += available_polygon_commands;
            available_polygon_commands = 0;
        }

        //draw_commands += 1;
        draw_vertex_count += vertex_count + 2;
    }

    /// Draw a solid closed polygon provided in CCW order.

    void detail::DevUI_DebugDrawCommandRecorder::DrawSolidPolygon(b2Vec2 const* vertices, int32 vertex_count, b2Color const& color) noexcept
    {
        DrawPolygon(vertices, vertex_count, color);
    }

    void detail::DevUI_DebugDrawCommandRecorder::DrawCircle(b2Vec2 const& center, float radius, b2Color const& color) noexcept
    {
        ice::rad constexpr angle = ice::math::radians(ice::deg{ 15.f });
        ice::u32 constexpr vertex_count = (360.f / 15.f);

        ice::mat2x2 const rotation_mtx{
            .v = {
                { ice::math::cos(angle), ice::math::sin(angle) },
                { -ice::math::sin(angle), ice::math::cos(angle) }
            }
        };

        b2Vec2 vertices[vertex_count]{ };

        ice::vec2f point = { 0.f, radius };
        for (ice::u32 idx = 0; idx < vertex_count; ++idx)
        {
            vertices[idx].x = point.x + center.x;
            vertices[idx].y = point.y + center.y;
            point = rotation_mtx * point;
        }

        DrawPolygon(vertices, vertex_count, color);
    }

    void detail::DevUI_DebugDrawCommandRecorder::DrawSolidCircle(b2Vec2 const& center, float radius, b2Vec2 const& axis, b2Color const& color) noexcept
    {
        DrawCircle(center, radius, color);
    }

    void detail::DevUI_DebugDrawCommandRecorder::DrawTransform(b2Transform const& xf)
    {
        if (_command_list != nullptr)
        {
            ice::vec3f* vertex = _vertex_list + draw_vertex_count;
            ice::vec1u* vertex_color = _color_list + draw_vertex_count;

            ice::mat2x2 const rot{
                .v = {
                    { xf.q.c, xf.q.s },
                    { -xf.q.s, xf.q.c }
                }
            };

            ice::vec2f const p0 = ice::vec2f{ xf.p.x, xf.p.y } *Constant_PixelsInMeter;
            ice::vec2f const py = ((rot * ice::vec2f{ 0.f, 0.25f }) + ice::vec2f{ xf.p.x, xf.p.y }) * Constant_PixelsInMeter;
            ice::vec2f const px = ((rot * ice::vec2f{ 0.25f, 0.f }) + ice::vec2f{ xf.p.x, xf.p.y }) * Constant_PixelsInMeter;

            ice::vec2f const points[]{ p0, py, p0, px, p0, p0 };
            ice::vec1u const colors[]{
                as_debug_color({ 0.f, 0.f, 0.f, 0.f }),
                as_debug_color({ 0.f, 1.f, 0.f, 1.f }),
                as_debug_color({ 0.f, 1.f, 0.f, 1.f }),
                as_debug_color({ 1.f, 0.f, 0.f, 1.f }),
                as_debug_color({ 1.f, 0.f, 0.f, 1.f }),
                as_debug_color({ 0.f, 0.f, 0.f, 0.f }),
            };

            for (ice::u32 idx = 0; idx < ice::size(points); ++idx)
            {
                *vertex = ice::vec3f{ points[idx].x, points[idx].y, 0.0f };
                *vertex_color = colors[idx];

                vertex += 1;
                vertex_color += 1;
            }

            if (_transform_draw_command == nullptr)
            {
                _transform_draw_command = _command_list + draw_commands;
                *_transform_draw_command = DebugDrawCommand{
                    .vertex_count = 6,
                    .vertex_list = _vertex_list + draw_vertex_count,
                    .vertex_color_list = _color_list + draw_vertex_count,
                };

                draw_commands += 1;
            }
            else
            {
                _transform_draw_command->vertex_count += 6;
            }
        }
        else
        {
            draw_commands += available_transform_commands;
            available_polygon_commands = 0;
        }

        //draw_commands += 1;
        draw_vertex_count += 6;
    }

    void detail::DevUI_DebugDrawCommandRecorder::DrawPoint(b2Vec2 const& p, float size, b2Color const& color)
    {
        if (_command_list != nullptr)
        {
            ice::vec3f* vertex = _vertex_list + draw_vertex_count;
            ice::vec1u* vertex_color = _color_list + draw_vertex_count;

            *vertex = ice::vec3f{ p.x, p.y - 0.1f, 1.0f } * Constant_PixelsInMeter;
            *vertex_color = as_debug_color(color);

            vertex += 1;
            vertex_color += 1;

            *vertex = ice::vec3f{ p.x, p.y + 0.1f, 1.0f } * Constant_PixelsInMeter;
            *vertex_color = as_debug_color(color);

            _command_list[draw_commands] = DebugDrawCommand{
                .vertex_count = 2,
                .vertex_list = _vertex_list + draw_vertex_count,
                .vertex_color_list = _color_list + draw_vertex_count,
            };
        }

        draw_commands += 1;
        draw_vertex_count += 2;
    }

} // namespace ice
