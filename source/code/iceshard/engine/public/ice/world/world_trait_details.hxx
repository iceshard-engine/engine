/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/world/world_trait_types.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/assert.hxx>
#include <ice/task.hxx>

namespace ice::detail
{

    template<typename Trait>
    inline auto default_trait_factory(
        ice::Allocator& alloc,
        ice::TraitContext& context,
        void* userdata
    ) noexcept -> ice::UniquePtr<ice::Trait>
    {
        return ice::make_unique<Trait>(alloc, alloc, context);
    }

    struct TraitTaskTracker
    {
        virtual ~TraitTaskTracker() noexcept = default;
        virtual auto report_resume(ice::u32 id) noexcept -> ice::u32 = 0;
        virtual auto report_suspend(ice::u32 id) noexcept -> ice::u32 = 0;
        virtual void report_finish(ice::u32 id) noexcept = 0;
    };

    struct TraitEvent : ice::Shard
    {
        using FnOnExpire = void(*)(void*, ice::Shard) noexcept;
        FnOnExpire fn_on_expire;
        void* ud_on_expire;
        ice::TraitSendMode mode;
    };

    template<typename ParamsType, typename DataType, typename Class, typename... Args>
    static auto trait_method_task_wrapper(
        ice::Task<>(Class::*Method)(Args...) noexcept,
        void* userdata,
        ParamsType const& trait_params,
        ice::Shard shard
    ) noexcept -> ice::Task<>
    {
        using ArgList = std::tuple<Args..., void>;
        using Arg0 = typename std::tuple_element_t<0, ArgList>;
        using ShardType = typename ArgMapper<DataType>::ShardType;

        if constexpr (sizeof...(Args) == 0)
        {
            co_await(static_cast<Class*>(userdata)->*Method)();
        }
        else
        {
            if constexpr (std::is_reference_v<ShardType> && ice::HasShardPayloadID<ice::clear_type_t<ShardType>*> == false)
            {
                co_await (reinterpret_cast<Class*>(userdata)->*Method)(
                    ice::detail::ArgMapper<Args>::select(trait_params, trait_params)...
                );
            }
            else if constexpr (std::is_reference_v<DataType>)
            {
                using ArgType = ice::clear_type_t<ShardType>;
                ICE_ASSERT(
                    ice::Constant_ShardPayloadID<ArgType*> == shard.id.payload,
                    "Shard payload ID incompatible with the argument. {} != {}",
                    ice::Constant_ShardPayloadID<ArgType*>.value, shard.id.payload.value
                );

                ArgType* shard_value = nullptr;
                bool const valid_value = ice::shard_inspect(shard, shard_value);
                ICE_ASSERT(valid_value, "Invalid value stored in Shard!");

                co_await (reinterpret_cast<Class*>(userdata)->*Method)(
                    ice::detail::ArgMapper<Args>::select(trait_params, *shard_value)...
                );
            }
            else
            {
                using ArgType = ShardType;
                ICE_ASSERT(
                    ice::Constant_ShardPayloadID<ArgType> == shard.id.payload,
                    "Shard payload ID incompatible with the argument. {} != {}",
                    ice::Constant_ShardPayloadID<ArgType>.value, shard.id.payload.value
                );

                ArgType shard_value{ };
                bool const valid_value = ice::shard_inspect(shard, shard_value);
                ICE_ASSERT(valid_value, "Invalid value stored in Shard!");

                co_await(static_cast<Class*>(userdata)->*Method)(
                    ice::detail::ArgMapper<Args>::select(trait_params, shard_value)...
                );
            }
        }

        co_return;
    }

    template<auto Method, typename ParamsType, typename DataType>
    static auto trait_method_task(
        void* userdata,
        ice::EngineParamsBase const& params,
        ice::Shard shard
    ) noexcept -> ice::Task<>
    {
        ParamsType const& task_params = static_cast<ParamsType const&>(params);
        co_await trait_method_task_wrapper<ParamsType, DataType>(Method, userdata, task_params, shard);
    }

    template<auto MemberPtr, ice::TraitTaskType TaskType, typename DataType>
    auto trait_task_binding(
        ice::ShardID event,
        void* userdata
    ) noexcept -> ice::TraitTaskBinding
    {
        ice::TraitTaskBinding result{
            .task_type = TaskType,
            .trigger_event = event,
            .procedure_userdata = userdata,
        };
        if constexpr (TaskType == TraitTaskType::Logic)
        {
            result.procedure = ice::detail::trait_method_task<MemberPtr, LogicTaskParams, DataType>;
        }
        else if constexpr (TaskType == TraitTaskType::Graphics)
        {
            result.procedure = ice::detail::trait_method_task<MemberPtr, GfxTaskParams, DataType>;
        }
        else if constexpr (TaskType == TraitTaskType::Render)
        {
            result.procedure = ice::detail::trait_method_task<MemberPtr, RenderTaskParams, DataType>;
        }
        else
        {
            ICE_ASSERT(false, "Unrecognized task type!");
        }
        return result;
    }

} // namespace ice::detail
