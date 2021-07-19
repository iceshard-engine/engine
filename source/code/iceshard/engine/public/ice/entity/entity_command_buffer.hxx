#pragma once
#include <ice/allocator.hxx>
#include <ice/entity/entity.hxx>
#include <ice/entity/entity_archetype.hxx>

namespace ice
{

    class EntityCommandBuffer
    {
    public:
        EntityCommandBuffer(
            ice::Allocator& alloc
        ) noexcept;
        ~EntityCommandBuffer() noexcept;


    };

} // namespace ice
