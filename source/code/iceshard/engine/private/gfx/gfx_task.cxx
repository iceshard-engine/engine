#include <ice/gfx/gfx_task.hxx>
#include <ice/gfx/gfx_frame.hxx>
#include <ice/gfx/gfx_device.hxx>
#include <ice/render/render_buffer.hxx>
#include <ice/render/render_device.hxx>

namespace ice::gfx
{

    auto load_image_data_task(
        ice::gfx::GfxDevice& gfx_device,
        ice::gfx::GfxFrame& gfx_frame,
        ice::gfx::GfxTaskLoadImage gfx_task_info
    ) noexcept -> ice::Task<void>
    {
        ice::u32 const image_data_size = gfx_task_info.image_info.width * gfx_task_info.image_info.height * 4;

        // Currently we start the task on the graphics thread, so we can dont race for access to the render device.
        //  It is planned to create an awaiter for graphics thread access so we can schedule this at any point from any thread.
        ice::render::RenderDevice& device = gfx_device.device();
        ice::render::Buffer const data_buffer = device.create_buffer(
            ice::render::BufferType::Transfer,
            image_data_size
        );

        ice::render::BufferUpdateInfo updates[]{
            ice::render::BufferUpdateInfo
            {
                .buffer = data_buffer,
                .data = {
                    .location = gfx_task_info.image_info.data,
                    .size = image_data_size,
                    .alignment = 4
                }
            }
        };

        device.update_buffers(updates);

        // Await command recording stage
        //  Here we have access to a command buffer where we can record commands.
        //  These commands will be later executed on the graphics thread.
        ice::gfx::GfxTaskCommands& cmds = co_await gfx_frame.frame_commands("default"_sid);

        cmds.update_texture(
            gfx_task_info.image,
            data_buffer,
            { gfx_task_info.image_info.width, gfx_task_info.image_info.height }
        );

        // Await end of graphics frame.
        //  Here we know that all commands have been executed
        //  and temporary objects can be destroyed.
        co_await gfx_frame.frame_end();

        device.destroy_buffer(data_buffer);
    }

} // namespace ice::gfx
