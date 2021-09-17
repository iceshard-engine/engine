#pragma once
#include <ice/allocator.hxx>
#include <ice/ecs/ecs_archetype.hxx>
#include <ice/span.hxx>

namespace ice::ecs
{

    class DataBlockPool;

    struct ArchetypePoolSubscription
    {
        ice::ecs::ArchetypeComponentsInfo components_info;
        ice::ecs::DataBlockPool* component_block_pool;
    };

    class ArchetypeIndex
    {
    public:
        ArchetypeIndex(
            ice::Allocator& alloc
        ) noexcept;

        ~ArchetypeIndex() noexcept = default;

        auto subscribe_archetype(
            ice::ecs::ArchetypePoolSubscription const& pool_subscription
        ) noexcept -> ice::ecs::Archetype;

        auto subscribe_archetype(
            ice::Span<ice::ecs::ArchetypePoolSubscription const> pool_subscriptions
        ) noexcept -> ice::ecs::Archetype;

        // #todo Create a archetype component query
        auto find_archetype_pool(
            ice::ecs::ArchetypeComponentsInfo const& components_info
        ) noexcept -> ice::ecs::DataBlockPool*;

        auto get_archetype_pool(
            ice::ecs::Archetype archetype
        ) noexcept -> ice::ecs::DataBlockPool*;

    private:
        ice::Allocator& _allocator;
    };

} // namespace ice::ecs
