#pragma once
#include <ice/span.hxx>
#include <ice/allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/entity/entity.hxx>

namespace ice
{

    class EntityIndex
    {
    public:
        EntityIndex(
            ice::Allocator& alloc,
            ice::u32 estimated_entity_count,
            ice::u32 maximum_entity_count = std::numeric_limits<ice::u32>::max()
        ) noexcept;
        ~EntityIndex() noexcept = default;

        [[nodiscard]]
        auto count() noexcept -> ice::u32;

        [[nodiscard]]
        bool is_alive(
            ice::Entity entity
        ) noexcept;


        [[nodiscard]]
        auto create(
            void const* owner = nullptr
        ) noexcept -> ice::Entity;

        bool create_many(
            ice::Span<ice::Entity> values_out,
            void const* owner = nullptr
        ) noexcept;

        void destroy(
            ice::Entity entity
        ) noexcept;


        auto count_owned(
            void const* owner
        ) noexcept -> ice::u32;

        void get_owned(
            void const* owner,
            ice::pod::Array<ice::Entity>& entities_out
        ) noexcept;

        bool destroy_owned(
            void const* owner
        ) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::u32 const _max_entity_count;

        ice::pod::Queue<ice::u32> _free_indices;

        ice::pod::Array<ice::u16> _generation;
        ice::pod::Array<void const*> _owner;
    };

} // namespace ice
