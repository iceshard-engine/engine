#pragma once
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/entity/entity.hxx>
#include <ice/span.hxx>

namespace ice
{

    enum class CommandType : ice::u32
    {
        DestroyEntity
    };

    class EntityCommandBuffer
    {
    public:
        EntityCommandBuffer(
            ice::Allocator& alloc
        ) noexcept;

        ~EntityCommandBuffer() noexcept;

        void destroy_entity(ice::Entity entity) noexcept;

        struct Command
        {
            ice::CommandType type;
            ice::u32 reserved;
            ice::Entity entity;
        };

        auto commands() const noexcept -> ice::Span<Command const>;

    private:
        ice::pod::Array<Command> _commands;
    };

} // namespace ice
