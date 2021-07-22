#pragma once
#include <ice/engine_types.hxx>
#include <ice/sync_manual_events.hxx>

namespace ice::gfx
{

    class GfxTrait;
    class GfxDevice;
    class GfxFrame;

    class GfxRunner
    {
    public:
        virtual ~GfxRunner() noexcept = default;

        virtual void add_trait(ice::gfx::GfxTrait* trait) noexcept = 0;

        virtual void set_event(ice::ManualResetEvent* event) noexcept = 0;

        virtual auto device() noexcept -> ice::gfx::GfxDevice& = 0;
        virtual auto frame() noexcept -> ice::gfx::GfxFrame& = 0;

        virtual void draw_frame(ice::EngineFrame const& engine_frame) noexcept = 0;
    };

} // namespace ice::gfx
