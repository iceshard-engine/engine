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
        SimpleTraitArchive(
            ice::Allocator& alloc,
            ice::EngineStateTracker& states
        ) noexcept
            : _allocator{ alloc }
            , _traits{ alloc }
            , _states{ states }
        {
        }

        void register_trait(ice::TraitDescriptor const& descriptor) noexcept override
        {
            bool const can_register = descriptor.fn_register == nullptr
                || descriptor.fn_register(_allocator, _states);

            ICE_LOG_IF(
                can_register == false, LogSeverity::Warning, LogTag::Engine,
                "Register function for trait {} returned unsuccessful.", descriptor.name
            );
            if (can_register)
            {
                ice::hashmap::set(_traits, ice::hash(descriptor.name), ice::move(descriptor));
            }
        }

        auto trait(ice::StringID_Arg name) const noexcept -> ice::TraitDescriptor const* override
        {
            return ice::hashmap::try_get(_traits, ice::hash(name));
        }

    private:
        ice::Allocator& _allocator;
        ice::HashMap<ice::TraitDescriptor> _traits;
        ice::EngineStateTracker& _states;
    };

    auto create_default_trait_archive(
        ice::Allocator& alloc,
        ice::EngineStateTracker& states
    ) noexcept -> ice::UniquePtr<ice::TraitArchive>
    {
        return ice::make_unique<ice::SimpleTraitArchive>(alloc, alloc, states);
    }

} // namespace ice
