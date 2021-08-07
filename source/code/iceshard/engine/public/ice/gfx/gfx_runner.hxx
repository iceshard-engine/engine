#pragma once
#include <ice/stringid.hxx>
#include <ice/pod/array.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/engine_types.hxx>

namespace ice::gfx
{

    class GfxTrait;
    class GfxDevice;
    class GfxFrame;

    class GfxRunner
    {
    public:
        virtual ~GfxRunner() noexcept = default;

        virtual void add_trait(
            ice::StringID_Arg name,
            ice::gfx::GfxTrait* trait
        ) noexcept = 0;

        virtual auto graphics_world_name() noexcept -> ice::StringID = 0;
        virtual void set_graphics_world(
            ice::StringID_Arg world_name
        ) noexcept = 0;

        virtual void prepare_world(ice::World* world) noexcept = 0;
        virtual void cleanup_world(ice::World* world) noexcept = 0;

        virtual void set_event(ice::ManualResetEvent* event) noexcept = 0;

        virtual auto device() noexcept -> ice::gfx::GfxDevice& = 0;
        virtual auto frame() noexcept -> ice::gfx::GfxFrame& = 0;

        virtual void draw_frame(ice::EngineFrame const& engine_frame) noexcept = 0;
    };

} // namespace ice::gfx
