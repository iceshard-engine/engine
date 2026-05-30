/// Copyright 2026 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "devui_colorpicker_oklch.hxx"
#include "devui_widget_utils.hxx"
#include <imgui/imgui_internal.h>

namespace ice
{

    struct DrawWheelParams
    {
        ice::u32 draw_style;
        ice::u32 draw_segments = 360;
        ice::f32 draw_radius;
        ice::f32 draw_center_offset_radius;
        ice::vec2f draw_center;

        ImVec2 uv_white;
    };

    inline constexpr auto oklch_maxchroma(ice::Color color) noexcept -> ice::f32
    {
        ice::f32 const safe_chroma = ice::max(color.chroma, ice::f32_eps * 100);
        ice::detail::OkLCH_HueCusp const cusp = ice::detail::find_cusp_ch(safe_chroma, ice::radians(color.hue));
        return safe_chroma * ice::detail::find_gamut_intersection(cusp, color.lightness, safe_chroma, color.lightness);
    }

    inline void drawwheel_points(
        ice::DrawWheelParams const& params,
        ice::deg angle,
        ice::f32 override_inner_radius,
        ice::vec2f& out_inner,
        ice::vec2f& out_outer
    ) noexcept
    {
        ice::mat2x2 const rotation = ice::rotate2d(ice::radians(angle));

        out_outer.y = 0.0f;
        out_outer.x = params.draw_radius;

        out_inner.y = 0;
        switch (params.draw_style)
        {
        case 1:
            out_inner.x = override_inner_radius;
            break;
        case 0:
        default:
            out_inner.x = params.draw_center_offset_radius;
            break;
        }

        out_outer = rotation * out_outer;
        out_inner = rotation * out_inner;
    }

    static inline void colorpicker_oklch_draw_wheel(
        ImDrawList* draw,
        ice::Color draw_color,
        ice::DrawWheelParams const& params,
        ice::f32 max_chroma,
        bool is_active
    ) noexcept
    {
        using ImGui::Vec2;
        using ImGui::SrgbU32;

        ice::u32 const max_indices = params.draw_segments * 6;
        ice::u32 const max_vertices = params.draw_segments * 2;
        draw->PrimReserve(max_indices, max_vertices);

        // Prepare the color variables
        ice::Color wheel_color = draw_color.with_hue(0.0_deg);
        ImU32 imcolor = SrgbU32(wheel_color);

        ice::f32 chroma_max_radius = params.draw_center_offset_radius;
        if (params.draw_style == 1)
        {
            ice::f32 const chroma_max = ice::oklch_maxchroma(wheel_color);
            chroma_max_radius = params.draw_radius - (chroma_max / 0.32f) * (params.draw_radius - params.draw_center_offset_radius);
        }

        ice::u32 vertices = 2;
        ice::u32 indices = 0;

        ImDrawIdx const first_idx = (ImDrawIdx)draw->_VtxCurrentIdx;
        ImDrawIdx idx = first_idx;

        ice::vec2f ip, op;
        ice::drawwheel_points(params, wheel_color.hue, chroma_max_radius, op, ip);
        draw->PrimWriteVtx(Vec2(params.draw_center + ip), params.uv_white, imcolor);
        draw->PrimWriteVtx(Vec2(params.draw_center + op), params.uv_white, imcolor);

        ice::f32 const step = 360.f / ice::f32(params.draw_segments);
        for (ice::deg angle = { step }; angle < 360.0f; angle = angle + step)
        {
            // Update the color hue
            wheel_color.hue = angle;
            imcolor = SrgbU32(wheel_color);

            // Recalculate the offset for the chroma max representation.
            if (params.draw_style == 1)
            {
                ice::f32 const chroma_max = ice::oklch_maxchroma(wheel_color);
                chroma_max_radius = params.draw_radius - (chroma_max / 0.32f) * (params.draw_radius - params.draw_center_offset_radius);
            }

            // Store the new vertices
            ice::drawwheel_points(params, wheel_color.hue, chroma_max_radius, op, ip);
            draw->PrimWriteVtx(Vec2(params.draw_center + ip), params.uv_white, imcolor);
            draw->PrimWriteVtx(Vec2(params.draw_center + op), params.uv_white, imcolor);

            // Add the triangle indices
            draw->PrimWriteIdx(idx + 0);
            draw->PrimWriteIdx(idx + 1);
            draw->PrimWriteIdx(idx + 3);
            draw->PrimWriteIdx(idx + 0);
            draw->PrimWriteIdx(idx + 3);
            draw->PrimWriteIdx(idx + 2);

            // Because we always work on 4 vertices at once we increase the index along with the number of added vertices.
            // This way indexes 0,1 point to the previous vertices and 2,3 to the next vertices.
            idx += 2;

            // Move both the vertex and index tracker by two.
            vertices += 2;
            indices += 6;
        }

        draw->PrimWriteIdx(idx + 0);
        draw->PrimWriteIdx(idx + 1);
        draw->PrimWriteIdx(first_idx + 1);
        draw->PrimWriteIdx(idx + 0);
        draw->PrimWriteIdx(first_idx + 1);
        draw->PrimWriteIdx(first_idx + 0);
        indices += 6;

        ICE_ASSERT_CORE(max_indices == indices);
        ICE_ASSERT_CORE(max_vertices == vertices);

        //draw->AddCircle(Vec2(params.draw_center), params.draw_radius, 0xff'ffffff, params.draw_segments / 4, 1.f);

        // Preview utility
        // Close-up color
        if (is_active)
        {
            ImVec2 const mousepos = ImGui::GetMousePos();

            //ice::Color const point_color{ color.lightness, lch_chroma, degs, 1.0 };
            draw->AddCircleFilled(mousepos, 18.f, SrgbU32(draw_color), 24);
            draw->AddCircle(mousepos, 19.f, 0xffffffff, 24);
        }
        else
        {
            ice::f32 const wheel_thickness = params.draw_radius - params.draw_center_offset_radius;

            ice::f32 const current_chroma_x = params.draw_center_offset_radius + wheel_thickness * (1.f - ice::clamp(draw_color.chroma / max_chroma, 0.0f, 1.f));
            ice::rad const current_hue_rad = ice::radians(draw_color.hue);
            ice::vec2f const current_point = params.draw_center + ice::rotate2d(current_hue_rad) * ice::vec2f{ current_chroma_x, 0.0f };

            imcolor = SrgbU32(draw_color);
            draw->AddCircleFilled({ current_point.x, current_point.y }, 7.f, imcolor, 8);
            draw->AddCircle({ current_point.x, current_point.y }, 7.f, 0xffffff'ff, 8);
        }
    }

    bool colorpicker_oklch_wheel_item(
        ImGuiStyle const& style,
        ImGui::OkLCHPickerFlags flags,
        ice::vec2f rect_topleft,
        ice::vec2f wheel_radius,
        ice::vec2f wheel_center,
        ice::Color& inout_color,
        ice::f32 max_chroma,
        bool is_readonly
    ) noexcept
    {
        ImVec2 const mousepos = ImGui::GetMousePos();

        ice::vec2f const wheel_width = ice::vec2f{ wheel_radius.x, wheel_radius.x } * 2.f;
        ice::vec2f const cursor = { mousepos.x, mousepos.y };
        ice::vec2f const cursor_line = cursor - wheel_center;
        ice::f32 const distance = ice::math::length(cursor_line);
        ice::deg const degs = [cursor_line, distance]() noexcept
            {
                ice::deg degs = ice::degrees(ice::rad{ std::asin(-cursor_line.y / distance) });
                if (cursor_line.x < 0.0f)
                {
                    degs = 180.0_deg + degs;
                }
                else if (cursor_line.y < 0.0f)
                {
                    degs = 360.0_deg - degs;
                }
                else
                {
                    degs = degs * -1.f;
                }
                return degs;
            }();

        ice::f32 const distance_norm = (distance - wheel_radius.y) / (wheel_radius.x - wheel_radius.y);
        ice::f32 const mousepos_chroma = (1.f - distance_norm) * max_chroma;

        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

        bool value_changed = false;

        // TODO: Hue-Chroma wheel / Make controllable by flags?
        if constexpr (1)
        {
            ImGui::SetCursorScreenPos2(rect_topleft);
            ImGui::InvisibleButton("hue", ImGui::Compat::Vec2(wheel_width));
            if (ImGui::IsItemActive() && !is_readonly)
            {
                if ((distance > wheel_radius.y) && (distance < wheel_radius.x))
                {
                    inout_color.hue = degs;
                    inout_color.chroma = mousepos_chroma;
                    value_changed = true;
                }
            }
            // TODO: OpenPopupOnItemClick
        }

        ImGui::PopItemFlag();
        return value_changed;
    }

    bool colorpicker_oklch_items(
        ImGuiStyle const& style,
        ImGui::OkLCHPickerFlags flags,
        ice::vec2f rect_topright,
        ice::f32 wheel_width,
        ice::Color& inout_color,
        ice::f32 max_chroma,
        bool is_readonly
    ) noexcept
    {
        using ImGui::Vec2;
        bool const enable_alpha_value = ice::has_none(flags, ImGui::OkLCHPickerFlags::NoAlpha);
        bool const enable_alpha_bar = enable_alpha_value; // TODO: && (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar) == ImGuiColorEditFlags_AlphaBar;
        ice::f32 const bar_width = ImGui::GetFrameHeight();
        ice::f32 const bar_spacing = bar_width + style.ItemSpacing.x;
        ice::f32 const bar_height = wheel_width;

        ice::vec2f const mousepos = ImGui::Vec2(ImGui::GetMousePos());
        ice::f32 const mousepos_bar = std::clamp((mousepos.y - rect_topright.y) / bar_height, 0.0f, 1.0f);

        ice::vec2f const bar_size = { bar_width, bar_height };
        ice::vec2f bar_topleft = rect_topright - ice::vec2f{ bar_width, 0.f };

        ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);

        bool value_changed = false;
        if (enable_alpha_bar)
        {
            ImGui::SetCursorScreenPos2(bar_topleft);
            ImGui::InvisibleButton("alpha", Vec2(bar_size));
            if (ImGui::IsItemActive() && !is_readonly)
            {
                inout_color.alpha = 1.f - mousepos_bar;
                value_changed = true;
            }
        }

        // Move to the next bar
        bar_topleft.x -= bar_spacing;

        // TODO: Lightness Bar / Make controllable by flags?
        if constexpr (1)
        {
            ImGui::SetCursorScreenPos2(bar_topleft);
            ImGui::InvisibleButton("light", Vec2(bar_size));
            if (ImGui::IsItemActive() && !is_readonly)
            {
                inout_color.lightness = 1.f - mousepos_bar;
                value_changed = true;
            }

        }

        // Move to the next bar
        bar_topleft.x -= bar_spacing;

        // TODO: Chroma Bar / Make controllable by flags?
        if constexpr (1)
        {
            ImGui::SetCursorScreenPos2(bar_topleft);
            ImGui::InvisibleButton("chroma", Vec2(bar_size));
            if (ImGui::IsItemActive() && !is_readonly)
            {
                inout_color.chroma = std::max(ice::f32_eps, max_chroma - mousepos_bar * max_chroma);
                value_changed = true;
            }
        }

        // Move to the next bar
        bar_topleft.x -= bar_spacing;

        // TODO: Hue Bar / Make controllable by flags?
        if constexpr (1)
        {
            ImGui::SetCursorScreenPos2(bar_topleft);
            ImGui::InvisibleButton("hue_bar", Vec2(bar_size));
            if (ImGui::IsItemActive())
            {
                inout_color.hue = 360.0_deg * mousepos_bar;
            }
        }

        ImGui::PopItemFlag();

        return value_changed;
    }

    void colorpicker_oklch_draws(
        ImDrawList* draw_list,
        ImGuiStyle const& style,
        ImGui::OkLCHPickerFlags flags,
        ice::vec2f rect_topright,
        ice::f32 bar_height,
        ice::Color const& color,
        ice::f32 max_chroma
    ) noexcept
    {
        using ImGui::SrgbU32;
        using ImGui::Compat::Vec2;
        const bool enable_alpha_value = ice::has_none(flags, ImGui::OkLCHPickerFlags::NoAlpha);
        const bool enable_alpha_bar = enable_alpha_value; // TODO: && (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar) == ImGuiColorEditFlags_AlphaBar;
        ice::f32 const bar_width = ImGui::GetFrameHeight();
        ice::f32 const bar_spacing = bar_width + style.ItemSpacing.x;

        ice::vec2f bar_topleft = rect_topright - ice::vec2f{ bar_width, 0.f };
        ice::vec2f bar_botright = rect_topright + ice::vec2f{ 0.f, bar_height };
        ImVec2 const uv_white = draw_list->_Data->TexUvWhitePixel;

        if (enable_alpha_bar)
        {
            ImVec2 const topleft = Vec2(bar_topleft);
            ImVec2 const botright = Vec2(bar_botright);

            ImU32 const colu32 = SrgbU32({ color.lightness, color.chroma, color.hue, 1.f });
            ImU32 const colu32_alpha0 = colu32 & ~IM_COL32_A_MASK;

            ImGui::RenderColorRectWithAlphaCheckerboard(draw_list, topleft, botright, 0, bar_width / 2.f, {});
            draw_list->AddRectFilledMultiColor(topleft, botright, colu32, colu32, colu32_alpha0, colu32_alpha0);
            ImGui::RenderArrowsForVerticalBar2(draw_list, { topleft.x - 1, topleft.y + (1.f - color.alpha) * bar_height }, { 5.f, 5.f }, bar_width * 1.1f, 1.f);
        }

        // Move to the next bar
        bar_topleft.x -= bar_spacing;
        bar_botright.x -= bar_spacing;

        // TODO: Lightness Bar / Make controllable by flags?
        if constexpr (1)
        {
            ImVec2 const topleft = Vec2(bar_topleft);
            ImVec2 const botright = Vec2(bar_botright);

            ImGui::RenderColorRectWithAlphaCheckerboard(draw_list, topleft, botright, 0, bar_width / 2.f, {});

            ice::u32 steps = 10;
            ice::f32 const hstep = bar_height / steps;
            ImVec2 lp = topleft;
            ImVec2 rp = { topleft.x + bar_width, topleft.y };

            ImDrawIdx const first_idx = (ImDrawIdx)draw_list->_VtxCurrentIdx;
            ImDrawIdx idx = first_idx;

            ice::Color bar_color = color.brightened(1.f);
            draw_list->PrimReserve((steps) * 6, (steps + 1) * 2);
            draw_list->PrimWriteVtx(lp, uv_white, SrgbU32(bar_color));
            draw_list->PrimWriteVtx(rp, uv_white, SrgbU32(bar_color));
            for (ice::u32 n = 0; n < steps; ++n)
            {
                bar_color = bar_color.darkened(0.1f);

                lp.y += hstep;
                rp.y += hstep;
                draw_list->PrimWriteVtx(lp, uv_white, SrgbU32(bar_color));
                draw_list->PrimWriteVtx(rp, uv_white, SrgbU32(bar_color));

                draw_list->PrimWriteIdx(idx + 0);
                draw_list->PrimWriteIdx(idx + 1);
                draw_list->PrimWriteIdx(idx + 3);
                draw_list->PrimWriteIdx(idx + 0);
                draw_list->PrimWriteIdx(idx + 3);
                draw_list->PrimWriteIdx(idx + 2);
                idx += 2;
            }

            ImGui::RenderArrowsForVerticalBar2(draw_list, { topleft.x - 1, topleft.y + (1.f - color.lightness) * bar_height }, { 5.f, 5.f }, bar_width * 1.1f, 1.f);
        }

        // Move to the next bar
        bar_topleft.x -= bar_spacing;
        bar_botright.x -= bar_spacing;

        // TODO: Chroma Bar / Make controllable by flags?
        if constexpr (1)
        {
            ice::f32 const current_chroma_y = bar_height * (1.f - ice::clamp(color.chroma / max_chroma, 0.0f, 1.f));
            ImVec2 const topleft = Vec2(bar_topleft);
            ImVec2 const botright = Vec2(bar_botright);

            ice::Color const maxsat = color.saturated(1.f, max_chroma);
            ice::Color const minsat = color.desaturated(1.f);

            ImGui::RenderColorRectWithAlphaCheckerboard(draw_list, topleft, botright, 0, bar_width / 2.f, {});

            draw_list->AddRectFilledMultiColor(topleft, botright, SrgbU32(maxsat), SrgbU32(maxsat), SrgbU32(minsat), SrgbU32(minsat));
            ImGui::RenderArrowsForVerticalBar2(draw_list, { topleft.x - 1, topleft.y + current_chroma_y }, { 5.f, 5.f }, bar_width * 1.1f, 1.f);
        }

        // Move to the next bar
        bar_topleft.x -= bar_spacing;
        bar_botright.x -= bar_spacing;

        // TODO: Hue Bar / Make controllable by flags?
        if constexpr (1)
        {
            ImVec2 const topleft = Vec2(bar_topleft);
            ImVec2 const botright = Vec2(bar_botright);

            ImGui::RenderColorRectWithAlphaCheckerboard(draw_list, topleft, botright, 0, bar_width / 2.f, {});

            ice::u32 steps = 180;
            ice::f32 const hstep = bar_height / steps;
            ImVec2 lp = topleft;
            ImVec2 rp = { topleft.x + bar_width, topleft.y };

            ImDrawIdx const first_idx = (ImDrawIdx)draw_list->_VtxCurrentIdx;
            ImDrawIdx idx = first_idx;

            ice::Color stepcolor = color;
            stepcolor.hue = ice::deg32{ 0.0f };

            draw_list->PrimReserve((steps) * 6, (steps + 1) * 2);
            draw_list->PrimWriteVtx(lp, uv_white, ImGui::SrgbU32(stepcolor));
            draw_list->PrimWriteVtx(rp, uv_white, ImGui::SrgbU32(stepcolor));
            for (ice::u32 n = 0; n < steps; ++n)
            {
                stepcolor.hue._value += 2.f;

                lp.y += hstep;
                rp.y += hstep;
                draw_list->PrimWriteVtx(lp, uv_white, ImGui::SrgbU32(stepcolor));
                draw_list->PrimWriteVtx(rp, uv_white, ImGui::SrgbU32(stepcolor));

                draw_list->PrimWriteIdx(idx + 0);
                draw_list->PrimWriteIdx(idx + 1);
                draw_list->PrimWriteIdx(idx + 3);
                draw_list->PrimWriteIdx(idx + 0);
                draw_list->PrimWriteIdx(idx + 3);
                draw_list->PrimWriteIdx(idx + 2);
                idx += 2;
            }

            ImGui::RenderArrowsForVerticalBar2(draw_list, { topleft.x - 1, topleft.y + (color.hue.raw_value() / 360.0f) * bar_height }, { 5.f, 5.f }, bar_width * 1.1f, 1.f);
        }
    }

    static bool colorpicker_oklch_components(
        ImGuiStyle const& style,
        ice::Color& inout_color,
        ice::f32 available_width,
        ice::vec2f& rect_topleft
    ) noexcept
    {
        ice::f32 const left_pos = rect_topleft.x;
        ice::f32 const item_width = std::floor((available_width - style.ItemSpacing.x * 3.f) * 0.25f);

        bool value_changed = false;

        ImGui::SetCursorScreenPos2(rect_topleft);
        { // LCH+A
            ImGui::PushID("#LCHA");
            ImGui::SetNextItemWidth(item_width);
            value_changed |= ImGui::DragFloat("##L", &inout_color.lightness, 1.f / 255.f, 0.0f, 1.0f, "L:%.3f", ImGuiSliderFlags_AlwaysClamp); ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            value_changed |= ImGui::DragFloat("##C", &inout_color.chroma, 0.0032f, ice::f32_eps, 0.32f, "C:%.3f", ImGuiSliderFlags_AlwaysClamp); ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            value_changed |= ImGui::DragFloat("##H", &inout_color.hue._value, 1.f, 0.0f, 360.0f, "H:%.2f°", ImGuiSliderFlags_AlwaysClamp); ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            value_changed |= ImGui::DragFloat("##A", &inout_color.alpha, 1.f / 255.f, 0.0f, 1.0f, "A:%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopID();
        }

        // Reposition
        rect_topleft.x = left_pos;
        rect_topleft.y = ImGui::GetCursorScreenPos().y;

        ImGui::SetCursorScreenPos2(rect_topleft);
        { // RGB+A

            ice::color::SRGB rgb = ImGui::Srgb(inout_color);
            ImGui::PushID("#RGBA");
            ImGui::SetNextItemWidth(item_width);
            bool rgb_value_changed = false;
            rgb_value_changed |= ImGui::DragFloat("##R", &rgb.red, 1.f / 255.f, 0.0f, 1.0f, "R:%.3f"); ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            rgb_value_changed |= ImGui::DragFloat("##G", &rgb.green, 1.f / 255.f, 0.0f, 1.f, "G:%.3f"); ImGui::SameLine();
            ImGui::SetNextItemWidth(item_width);
            rgb_value_changed |= ImGui::DragFloat("##B", &rgb.blue, 1.f / 255.f, 0.0f, 1.f, "B:%.3f"); ImGui::SameLine();
            if (rgb_value_changed)
            {
                value_changed = rgb_value_changed;
                ice::Color const from_rgb = rgb.to_lrgb().to_oklch();
                inout_color.lightness = from_rgb.lightness;
                inout_color.chroma = from_rgb.chroma;
                inout_color.hue = from_rgb.hue;
            }
            ImGui::SetNextItemWidth(item_width);
            value_changed |= ImGui::DragFloat("##A", &inout_color.alpha, 1.f / 255.f, 0.0f, 1.0f, "A:%.3f", ImGuiSliderFlags_AlwaysClamp);
            ImGui::PopID();
        }

        rect_topleft.x = left_pos;
        rect_topleft.y = ImGui::GetCursorScreenPos().y;
        return value_changed;
    }

    static void colorpicker_oklch_previews(
        ImDrawList& draw_list,
        ImGuiStyle const& style,
        ice::vec2f rect_topleft,
        ice::vec2f rect_bottomright,
        ice::u32 im_color_current,
        ice::u32 im_color_ref,
        bool im_color_current_fallback,
        bool im_color_ref_fallback
    ) noexcept
    {
        using ImGui::Vec2;

        bool const show_refcolor = im_color_ref != 0 || im_color_ref_fallback == false;
        ice::vec2f const inner_spacing = Vec2(style.ItemSpacing);
        ice::f32 const width = (rect_bottomright.x - rect_topleft.x) / 4.0f;

        ice::vec2f const item_distance{ width, 0 };
        ice::vec2f const iteminner_distance{ inner_spacing.x, 0 };

        ImGui::SetCursorScreenPos2(rect_topleft);
        ImGui::Text("Current");
        if (im_color_current_fallback)
        {
            ImGui::SetCursorScreenPos2(rect_topleft + item_distance);
            ImGui::Text("(Fallback)");
        }

        if (show_refcolor)
        {
            ImGui::SetCursorScreenPos2(rect_topleft + iteminner_distance * 0.5f + item_distance * 2.f);
            ImGui::Text("Original");
            if (im_color_ref_fallback)
            {
                ImGui::SetCursorScreenPos2(rect_topleft + iteminner_distance * 0.5f + item_distance * 3.f);
                ImGui::Text("(Fallback)");
            }
        }

        // Restore the horizontal position
        ice::vec2f const box_topleft{ rect_topleft.x, ImGui::GetCursorScreenPos().y };
        ice::vec2f const box_bottomleft{ rect_topleft.x, rect_bottomright.y };

        // Draw the previews
        draw_list.AddRectFilled(
            Vec2(box_topleft),
            Vec2(box_bottomleft + item_distance),
            im_color_current_fallback ? 0x00'000000 : im_color_current
        );
        draw_list.AddRectFilled(
            Vec2(box_topleft + item_distance - iteminner_distance * 0.25f),
            Vec2(box_bottomleft + item_distance * 2.f - iteminner_distance * 0.5f),
            im_color_current
        );

        if (show_refcolor)
        {
            draw_list.AddRectFilled(
                Vec2(box_topleft + item_distance * 2.f + iteminner_distance * 0.5f),
                Vec2(box_bottomleft + item_distance * 3.f + iteminner_distance * 0.25f),
                im_color_ref_fallback ? 0x00'000000 : im_color_ref
            );
            draw_list.AddRectFilled(
                Vec2(box_topleft + item_distance * 3.f + iteminner_distance * 0.25f),
                Vec2(rect_bottomright),
                im_color_ref
            );
        }
    }

    bool colorpicker_oklch(
        ice::String label,
        ice::Color& inout_color,
        ImGui::OkLCHPickerFlags flags,
        ice::Color const* ref_color,
        ice::f32 max_chroma
    ) noexcept
    {
        using ImGui::Vec2;
        using ImGui::SrgbU32;

        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiWindow& window = *ImGui::GetCurrentWindow();
        if (window.SkipItems)
        {
            return false;
        }

        ImDrawList* const draw_list = window.DrawList;
        ImGuiStyle& style = g.Style;
        [[maybe_unused]] ImGuiIO& io = g.IO;

        ice::f32 const width_picker = std::max(ImGui::CalcItemWidth(), 400.f);
        bool const is_readonly = ((g.NextItemData.ItemFlags | g.CurrentItemFlags) & ImGuiItemFlags_ReadOnly) != 0;
        g.NextItemData.ClearFlags();

        ImGui::PushID(label);
        bool const set_current_color_edit_id = (g.ColorEditCurrentID == 0);
        if (set_current_color_edit_id)
        {
            g.ColorEditCurrentID = window.IDStack.back();
        }

        ImGui::BeginGroup();

#if 0  // TODO: Allow disabling the preview?
        if (!(flags & ImGuiColorEditFlags_NoSidePreview))
            flags |= ImGuiColorEditFlags_NoSmallPreview;
#endif

#if 0  // TODO: Implement options
        // Context menu: display and store options.
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            ColorPickerOptionsPopup(col, flags);
#endif

#if 0 // TODO: Try to understand this part?
        // Read stored options
        if (!(flags & ImGuiColorEditFlags_PickerMask_))
            flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_PickerMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_PickerMask_;
        if (!(flags & ImGuiColorEditFlags_InputMask_))
            flags |= ((g.ColorEditOptions & ImGuiColorEditFlags_InputMask_) ? g.ColorEditOptions : ImGuiColorEditFlags_DefaultOptions_) & ImGuiColorEditFlags_InputMask_;
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_PickerMask_)); // Check that only 1 is selected
        IM_ASSERT(ImIsPowerOfTwo(flags & ImGuiColorEditFlags_InputMask_));  // Check that only 1 is selected
#endif
        const bool enable_alpha_value = ice::has_none(flags, ImGui::OkLCHPickerFlags::NoAlpha);
        const bool enable_alpha_bar = enable_alpha_value; // TODO: && (g.ColorEditOptions & ImGuiColorEditFlags_AlphaBar) == ImGuiColorEditFlags_AlphaBar;

        // Setup
        ice::vec2f const padding = {};// Vec2(style.FramePadding);
        ice::vec2f const spacing = Vec2(style.ItemSpacing);
        ice::vec2f const picker_position = Vec2(ImGui::GetCursorScreenPos()) + padding;
        ice::f32 const picker_width_inner = width_picker - padding.x * 2;

        ice::u32 const count_bars = enable_alpha_bar ? 4 : 3;
        ice::f32 const width_bar = ImGui::GetFrameHeight(); // Arbitrary smallish width of Hue/Alpha picking bars
        ice::f32 const width_bars = count_bars * (width_bar + spacing.x); // Has spacing between wheel already
        ice::f32 const width_wheel = ice::max(width_bars, picker_width_inner - width_bars); // L + R

        ice::u32 const wheel_segments = 360;
        ice::f32 const wheel_radius_outer = width_wheel * 0.5f;
        ice::f32 const wheel_radius_inner = wheel_radius_outer * 0.1f;
        ice::vec2f const wheel_center = { wheel_radius_outer, wheel_radius_outer };
        ice::vec2f const wheel_position = picker_position + /*spacing +*/ wheel_center;

        ice::DrawWheelParams const params{
            .draw_style = ice::u32(ice::has_all(flags, ImGui::OkLCHPickerFlags::Chroma_ClipToMax)),
            .draw_segments = wheel_segments,
            .draw_radius = wheel_radius_outer,
            .draw_center_offset_radius = wheel_radius_inner,
            .draw_center = wheel_position,
            .uv_white = draw_list->_Data->TexUvWhitePixel
        };

        bool current_color_is_fallback;
        ImU32 const current_color = SrgbU32(inout_color, current_color_is_fallback);

        bool const wheel_active = colorpicker_oklch_wheel_item(
            style, flags, picker_position, { wheel_radius_outer, wheel_radius_inner }, wheel_position, inout_color, max_chroma, is_readonly
        );
        bool value_changed = wheel_active;

        ice::vec2f const bars_topright = { picker_position.x + picker_width_inner, picker_position.y };
        colorpicker_oklch_items(style, flags, bars_topright, width_wheel, inout_color, max_chroma, is_readonly);

        ice::vec2f mid_left = { picker_position.x, picker_position.y + width_wheel + spacing.y };
        colorpicker_oklch_components(style, inout_color, picker_width_inner, mid_left);

        // Rendering
        {
            colorpicker_oklch_draw_wheel(draw_list, inout_color, params, max_chroma, wheel_active);
            colorpicker_oklch_draws(draw_list, style, flags, bars_topright, width_wheel, inout_color, max_chroma);

            bool refcolor_is_fallback = true;
            ImU32 const refcolor = ref_color != nullptr ? SrgbU32(*ref_color, refcolor_is_fallback) : 0;

            colorpicker_oklch_previews(
                *draw_list, style,
                mid_left,
                { picker_position.x + picker_width_inner, mid_left.y + 60.f },
                current_color,
                refcolor,
                current_color_is_fallback,
                refcolor_is_fallback
            );

            ImGui::SetCursorScreenPos({ picker_position.x + width_picker, mid_left.y + 60.f + padding.y });
        }

        ImGui::Dummy({});
        ImGui::EndGroup();

        //// Render Frame
        //if (style.FrameBorderSize > 0.0f)
        //{
        //    ImVec2 const p_min = ImGui::GetItemRectMin();
        //    ImVec2 const p_max = ImGui::GetItemRectMax();
        //    draw_list->AddRect({ p_min.x + 1, p_min.y + 1 }, { p_max.x + 1, p_max.y + 1 }, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding, 0, style.FrameBorderSize);
        //    draw_list->AddRect(p_min, p_max, ImGui::GetColorU32(ImGuiCol_Border), style.FrameRounding, 0, style.FrameBorderSize);
        //}

#if 0
        if (value_changed && memcmp(backup_initial_col, col, components * sizeof(float)) == 0)
            value_changed = false;
#endif
        if (value_changed && g.LastItemData.ID != 0) // In case of ID collision, the second EndGroup() won't catch g.ActiveId
        {
            ImGui::MarkItemEdited(g.LastItemData.ID);
        }
        if (set_current_color_edit_id)
        {
            g.ColorEditCurrentID = 0;
        }
        ImGui::PopID();

        return value_changed;
    }

#if 0
    bool colorbutton_oklch(
        ice::String label,
        ice::Color& inout_color,
        ImGui::OkLCHPickerFlags flags,
        ice::Color const* ref_color,
        ice::f32 max_chroma
    ) noexcept
    {
        ImGuiContext& g = *ImGui::GetCurrentContext();
        ImGuiStyle& style = g.Style;

        inout_color.to_lrgb()

        const ImVec4 col_v4(col[0], col[1], col[2], alpha ? col[3] : 1.0f);
        if (ImGui::ColorButton("##ColorButton", col_v4, 0))
        {
            if (!(flags & ImGuiColorEditFlags_NoPicker))
            {
                // Store current color and open a picker
                g.ColorPickerRef = ImGui::Vec4(inout_color);
                ImGui::OpenPopup("oklch_picker");
                ImGui::SetNextWindowPos(g.LastItemData.Rect.GetBL() + ImVec2(0.0f, style.ItemSpacing.y));
            }
        }
        if (!(flags & ImGuiColorEditFlags_NoOptions))
            OpenPopupOnItemClick("context", ImGuiPopupFlags_MouseButtonRight);

        if (BeginPopup("picker"))
        {
            if (g.CurrentWindow->BeginCount == 1)
            {
                picker_active_window = g.CurrentWindow;
                if (label != label_display_end)
                {
                    TextEx(label, label_display_end);
                    Spacing();
                }
                ImGuiColorEditFlags picker_flags_to_forward = ImGuiColorEditFlags_DataTypeMask_ | ImGuiColorEditFlags_PickerMask_ | ImGuiColorEditFlags_InputMask_ | ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_AlphaBar;
                ImGuiColorEditFlags picker_flags = (flags_untouched & picker_flags_to_forward) | ImGuiColorEditFlags_DisplayMask_ | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_AlphaPreviewHalf;
                SetNextItemWidth(square_sz * 12.0f); // Use 256 + bar sizes?
                value_changed |= ColorPicker4("##picker", col, picker_flags, &g.ColorPickerRef.x);
            }
            EndPopup();
        }
    }
#endif

} // namespace ice
