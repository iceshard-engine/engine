/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/ecs/ecs_entity.hxx>
#include <ice/container/queue.hxx>
#include <atomic>

namespace ice::ecs
{

    //! \brief A constant value that is used to cut of where the Entity Index
    //!  is allowed to re-use already destroyed entities.
    //! \note If the `_free_indices` array is bigger than this value, the index is allowed
    //!  to reuse the released indices first.
    static constexpr ice::u32 Constant_MinimumFreeIndicesBeforeReuse = 1024;

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

        ice::Queue<ice::u32> _free_indices;
        ice::Array<ice::u8> _generation;
    };

} // namespace ice::ecs
