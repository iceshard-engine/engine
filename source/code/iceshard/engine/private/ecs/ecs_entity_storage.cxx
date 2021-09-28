#include <ice/ecs/ecs_entity_storage.hxx>
#include <ice/ecs/ecs_entity_operations.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice::ecs
{

    EntityStorage::EntityStorage(
        ice::Allocator& alloc,
        ice::ecs::ArchetypeIndex const& archetype_index
    ) noexcept
        : _allocator{ alloc }
        , _archetype_index{ archetype_index }
    {
    }

    void EntityStorage::execute_operations(
        ice::ecs::EntityOperations const& operations
    ) noexcept
    {
        // Set Archetype: {EntityHandle[], DstArchetype, ComponentData[]} // set
        // Rep Archetype: {EntityHandle[], DstArchetype, <implicit: SrcArchetype>, ComponentData[]} // change
        // Set Component: {EntityHandle[], None, ComponentData[]} // update data
        // Set Component: {EntityHandle[], None} // remove

        for (EntityOperation const& operation : operations)
        {
            ICE_LOG(
                ice::LogSeverity::Debug, ice::LogTag::Engine,
                "Executing operation with {} entities.",
                operation.entity_count
            );

            if constexpr (ice::build::is_debug || ice::build::is_develop)
            {
                // #todo: Performance warning check.
                //  This might have quite a big performance impact if done from multiple source archetypes.
                for (ice::u32 idx = 0; idx < operation.entity_count; ++idx)
                {
                    EntityHandle const handle = operation.entities[idx];

                    // #todo: Ensure all entities are using the same source archetype if any.
                    // #todo: Think through if we might want to allow mixing entity source archetypes when changing their destination archetype?
                }
            }

            // If we have an invalid archetype we are destroying entities or just updating data.
            //  This is a valid value and the operation should be well-formed.
            if (operation.archetype == Archetype::Invalid)
            {

            }
            else
            {
                DataBlockPool* instance_pool = nullptr;
                ArchetypeInstanceInfo* instance_info = nullptr;

                // #todo: this function assumes that the archetype is valid, otherwise it assets. Maybe we should allow invalid archetypes?
                _archetype_index.fetch_archetype_instance_info_with_pool(operation.archetype, instance_info, instance_pool);
            }

        }
    }

} // namespace ice::ecs
