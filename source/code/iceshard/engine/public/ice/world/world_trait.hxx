/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task.hxx>
#include <ice/shard_container.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/stringid.hxx>
#include <ice/span.hxx>
#include <ice/engine_types.hxx>
#include <ice/world/world_trait_context.hxx>

namespace ice
{

    struct Trait
    {
        ice::TraitContext context;

        virtual ~Trait() noexcept = default;

        virtual void gather_tasks(ice::TraitTaskRegistry& task_launcher) noexcept { }

        virtual auto activate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> { co_return; }
        virtual auto deactivate(ice::WorldStateParams const& world_update) noexcept -> ice::Task<> { co_return; }
    };

    struct TraitTaskRegistry
    {
        virtual ~TraitTaskRegistry() noexcept = default;

        virtual void bind(ice::TraitTaskBinding const& binding) noexcept = 0;

        template<auto MemberPtr> requires (ice::is_method_member_v<decltype(MemberPtr)>)
        void bind(ice::ShardID event = ice::Shard_Invalid.id) noexcept;
    };

    template<auto MemberPtr> requires (ice::is_method_member_v<decltype(MemberPtr)>)
    void TraitTaskRegistry::bind(ice::ShardID event) noexcept
    {
        this->bind(TraitTaskBinding{
            .trigger_event = event,
            .procedure = detail::trait_method_task_wrapper<MemberPtr>,
            .task_type = ice::TraitTaskType::Frame,
            .procedure_userdata = nullptr,
        });
    }


    static constexpr ice::Shard Shard_TraitSetup = "event/trait/setup`void"_shard;
    static constexpr ice::Shard Shard_TraitActivate = "event/trait/activate`void"_shard;
    static constexpr ice::Shard Shard_TraitUpdate = "event/trait/update`void"_shard;
    static constexpr ice::Shard Shard_TraitDeactivate = "event/trait/deactivate`void"_shard;
    static constexpr ice::Shard Shard_TraitShutdown = "event/trait/shutdown`void"_shard;

    static constexpr ice::StringID TraitID_GfxImageStorage = "trait.gfx-image-storage"_sid;
    static constexpr ice::StringID TraitID_GfxShaderStorage = "trait.gfx-shader-storage"_sid;

} // namespace ice
