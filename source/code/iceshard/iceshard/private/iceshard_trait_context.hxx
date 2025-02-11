/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/interfaces.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/world/world_trait_context.hxx>
#include <ice/shard_container.hxx>

namespace ice
{

    class IceshardWorldContext;

    class IceshardTraitContext : public ice::TraitContext, public ice::InterfaceSelector
    {
    public:
        ice::UniquePtr<ice::Trait> trait;

    public:
        using InterfaceSelector::query_interface;

        IceshardTraitContext(ice::IceshardWorldContext& world_context, ice::u32 index) noexcept;
        ~IceshardTraitContext() noexcept;

        auto world() noexcept -> ice::World& override;

        void send(ice::detail::TraitEvent event) noexcept override;
        void sync(ice::ShardContainer& out_shards) noexcept;

        auto checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate override;
        bool register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept override;
        void unregister_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept override;

        auto bind(ice::TraitTaskBinding const& binding) noexcept -> ice::Result override;

        void register_interface_selector(ice::InterfaceSelector* selector) noexcept override;

    public: // ice::InterfaceSelector
        auto query_interface(ice::StringID_Arg id) noexcept -> ice::Expected<void*> override;

    private:
        ice::u32 const _trait_index;
        ice::IceshardWorldContext& _world_context;
        ice::InterfaceSelector* _interface_selector;

        ice::Array<ice::detail::TraitEvent> _events;
        ice::Array<ice::detail::TraitEvent> _events_expired;
    };

} // namespace ice
