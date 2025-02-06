/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/world/world_types.hxx>

namespace ice
{

    class Trait;
    class TraitArchive;
    class TraitContext;
    class TraitDevUI;

    struct TraitDescriptor;
    struct TraitParams;
    struct TraitTaskBinding;

    enum class TraitTaskType : ice::u8
    {
        Frame,
        Runner,
    };

    using TraitIndirectTaskFn = auto (*)(void*, ice::TraitParams const&, ice::Shard) noexcept -> ice::Task<>;

    struct TraitParams
    {
        ice::Clock const& clock;
        ice::ResourceTracker& resources;
        ice::AssetStorage& assets;
    };

    struct TraitTaskBinding
    {
        ice::TraitTaskType task_type = ice::TraitTaskType::Frame;
        ice::ShardID trigger_event;
        ice::TraitIndirectTaskFn procedure;
        void* procedure_userdata;
    };

} // namespace ice

namespace ice::detail
{

    struct TraitContextImpl;

} // namespace ice::detail
