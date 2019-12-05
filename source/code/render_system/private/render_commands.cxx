#include <render_system/render_commands.hxx>
#include <render_system/render_command_buffer.hxx>

render::CommandName render::data::Clear::command_name{ core::cexpr::stringid("RenderCommand.Clear") };

void render::assert_render_api() noexcept
{
    render::api::v1::render_api_instance->check_func();
}

namespace render::command
{

    void clear(RenderCommandBuffer& buffer, const data::Clear& data) noexcept
    {
        buffer::push(buffer, data);
    }

} // namespace render::command