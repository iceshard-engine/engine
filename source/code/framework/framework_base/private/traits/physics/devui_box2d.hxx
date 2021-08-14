#pragma once
#include <ice/devui/devui_widget.hxx>

#include "trait_box2d.hxx"
#include "box2d.hxx"

namespace ice
{

    class DevUI_Box2D : public ice::devui::DevUIWidget
    {
    public:
        DevUI_Box2D(
            b2World& box2d_world
        ) noexcept;

        void on_prepare(void* context) noexcept override;
        void on_draw() noexcept override;

        void on_frame(ice::EngineFrame& frame) noexcept;

    private:
        b2World& _world;
        ice::u32 _debug_draw_flags;
        bool _visible;
    };

} // namespace ice
