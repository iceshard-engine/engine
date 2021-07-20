#include <ice/entity/entity_command_buffer.hxx>
#include <ice/hash.hxx>

namespace ice
{

    EntityCommandBuffer::EntityCommandBuffer(ice::Allocator& alloc) noexcept
        : _commands{ alloc }
    {
        ice::pod::array::reserve(_commands, 10);
    }

    EntityCommandBuffer::~EntityCommandBuffer() noexcept
    {
    }

    void EntityCommandBuffer::destroy_entity(ice::Entity entity) noexcept
    {
        ice::pod::array::push_back(
            _commands, Command
            {
                .type = CommandType::DestroyEntity,
                .entity = entity
            }
        );
    }

    auto EntityCommandBuffer::commands() const noexcept -> ice::Span<Command const>
    {
        return _commands;
    }

} // namespace ice
