#pragma once
#include <ice/span.hxx>
#include <ice/pod/hash.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <atomic>

namespace ice::ecs
{

    class EntityIndex
    {
    public:
        EntityIndex(
            ice::Allocator& alloc,
            ice::u32 estimated_entity_count,
            ice::u32 maximum_entity_count = ice::u32_max
        ) noexcept;

        ~EntityIndex() noexcept = default;

        auto count() const noexcept -> ice::u32;
        bool is_alive(ice::ecs::Entity entity) const noexcept;

        auto create() noexcept -> ice::ecs::Entity;
        bool create_many(ice::Span<ice::ecs::Entity> out_entities) noexcept;

        void destroy(ice::ecs::Entity entity) noexcept;
        void destroy_many(ice::Span<ice::ecs::Entity> entities) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::u32 const _max_entity_count;

        ice::pod::Queue<ice::u32> _free_indices;
        ice::pod::Array<ice::u8> _generation;
    };

} // namespace ice::ecs
