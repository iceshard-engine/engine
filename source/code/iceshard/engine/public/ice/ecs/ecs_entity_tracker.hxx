#pragma once
#include <ice/allocator.hxx>
#include <ice/stringid.hxx>
#include <ice/pod/collections.hxx>
#include <ice/shard_container.hxx>

#include <ice/ecs/ecs_entity.hxx>

namespace ice::ecs
{

    class EntityTracker
    {
    public:
        EntityTracker(
            ice::Allocator& alloc
        ) noexcept;
        ~EntityTracker() noexcept;

        auto find_handle(ice::ecs::Entity entity) const noexcept -> ice::ecs::EntityHandle;
        auto find_handle(ice::StringID name) const noexcept -> ice::ecs::EntityHandle;

        void track_entity(
            ice::ecs::Entity entity,
            ice::StringID name = ice::stringid_invalid
        ) noexcept;

        void forget_entity(ice::ecs::Entity entity) noexcept;
        void forget_entity(ice::StringID name) noexcept;

        void refresh_handles(ice::ShardContainer const& shards) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::ecs::EntityHandle> _tracked_entities;
        ice::pod::Hash<ice::ecs::Entity> _named_entities;
    };

} // namespace ice::ecs
