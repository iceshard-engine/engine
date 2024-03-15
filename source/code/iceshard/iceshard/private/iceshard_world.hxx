/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world.hxx>
#include "iceshard_world_tasks_launcher.hxx"

namespace ice
{

    class IceshardWorld final : public ice::World
    {
    public:
        IceshardWorld(
            ice::Allocator& alloc,
            ice::StringID_Arg worldid,
            ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> traits
        ) noexcept;

        auto trait(ice::StringID_Arg trait_identifier) noexcept -> ice::Trait* override { return nullptr; }
        auto trait(ice::StringID_Arg trait_identifier) const noexcept -> ice::Trait const* override { return nullptr; }
        auto trait_storage(ice::Trait* trait) noexcept -> ice::DataStorage* override { return nullptr; }
        auto trait_storage(ice::Trait const* trait) const noexcept -> ice::DataStorage const* override { return nullptr; }

        auto task_launcher() noexcept -> ice::IceshardTasksLauncher& { return _tasks_launcher; }

        auto activate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;
        auto deactivate(ice::WorldStateParams const& update) noexcept -> ice::Task<> override;

        ice::StringID const worldID;
    private:
        ice::IceshardTasksLauncher _tasks_launcher;
        ice::Array<ice::UniquePtr<ice::Trait>, ice::ContainerLogic::Complex> _traits;
    };

} // namespace ice
