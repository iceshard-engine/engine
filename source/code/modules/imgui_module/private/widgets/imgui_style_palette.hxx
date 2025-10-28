#pragma once
#include <ice/mem_allocator.hxx>
#include <ice/devui_widget.hxx>

namespace ice::devui
{

    namespace styles
    {

        enum class Theme : ice::u32
        {
            Dark = 0,
            Light,
        };

        void apply_color_theme(Theme theme) noexcept;
        void pop_color_theme() noexcept;

        auto apply_stylesheet() noexcept -> ice::u32;
        void pop_stylesheet() noexcept;

    } // namespace styles


    class ImGuiStylePalette : public ice::DevUIWidget
    {
    public:
        ImGuiStylePalette(ice::Allocator& alloc) noexcept;
        ~ImGuiStylePalette() noexcept override = default;

        void build_content() noexcept override;
    };

} // namespace ice::devui::styles
