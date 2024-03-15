/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "iceshard_world.hxx"

namespace ice
{

    IceshardWorld::IceshardWorld(
        ice::Allocator& alloc,
        ice::StringID_Arg worldid,
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
    ) noexcept
        : worldID{ worldid }
        , _tasks_launcher{ alloc }
        , _traits{ ice::move(traits) }
    {
        for (auto& trait : _traits)
        {
            auto launcher = _tasks_launcher.trait_launcher(trait.get());
            trait->gather_tasks(launcher);
        }
    }

    auto IceshardWorld::activate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
        for (auto& trait : _traits)
        {
            co_await trait->activate(update);
        }
        co_return;
    }

    auto IceshardWorld::deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<>
    {
        for (auto& trait : _traits)
        {
            co_await trait->deactivate(update);
        }
        co_return;
    }

} // namespace ice
