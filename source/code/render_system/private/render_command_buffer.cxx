#include <render_system/render_command_buffer.hxx>

#include <core/pod/array.hxx>

namespace render
{

    RenderCommandBuffer::RenderCommandBuffer(core::allocator& alloc) noexcept
        : _command_list{ alloc }
        , _command_data{ alloc }
    {
    }

    RenderCommandBuffer::~RenderCommandBuffer() noexcept
    {
        core::pod::array::clear(_command_list);
        _command_data.clear();
    }

    void RenderCommandBuffer::clear() noexcept
    {
        _command_data.clear();
    }

    void RenderCommandBuffer::push(CommandName command, core::data_view_aligned command_data) noexcept
    {
        core::pod::array::push_back(_command_list, command);
        _command_data.push(command_data);
    }

} // namespace render
