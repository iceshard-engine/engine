/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/ecs/ecs_types.hxx>
#include <ice/engine_params.hxx>
#include <ice/world/world_types.hxx>

namespace ice
{

    class Trait;
    class TraitArchive;
    class TraitContext;
    class TraitDevUI;

    struct TraitDescriptor;
    struct TraitTaskBinding;

    enum class TraitSendMode : ice::u8
    {
        Add,
        Replace,
        Default = Add
    };

    using TraitIndirectTaskFn = auto (*)(void*, ice::EngineParamsBase const&, ice::Shard) noexcept -> ice::Task<>;

    struct TraitTaskBinding
    {
        ice::TraitTaskType task_type = ice::TraitTaskType::Logic;
        ice::ShardID trigger_event;
        ice::TraitIndirectTaskFn procedure;
        void* procedure_userdata;
    };

} // namespace ice

namespace ice::detail
{

    struct TraitContextImpl;

    template<typename R, typename T = R>
    auto map_task_arg(T value) noexcept -> R
    {
        return static_cast<R>(value);
    }

    template<typename Target>
    struct ArgMapper
    {
        using ShardType = Target;

        template<typename ContextParams, typename Source>
        static auto select(
            ContextParams const& params,
            Source&& source
        ) noexcept -> Target
        {
            return ice::detail::map_task_arg<Target, decltype(source)>(source);
        }
    };

} // namespace ice::detail

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::AssetHandle*> = ice::shard_payloadid("ice::Asset");

template<>
constexpr inline ice::ShardPayloadID ice::Constant_ShardPayloadID<ice::Resource*> = ice::shard_payloadid("ice::ResourceHandle");
