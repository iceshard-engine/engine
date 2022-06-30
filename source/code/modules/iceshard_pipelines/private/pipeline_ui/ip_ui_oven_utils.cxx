#include "ip_ui_oven_utils.hxx"
#include <ice/ui_element_info.hxx>

namespace ice
{
    void parse_element_size(char const* it, char const* end, ice::ui::ElementFlags& out_flags, ice::ui::Size& size) noexcept
    {
        using ice::ui::ElementFlags;

        auto const* const separator = std::find_if(it, end, [](char c) noexcept { return c == ','; });
        if (separator)
        {
            auto const res_w = std::from_chars(it, separator, size.width);
            auto const res_h = std::from_chars(separator + 1, end, size.height);

            if (res_w; res_w.ec != std::errc{})
            {
                if (strncmp(res_w.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_AutoWidth;
                }
                else if (strncmp(res_w.ptr, "*", 1) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_StretchWidth;
                }
            }
            if (res_h; res_h.ec != std::errc{})
            {
                if (strncmp(res_h.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_AutoHeight;
                }
                else if (strncmp(res_h.ptr, "*", 1) == 0)
                {
                    out_flags = out_flags | ElementFlags::Size_StretchHeight;
                }
            }
        }
    }

    void parse_element_pos(char const* it, char const* end, ice::ui::ElementFlags& out_flags, ice::ui::Position& pos) noexcept
    {
        using ice::ui::ElementFlags;
        auto const* const separator = std::find_if(it, end, [](char c) noexcept { return c == ','; });

        if (separator)
        {
            auto const res_x = std::from_chars(it, separator, pos.x);
            auto const res_y = std::from_chars(separator + 1, end, pos.y);

            if (res_x; res_x.ec != std::errc{})
            {
                if (strncmp(res_x.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Position_AutoX;
                }
            }
            else if (pos.x < 0)
            {
                out_flags = out_flags | ElementFlags::Position_AnchorRight;
                pos.x *= -1.f;
            }

            if (res_y; res_y.ec != std::errc{})
            {
                if (strncmp(res_y.ptr, "auto", 4) == 0)
                {
                    out_flags = out_flags | ElementFlags::Position_AutoY;
                }
            }
            else if (pos.y < 0)
            {
                out_flags = out_flags | ElementFlags::Position_AnchorBottom;
                pos.y *= -1.f;
            }
        }
    }

    void parse_element_offset(char const* it, char const* end, ice::ui::ElementFlags& out_flags, ice::ui::RectOffset& offset) noexcept
    {
        using ice::ui::ElementFlags;

        char const* const sep1 = std::find_if(it, end, [](char c) noexcept { return c == ','; });
        char const* const sep2 = std::find_if(sep1 + 1, end, [](char c) noexcept { return c == ','; });
        char const* const sep3 = std::find_if(sep2 + 1, end, [](char c) noexcept { return c == ','; });

        if (sep1 && sep2 && sep3)
        {
            auto const res_l = std::from_chars(it, sep1, offset.left);
            auto const res_t = std::from_chars(sep1 + 1, sep2, offset.top);
            auto const res_r = std::from_chars(sep2 + 1, sep3, offset.right);
            auto const res_b = std::from_chars(sep3 + 1, end, offset.bottom);

            //if (res_l; res_l.ec != std::errc{})
            //{
            //    if (strncmp(res_l.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoLeft;
            //    }
            //}
            //if (res_t; res_t.ec != std::errc{})
            //{
            //    if (strncmp(res_t.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoTop;
            //    }
            //}
            //if (res_r; res_r.ec != std::errc{})
            //{
            //    if (strncmp(res_r.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoRight;
            //    }
            //}
            //if (res_b; res_b.ec != std::errc{})
            //{
            //    if (strncmp(res_b.ptr, "auto", 4) == 0)
            //    {
            //        out_flags = out_flags | ElementFlags::Offset_AutoBottom;
            //    }
            //}
        }
    }
} // namespace ice
