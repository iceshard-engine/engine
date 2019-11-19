#include <iceshard/entity/entity_index.hxx>
#include <iceshard/component/component_system.hxx>
#include <core/pod/hash.hxx>

namespace iceshard
{
    namespace detail
    {

        auto hash(entity_handle_type handle) noexcept
        {
            return static_cast<uint64_t>(handle);
        }

        auto hash(core::cexpr::stringid_argument_type sid) noexcept
        {
            return static_cast<uint64_t>(sid.hash_value);
        }

        auto hash(ComponentSystem* component_system) noexcept
        {
            return hash(component_system->name());
        }

        auto hash_mix(uint64_t left, uint64_t right) noexcept
        {
            left ^= (left >> 33);
            left ^= right;
            left *= 0xff51afd7ed558ccd;
            left ^= (left >> 33);
            left ^= right;
            left *= 0xc4ceb9fe1a85ec53;
            left ^= (left >> 33);
            return left;
        }

        auto make_prototype(uint64_t prototype_hash) noexcept -> core::cexpr::stringid_type
        {
            return { core::cexpr::stringid_hash_type{ prototype_hash } };
        }

    } // namespace detail

    EntityIndex::EntityIndex(core::allocator& alloc) noexcept
        : _temp_allocator{ alloc, 4096 }
        , _entity_index{ alloc }
        , _prototype_map{ alloc }
    {
        core::pod::hash::reserve(_prototype_map, 100);
    }

    void EntityIndex::register_component(entity_handle_type entity, core::cexpr::stringid_argument_type component_name, ComponentSystem* component_system) noexcept
    {
        const auto entity_prototype = core::pod::hash::get(_entity_index, detail::hash(entity), core::cexpr::stringid_invalid);
        const auto new_prototype_hash = detail::hash_mix(
            detail::hash(entity_prototype),
            detail::hash_mix(detail::hash(component_name), detail::hash(component_system))
        );

        if (!core::pod::hash::has(_prototype_map, new_prototype_hash))
        {
            core::pod::hash::set(_prototype_map, new_prototype_hash, { entity_prototype, component_name, component_system });
        }

        core::pod::hash::set(_entity_index, detail::hash(entity), detail::make_prototype(new_prototype_hash));
    }

    auto EntityIndex::find_component_system(entity_handle_type entity, core::cexpr::stringid_argument_type component_name) noexcept -> ComponentSystem *
    {
        auto entity_prototype = core::pod::hash::get(_entity_index, detail::hash(entity), core::cexpr::stringid_invalid);
        while (entity_prototype != core::cexpr::stringid_invalid)
        {
            const auto prototype_hash = detail::hash(entity_prototype);

            auto prototype_info = core::pod::hash::get(
                _prototype_map,
                prototype_hash,
                { core::cexpr::stringid_invalid, core::cexpr::stringid_invalid, nullptr }
            );

            if (prototype_info.component_name == component_name)
            {
                return prototype_info.component_system;
            }
            entity_prototype = prototype_info.base_prototype;
        }
        return nullptr;
    }

    void EntityIndex::remove_component(entity_handle_type entity, core::cexpr::stringid_argument_type component_name) noexcept
    {
        //const auto entity_hash = detail::hash(entity);
        //const auto entity_prototype = core::pod::hash::get(_entity_index, entity_hash, core::cexpr::stringid_invalid);
        //const auto prototype_hash = detail::hash(entity_prototype);

        //if (core::pod::hash::has(_entity_index, entity_hash))
        //{
        //    const auto prototype_info = core::pod::hash::get(
        //        _prototype_map,
        //        prototype_hash,
        //        { core::cexpr::stringid_invalid, core::cexpr::stringid_invalid, nullptr }
        //    );

        //    core::pod::hash::set(_entity_index, entity_hash, prototype_info.base_prototype);
        //}

        core::pod::Array<PrototypeInfo> _prototypes{ _temp_allocator };

        auto old_entity_prototype = core::pod::hash::get(_entity_index, detail::hash(entity), core::cexpr::stringid_invalid);
        while (old_entity_prototype != core::cexpr::stringid_invalid)
        {
            const auto prototype_hash = detail::hash(old_entity_prototype);

            auto prototype_info = core::pod::hash::get(
                _prototype_map,
                prototype_hash,
                { core::cexpr::stringid_invalid, core::cexpr::stringid_invalid, nullptr }
            );

            if (prototype_info.component_name == component_name)
            {
                auto new_entity_protype = prototype_info.base_prototype;

                while (!core::pod::array::empty(_prototypes))
                {
                    const auto& prototype = core::pod::array::back(_prototypes);
                    core::pod::array::pop_back(_prototypes);

                    // Create a new prototype from those stored back for later
                    const auto new_prototype_hash = detail::hash_mix(
                        detail::hash(new_entity_protype),
                        detail::hash_mix(detail::hash(prototype.component_name), detail::hash(prototype.component_system))
                    );

                    // Store the created prototype if not existing
                    if (!core::pod::hash::has(_prototype_map, new_prototype_hash))
                    {
                        core::pod::hash::set(_prototype_map, new_prototype_hash, { new_entity_protype, prototype.component_name, prototype.component_system });
                    }

                    new_entity_protype = detail::make_prototype(new_prototype_hash);
                }

                core::pod::hash::set(_entity_index, detail::hash(entity), new_entity_protype);
                return;
            }
            else
            {
                core::pod::array::push_back(_prototypes, prototype_info);
            }
            old_entity_prototype = prototype_info.base_prototype;
        }
        return;
    }

    void EntityIndex::remove_entity(entity_handle_type entity) noexcept
    {
        core::pod::hash::remove(_entity_index, detail::hash(entity));
    }

} // namespace iceshard
