#include <ice/world/world_trait_archive.hxx>
#include <ice/ecs/ecs_archetype_index.hxx>
#include <ice/pod/hash.hxx>
#include <ice/assert.hxx>

namespace ice
{

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
        ) const noexcept;

    private:
        ice::Allocator& _allocator;
        ice::pod::Hash<ice::WorldTraitDescription> _trait_descriptions;
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
            ice::pod::hash::has(_trait_descriptions, name_hash) == false,
            "Trait with this name already exists!"
        );

        ice::pod::hash::set(
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
        ice::WorldTraitDescription const& description = ice::pod::hash::get(_trait_descriptions, name_hash, invalid);
        return description.factory == nullptr ? nullptr : &description;
    }

    void SimpleWorldTraitArchive::register_archetypes(
        ice::ecs::ArchetypeIndex& archetype_index
    ) const noexcept
    {
        for (auto const& entry : _trait_descriptions)
        {
            ice::WorldTraitDescription const& trait_desc = entry.value;
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

        ice::u32 const count = ice::size(traits);
        ice::u32 idx = 0;

        while (valid && idx < count)
        {
            ice::Span<ice::StringID const> const list_head{ traits.data(), idx };
            ice::Span<ice::StringID const> const list_tail{ traits.data() + (idx + 1), count - (idx + 1) };

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
        return ice::make_unique<ice::WorldTraitArchive, ice::SimpleWorldTraitArchive>(alloc, alloc);
    }

} // namespace ice
