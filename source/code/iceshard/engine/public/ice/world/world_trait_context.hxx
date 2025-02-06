/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait_details.hxx>
#include <ice/task_checkpoint.hxx>
#include <ice/task.hxx>

namespace ice
{

    static constexpr ice::ErrorCode E_TraitContextNotInitialized{ "E.1100:Worlds:World trait context was not initialized!" };

    class TraitContext
    {
    public:
        virtual ~TraitContext() noexcept = default;

        virtual void register_interface_selector(ice::InterfaceSelector* selector) noexcept = 0;

        virtual auto checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate = 0;
        virtual bool register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept = 0;
        virtual void unregister_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept = 0;

        virtual auto bind(ice::TraitTaskBinding const& binding) noexcept -> ice::Result = 0;

        template<auto MemberPtr>
        auto bind(ice::ShardID event = ice::Shard_Invalid, void* userdata = nullptr) noexcept -> ice::Result;

        template<auto MemberPtr, typename DataType>
        auto bind(ice::ShardID event = ice::Shard_Invalid, void* userdata = nullptr) noexcept -> ice::Result;

        template<typename T, typename... Args>
        auto make_unique(ice::Allocator& alloc, Args&&... args) noexcept -> ice::UniquePtr<T>
        {
            ice::UniquePtr<T> result = ice::make_unique<T>(alloc, *this, ice::forward<Args>(args)...);
            if constexpr (std::derived_from<T, ice::InterfaceSelector>)
            {
                this->register_interface_selector(result.get());
            }
            return result;
        }
    };

    template<auto MemberPtr>
    auto TraitContext::bind(ice::ShardID event, void* userdata) noexcept -> ice::Result
    {
        using MemberType = decltype(MemberPtr);

        if constexpr (ice::member_info<MemberType>::argument_count > 0)
        {
            return this->bind(
                TraitTaskBinding{
                    .trigger_event = event,
                    .procedure = ice::detail::trait_method_task<MemberPtr, ice::member_arg_type_t<MemberType, 0>>,
                    .procedure_userdata = userdata,
                }
            );
        }
        else
        {
            return this->bind(
                TraitTaskBinding{
                    .trigger_event = event,
                    .procedure = ice::detail::trait_method_task<MemberPtr, void>,
                    .procedure_userdata = userdata,
                }
            );
        }
    }

    template<auto MemberPtr, typename DataType>
    auto TraitContext::bind(ice::ShardID event, void* userdata) noexcept -> ice::Result
    {
        return this->bind(
            TraitTaskBinding{
                .trigger_event = event,
                .procedure = ice::detail::trait_method_task<MemberPtr, DataType>,
                .procedure_userdata = userdata,
            }
        );
    }

} // namespace ice
