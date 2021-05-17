#pragma once
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/render/render_image.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::gfx
{

    class GfxTaskCommands
    {
    public:
        virtual ~GfxTaskCommands() noexcept = default;

        virtual void update_texture(
            ice::render::Image image,
            ice::render::Buffer image_contents,
            ice::vec2u extents
        ) noexcept = 0;
    };

    struct GfxTaskLoadImage
    {
        ice::render::Image image;
        ice::render::ImageInfo image_info;
    };

    class GfxDevice;
    class GfxFrame;

    auto load_image_data_task(
        ice::gfx::GfxDevice& gfx_device,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxTaskLoadImage gfx_task_info
    ) noexcept -> ice::Task<void>;

} // namespace ice::gfx
