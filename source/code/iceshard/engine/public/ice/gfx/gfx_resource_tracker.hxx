#pragma once
#include <ice/stringid.hxx>
#include <ice/gfx/gfx_resource.hxx>

namespace ice::gfx
{

    class GfxResourceTracker
    {
    public:
        virtual ~GfxResourceTracker() noexcept = default;

        inline void track_resource(
            ice::StringID_Arg name,
            ice::render::Renderpass renderpass
        ) noexcept;

        inline void track_resource(
            ice::StringID_Arg name,
            ice::render::Framebuffer framebuffer
        ) noexcept;

        inline void track_resource(
            ice::StringID_Arg name,
            ice::render::Image image
        ) noexcept;

        virtual void track_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResource resource
        ) noexcept = 0;

        virtual auto find_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResource::Type type
        ) noexcept -> ice::gfx::GfxResource = 0;
    };

    inline void GfxResourceTracker::track_resource(
        ice::StringID_Arg name,
        ice::render::Renderpass renderpass
    ) noexcept
    {
        this->track_resource(
            name,
            ice::gfx::GfxResource{
                .type = GfxResource::Type::Renderpass,
                .value{.renderpass = renderpass}
            }
        );
    }

    inline void GfxResourceTracker::track_resource(
        ice::StringID_Arg name,
        ice::render::Framebuffer framebuffer
    ) noexcept
    {
        this->track_resource(
            name,
            ice::gfx::GfxResource{
                .type = GfxResource::Type::Framebuffer,
                .value{.framebuffer = framebuffer}
            }
        );
    }

    inline void GfxResourceTracker::track_resource(
        ice::StringID_Arg name,
        ice::render::Image image
    ) noexcept
    {
        this->track_resource(
            name,
            ice::gfx::GfxResource{
                .type = GfxResource::Type::Image,
                .value{.image = image}
            }
        );
    }

} // namespace ice::gfx
