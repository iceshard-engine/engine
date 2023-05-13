/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "devui_chipmunk2d.hxx"

#include <ice/engine_frame.hxx>
#include <ice/game_render_traits.hxx>

#include <imgui/imgui.h>
#undef assert

namespace ice
{

    namespace detail
    {

        inline auto as_debug_color(ice::vec4f const& color, ice::f32 replace_a) noexcept -> ice::vec1u
        {
            ice::u32 const r = static_cast<ice::u8>(color.x * 255.f);
            ice::u32 const g = static_cast<ice::u8>(color.y * 255.f) << 8;
            ice::u32 const b = static_cast<ice::u8>(color.z * 255.f) << 16;
            ice::u32 const a = static_cast<ice::u8>(replace_a * 255.f) << 24;
            return ice::vec1u{ r | g | b | a };
        }

        inline auto as_debug_color(ice::vec4f const& color) noexcept -> ice::vec1u
        {
            return as_debug_color(color, color.w);
        }

        class DevUI_DebugDrawCommandRecorder
        {
        public:
            DevUI_DebugDrawCommandRecorder(ice::u32 flags) noexcept
                : _flags{ flags }
            {
            }

            static void on_draw_shape(cpBody* body, cpShape* shape, void* userdata)
            {
                DevUI_DebugDrawCommandRecorder* recorder = (DevUI_DebugDrawCommandRecorder*)userdata;

                if (shape->klass->type == cpShapeType::CP_POLY_SHAPE)
                {
                    recorder->draw_polygon(shape);
                }
                else if (shape->klass->type == cpShapeType::CP_CIRCLE_SHAPE)
                {
                    recorder->draw_circle(shape);
                }
            }

            static void on_draw(cpBody* body, void* userdata) noexcept
            {
                cpBodyEachShape(body, on_draw_shape, userdata);
            }

            void set_lists(
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
            void draw_polygon(cpShape* shape) noexcept;

            /// Draw a solid closed polygon provided in CCW order.
            void draw_solid_polygon(cpShape* shape) noexcept;

            /// Draw a circle.
            void draw_circle(cpShape* shape) noexcept;

            /// Draw a solid circle.
            void draw_solid_circle(cpShape* shape) noexcept;

            /// Draw a line segment.
            void draw_segment(cpShape* shape) { }

            /// Draw a transform. Choose your own length scale.
            /// @param xf a transform.
            void draw_transform(const cpTransform& xf);

            /// Draw a point.
            void draw_point(cpShape* shape);

            ice::u32 draw_commands = 0;
            ice::u32 draw_vertex_count = 0;

        private:
            ice::u32 _flags;

            ice::vec3f* _vertex_list = nullptr;
            ice::vec1u* _color_list = nullptr;

            ice::DebugDrawCommand* _command_list = nullptr;
            ice::DebugDrawCommand* _polygon_draw_command = nullptr;
            ice::DebugDrawCommand* _transform_draw_command = nullptr;

            ice::u32 available_polygon_commands = 1;
            ice::u32 available_transform_commands = 1;
        };
    }

    DevUI_Chipmunk2D::DevUI_Chipmunk2D(cpSpace& cp_space) noexcept
        : _space{ cp_space }
        , _state{ nullptr }
        , _debug_draw_flags{ 0 }
    {
    }

    auto DevUI_Chipmunk2D::settings() const noexcept -> ice::devui::WidgetSettings const&
    {
        static devui::WidgetSettings settings{
            .menu_text = "Box2D (DebugDraw)",
            .menu_category = "Tools",
        };
        return settings;
    }

    void DevUI_Chipmunk2D::on_prepare(void* context, ice::devui::WidgetState& state) noexcept
    {
        ImGui::SetCurrentContext(reinterpret_cast<ImGuiContext*>(context));
        _state = &state;
    }

    void DevUI_Chipmunk2D::on_draw() noexcept
    {
        if (ImGui::Begin("Box2D (DebugDraw)", &_state->is_visible))
        {
            ImGui::CheckboxFlags("Draw Shapes", &_debug_draw_flags, 0x01);
            ImGui::CheckboxFlags("Draw Bounding Boxes", &_debug_draw_flags, 0x02);
            ImGui::CheckboxFlags("Draw Transforms", &_debug_draw_flags, 0x04);
        }
        ImGui::End();
    }

    void DevUI_Chipmunk2D::on_frame(ice::EngineFrame& frame) noexcept
    {
        if (_state && _state->is_visible && _debug_draw_flags != 0)
        {
            detail::DevUI_DebugDrawCommandRecorder recorder{ _debug_draw_flags };

            cpSpaceEachBody(&_space, detail::DevUI_DebugDrawCommandRecorder::on_draw, &recorder);

            if (recorder.draw_commands == 0)
            {
                return;
            }

            ice::DebugDrawCommand* const commands = ice::span::data(
                frame.storage().create_named_span<ice::DebugDrawCommand>(
                    "physics2d.debug-draw.commands"_sid,
                    recorder.draw_commands
                )
            );

            ice::vec3f* const vertex_list = ice::span::data(
                frame.storage().create_named_span<ice::vec3f>(
                    "physics2d.debug-draw.vertex-list"_sid,
                    recorder.draw_vertex_count
                )
            );

            ice::vec1u* const color_list = ice::span::data(
                frame.storage().create_named_span<ice::vec1u>(
                    "physics2d.debug-draw.color-list"_sid,
                    recorder.draw_vertex_count
                )
            );

            recorder.set_lists(commands, vertex_list, color_list);

            cpSpaceEachBody(&_space, detail::DevUI_DebugDrawCommandRecorder::on_draw, &recorder);

            ice::DebugDrawCommandList const* const command_list = frame.storage().create_named_object<ice::DebugDrawCommandList>(
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

    void detail::DevUI_DebugDrawCommandRecorder::draw_polygon(cpShape* shape) noexcept
    {
        ice::vec4f constexpr color{ 0.9f, 0.2f, 0.2f, 1.0f };

        if (_command_list != nullptr)
        {
            ice::vec3f* vertex = _vertex_list + draw_vertex_count;
            ice::vec1u* vertex_color = _color_list + draw_vertex_count;

            ice::i32 const vertex_count = cpPolyShapeGetCount(shape);
            cpVect vert = cpPolyShapeGetVert(shape, vertex_count - 1);

            *vertex = ice::vec3f{ (f32)vert.x, (f32)vert.y, 0.f } *Constant_PixelsInMeter;
            *vertex_color = as_debug_color(color, 0.f);

            vertex += 1;
            vertex_color += 1;

            for (ice::i32 idx = 0; idx < vertex_count; ++idx)
            {
                vert = cpPolyShapeGetVert(shape, idx);
                *vertex = ice::vec3f{ (f32)vert.x, (f32)vert.y, color.w } * Constant_PixelsInMeter;
                *vertex_color = as_debug_color(color);

                vertex += 1;
                vertex_color += 1;
            }

            vert = cpPolyShapeGetVert(shape, 0);
            *vertex = ice::vec3f{ (f32)vert.x,(f32)vert.y, 0.f } * Constant_PixelsInMeter;
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
        draw_vertex_count += cpPolyShapeGetCount(shape) + 2;
    }

    /// Draw a solid closed polygon provided in CCW order.

    void detail::DevUI_DebugDrawCommandRecorder::draw_solid_polygon(cpShape* shape) noexcept
    {
        draw_polygon(shape);
    }

    void detail::DevUI_DebugDrawCommandRecorder::draw_circle(cpShape* shape) noexcept
    {
        ice::f32 const radius = (f32)cpCircleShapeGetRadius(shape);
        cpVect const center = cpCircleShapeGetOffset(shape);

        ice::rad constexpr angle = ice::math::radians(ice::deg{ 15.f });
        ice::u32 constexpr vertex_count = ice::u32(360.f / 15.f);

        ice::mat2x2 const rotation_mtx{
            .v = {
                { ice::math::cos(angle), ice::math::sin(angle) },
                { -ice::math::sin(angle), ice::math::cos(angle) }
            }
        };

        cpVect vertices[vertex_count]{ };

        ice::vec2f point = { 0.f, radius };
        for (ice::u32 idx = 0; idx < vertex_count; ++idx)
        {
            vertices[idx].x = point.x + center.x;
            vertices[idx].y = point.y + center.y;
            point = rotation_mtx * point;
        }

        //draw_polygon(vertices, vertex_count, color);
    }

    void detail::DevUI_DebugDrawCommandRecorder::draw_solid_circle(cpShape* shape) noexcept
    {
        draw_circle(shape);
    }

    void detail::DevUI_DebugDrawCommandRecorder::draw_transform(cpTransform const& xf)
    {
        //if (_command_list != nullptr)
        //{
        //    ice::vec3f* vertex = _vertex_list + draw_vertex_count;
        //    ice::vec1u* vertex_color = _color_list + draw_vertex_count;

        //    ice::mat2x2 const rot{
        //        .v = {
        //            { xf.q.c, xf.q.s },
        //            { -xf.q.s, xf.q.c }
        //        }
        //    };

        //    ice::vec2f const p0 = ice::vec2f{ xf.p.x, xf.p.y } * Constant_PixelsInMeter;
        //    ice::vec2f const py = ((rot * ice::vec2f{ 0.f, 0.25f }) + ice::vec2f{ xf.p.x, xf.p.y }) * Constant_PixelsInMeter;
        //    ice::vec2f const px = ((rot * ice::vec2f{ 0.25f, 0.f }) + ice::vec2f{ xf.p.x, xf.p.y }) * Constant_PixelsInMeter;

        //    ice::vec2f const points[]{ p0, py, p0, px, p0, p0 };
        //    ice::vec1u const colors[]{
        //        as_debug_color({ 0.f, 0.f, 0.f, 0.f }),
        //        as_debug_color({ 0.f, 1.f, 0.f, 1.f }),
        //        as_debug_color({ 0.f, 1.f, 0.f, 1.f }),
        //        as_debug_color({ 1.f, 0.f, 0.f, 1.f }),
        //        as_debug_color({ 1.f, 0.f, 0.f, 1.f }),
        //        as_debug_color({ 0.f, 0.f, 0.f, 0.f }),
        //    };

        //    for (ice::u32 idx = 0; idx < ice::count(points); ++idx)
        //    {
        //        *vertex = ice::vec3f{ points[idx].x, points[idx].y, 0.0f };
        //        *vertex_color = colors[idx];

        //        vertex += 1;
        //        vertex_color += 1;
        //    }

        //    if (_transform_draw_command == nullptr)
        //    {
        //        _transform_draw_command = _command_list + draw_commands;
        //        *_transform_draw_command = DebugDrawCommand{
        //            .vertex_count = 6,
        //            .vertex_list = _vertex_list + draw_vertex_count,
        //            .vertex_color_list = _color_list + draw_vertex_count,
        //        };

        //        draw_commands += 1;
        //    }
        //    else
        //    {
        //        _transform_draw_command->vertex_count += 6;
        //    }
        //}
        //else
        //{
        //    draw_commands += available_transform_commands;
        //    available_polygon_commands = 0;
        //}

        ////draw_commands += 1;
        //draw_vertex_count += 6;
    }

    void detail::DevUI_DebugDrawCommandRecorder::draw_point(cpShape* shape)
    {
        ice::vec4f constexpr color{ 0.9f, 0.3f, 0.2f, 1.0f };

        cpVect const p = cpBodyGetPosition(cpShapeGetBody(shape));

        if (_command_list != nullptr)
        {
            ice::vec3f* vertex = _vertex_list + draw_vertex_count;
            ice::vec1u* vertex_color = _color_list + draw_vertex_count;

            *vertex = ice::vec3f{ (f32)p.x, (f32)p.y - 0.1f, 1.0f } * Constant_PixelsInMeter;
            *vertex_color = as_debug_color(color);

            vertex += 1;
            vertex_color += 1;

            *vertex = ice::vec3f{ (f32) p.x, (f32)p.y + 0.1f, 1.0f } * Constant_PixelsInMeter;
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
