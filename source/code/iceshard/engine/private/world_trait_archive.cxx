/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/world/world_trait_archive.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>
#include <ice/world/world_trait.hxx>

namespace ice
{

#if 0
    class SimpleWorldTraitArchive : public ice::WorldTraitArchive
    {
    public:
        SimpleWorldTraitArchive(ice::Allocator& alloc) noexcept;

        void register_trait(
            ice::StringID_Arg name,
            ice::WorldTraitDescription description
        ) noexcept override;

        auto find_trait(
            ice::StringID_Arg name
        ) const noexcept -> ice::WorldTraitDescription const* override;

        void register_archetypes(
            ice::ecs::ArchetypeIndex& archetype_index
        ) const noexcept override;

        bool validate_trait_list(
            ice::Span<ice::StringID const> traits
        ) const noexcept override;

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::WorldTraitDescription> _trait_descriptions;
    };

    SimpleWorldTraitArchive::SimpleWorldTraitArchive(ice::Allocator& alloc) noexcept
        : _allocator{ alloc }
        , _trait_descriptions{ alloc }
    {
    }

    void SimpleWorldTraitArchive::register_trait(
        ice::StringID_Arg name,
        ice::WorldTraitDescription description
    ) noexcept
    {
        ice::u64 const name_hash = ice::hash(name);

        ICE_ASSERT(
            ice::hashmap::has(_trait_descriptions, name_hash) == false,
            "Trait with this name already exists!"
        );

        ice::hashmap::set(
            _trait_descriptions,
            name_hash,
            description
        );
    }

    auto SimpleWorldTraitArchive::find_trait(
        ice::StringID_Arg name
    ) const noexcept -> ice::WorldTraitDescription const*
    {
        ice::u64 const name_hash = ice::hash(name);

        static ice::WorldTraitDescription invalid{ .factory = nullptr };
        ice::WorldTraitDescription const& description = ice::hashmap::get(_trait_descriptions, name_hash, invalid);
        return description.factory == nullptr ? nullptr : &description;
    }

    void SimpleWorldTraitArchive::register_archetypes(
        ice::ecs::ArchetypeIndex& archetype_index
    ) const noexcept
    {
        for (WorldTraitDescription const& trait_desc : _trait_descriptions)
        {
            for (ice::ecs::ArchetypeInfo const& arch_info : trait_desc.defined_archetypes)
            {
                archetype_index.register_archetype(arch_info);
            }
        }
    }

    bool SimpleWorldTraitArchive::validate_trait_list(
        ice::Span<ice::StringID const> traits
    ) const noexcept
    {
        bool valid = true;

        ice::u32 const count = ice::count(traits);
        ice::u32 idx = 0;

        while (valid && idx < count)
        {
            ice::Span<ice::StringID const> const list_head = ice::span::subspan(traits, 0, 1);
            ice::Span<ice::StringID const> const list_tail = ice::span::subspan(traits, 1);

            ice::WorldTraitDescription const* desc = find_trait(traits[idx]);
            valid = desc != nullptr;

            if (valid)
            {
                // Check if we have all dependencies added
                for (ice::StringID_Arg dependency : desc->required_dependencies)
                {
                    bool found = false;
                    for (ice::StringID_Arg trait : list_head)
                    {
                        found |= trait == dependency;
                    }

                    valid &= found;
                }

                // Check if non of the weak dependencies are seen later in the list
                for (ice::StringID_Arg dependency : desc->optional_dependencies)
                {
                    bool found = false;
                    for (ice::StringID_Arg trait : list_tail)
                    {
                        found |= trait == dependency;
                    }

                    valid &= !found;
                }
            }

            idx += 1;
        }

        return valid;
    }

    auto create_world_trait_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::WorldTraitArchive>
    {
        return ice::make_unique<ice::SimpleWorldTraitArchive>(alloc, alloc);
    }
#endif

} // namespace ice

namespace ice
{

    class SimpleTraitArchive : public ice::TraitArchive
    {
    public:
        SimpleTraitArchive(ice::Allocator& alloc) noexcept
            : _traits{ alloc }
        {
        }

        void register_trait(ice::TraitDescriptor descriptor) noexcept override
        {
            ice::hashmap::set(_traits, ice::hash(descriptor.name), ice::move(descriptor));
        }

        auto trait(ice::StringID_Arg name) const noexcept -> ice::TraitDescriptor const* override
        {
            return ice::hashmap::try_get(_traits, ice::hash(name));
        }

    private:
        ice::HashMap<ice::TraitDescriptor> _traits;
    };

    auto create_default_trait_archive(
        ice::Allocator& alloc
    ) noexcept -> ice::UniquePtr<ice::TraitArchive>
    {
        return ice::make_unique<ice::SimpleTraitArchive>(alloc, alloc);
    }

} // namespace ice::v2
