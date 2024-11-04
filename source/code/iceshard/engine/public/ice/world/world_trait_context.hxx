#pragma once
#include <ice/task_checkpoint.hxx>
#include <ice/world/world_trait_details.hxx>

namespace ice
{

    static constexpr ice::ErrorCode E_TraitContextNotInitialized{ "E.1100:Worlds:World trait context was not initialized!" };

    struct TraitContext
    {
        virtual ~TraitContext() noexcept = default;

        virtual void register_interface_selector(ice::InterfaceSelector* selector) noexcept = 0;

        virtual auto checkpoint(ice::StringID id) noexcept -> ice::TaskCheckpointGate = 0;
        virtual bool register_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept = 0;
        virtual void unregister_checkpoint(ice::StringID id, ice::TaskCheckpoint& checkpoint) noexcept = 0;

        virtual auto bind(ice::TraitTaskBinding const& binding) noexcept -> ice::Result = 0;

        template<auto MemberPtr> requires (ice::is_method_member_v<decltype(MemberPtr)>)
        auto bind(ice::ShardID event = ice::Shard_Invalid.id) noexcept -> ice::Result;

        template<auto MemberPtr, typename DataType> requires (ice::is_method_member_v<decltype(MemberPtr)>)
        auto bind(ice::ShardID event = ice::Shard_Invalid.id) noexcept -> ice::Result;

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

    using TraitTaskFn = auto (ice::Trait::*)(ice::Shard) noexcept -> ice::Task<>;
    using TraitIndirectTaskFn = auto (*)(ice::Trait*, ice::Shard, ice::detail::TraitTaskTracker*) noexcept -> ice::Task<>;

    struct TraitTaskBinding
    {
        ice::ShardID trigger_event;
        ice::TraitIndirectTaskFn procedure;
        ice::TraitTaskType task_type = ice::TraitTaskType::Frame;
    };

    template<auto MemberPtr> requires (ice::is_method_member_v<decltype(MemberPtr)>)
    auto TraitContext::bind(ice::ShardID event) noexcept -> ice::Result
    {
        return this->bind(
            TraitTaskBinding{
                .trigger_event = event,
                .procedure = detail::trait_method_task_wrapper<MemberPtr>,
                .task_type = ice::TraitTaskType::Frame,
            }
        );
    }

    template<auto MemberPtr, typename DataType> requires (ice::is_method_member_v<decltype(MemberPtr)>)
    auto TraitContext::bind(ice::ShardID event) noexcept -> ice::Result
    {
        return this->bind(
            TraitTaskBinding{
                .trigger_event = event,
                .procedure = detail::trait_method_task_wrapper<MemberPtr, DataType>,
                .task_type = ice::TraitTaskType::Frame,
            }
        );
    }

} // namespace ice
