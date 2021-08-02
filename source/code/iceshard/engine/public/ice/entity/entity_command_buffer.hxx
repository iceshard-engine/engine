#pragma once
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/entity/entity.hxx>
#include <ice/span.hxx>
#include <ice/shard.hxx>

namespace ice
{

    class EntityCommandBuffer
    {
    public:
        EntityCommandBuffer(
            ice::Allocator& alloc
        ) noexcept;

        ~EntityCommandBuffer() noexcept;

        void destroy_entity(ice::Entity entity) noexcept;

        auto commands() const noexcept -> ice::Span<ice::Shard const>;

    private:
        ice::pod::Array<ice::Shard> _commands;
    };

} // namespace ice
