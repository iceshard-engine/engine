/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/world/world_trait_archive.hxx>
#include <ice/world/world_trait.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/assert.hxx>

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

} // namespace ice
