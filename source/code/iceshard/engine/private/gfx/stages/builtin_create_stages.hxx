#pragma once
#include <ice/gfx/gfx_stage.hxx>
#include <ice/gfx/gfx_object_storage.hxx>
#include <ice/gfx/gfx_device.hxx>

#include <ice/render/render_declarations.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_device.hxx>

namespace ice::gfx::v2::builtin
{

    using namespace ice::render;

    struct Stage_CreateImage final : ice::gfx::v2::GfxStage
    {
        struct Params
        {
            ice::BaseStringID<false> name;
            ice::render::ImageType type;
            ice::render::ImageFormat format;
            ice::render::ImageUsageFlags flags;
            ice::vec2f size;
        };

        struct Runtime
        {
            ice::render::Image* _image;
        };

        Stage_CreateImage(Params const* params, Runtime const* runtime) noexcept
            : _params{ params }
            , _runtime{ runtime }
        { }

        Params const* _params;
        Runtime const* _runtime;

        void execute(
            ice::gfx::v2::GfxStageParams const& params,
            ice::gfx::v2::GfxObjectStorage& storage
        ) noexcept override;
    };

    struct Stage_CreateFramebuffer final : ice::gfx::v2::GfxStage
    {

        struct Params
        {
        };

        struct Runtime
        {
            ice::vec2u _size;
            //ice::gfx::v2::GfxObject const* _renderpass;
            ice::render::Renderpass const* _renderpass;
            ice::gfx::v2::GfxObject const** _inputs;
            //ice::Span<ice::render::Image> _inputs;
            //ice::gfx::v2::GfxObject* _out_framebuffer;
            ice::render::Framebuffer* _framebuffer;
        };

        Params const* _params;
        Runtime const* _runtime;

        void execute(
            ice::gfx::v2::GfxStageParams const& params,
            ice::gfx::v2::GfxObjectStorage& storage
        ) noexcept override;
    };

    struct Stage_CreateRenderpass final : ice::gfx::v2::GfxStage
    {
        struct Params { };
        struct Runtime
        {
            Renderpass* _renderpass;
        };

        Params const* _params;
        Runtime const* _runtime;

        void execute(
            ice::gfx::v2::GfxStageParams const& params,
            ice::gfx::v2::GfxObjectStorage& storage
        ) noexcept override;
    };

    struct StageEntry
    {
        GfxStage* stage;
        void const* params;
        void const* runtime;
    };

} // namespace ice::gfx::v2
