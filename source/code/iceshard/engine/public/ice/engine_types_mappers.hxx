/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/engine_params.hxx>
#include <ice/engine_runner.hxx>
#include <ice/engine_frame.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_shards.hxx>
#include <ice/world/world_trait_types.hxx>

namespace ice
{

    struct LogicTaskParams;
    struct GfxTaskParams;
    struct RenderTaskParams;

} // namespace ice

// TraitParams based objects.

template<>
struct ice::detail::ArgMapper<ice::Clock const&>
{
    using ShardType = ice::Clock const&;

    template<typename Source>
    static auto select(ice::LogicTaskParams const& params, Source&&) noexcept -> ice::Clock const&
    {
        return params.clock;
    }
    template<typename Source>
    static auto select(ice::GfxTaskParams const& params, Source&&) noexcept -> ice::Clock const&
    {
        return params.clock;
    }
    template<typename Source>
    static auto select(ice::RenderTaskParams const& params, Source&&) noexcept -> ice::Clock const&
    {
        return params.clock;
    }
};

template<>
struct ice::detail::ArgMapper<ice::ResourceTracker&>
{
    using ShardType = ice::ResourceTracker&;

    template<typename Source>
    static auto select(ice::LogicTaskParams const& params, Source&&) noexcept -> ice::ResourceTracker&
    {
        return params.resources;
    }
    template<typename Source>
    static auto select(ice::GfxTaskParams const& params, Source&&) noexcept -> ice::ResourceTracker&
    {
        return params.resources;
    }
    template<typename Source>
    static auto select(ice::RenderTaskParams const& params, Source&&) noexcept -> ice::ResourceTracker&
    {
        return params.resources;
    }
};

template<>
struct ice::detail::ArgMapper<ice::AssetStorage&>
{
    using ShardType = ice::AssetStorage&;

    template<typename Source>
    static auto select(ice::LogicTaskParams const& params, Source&&) noexcept -> ice::AssetStorage&
    {
        return params.assets;
    }
    template<typename Source>
    static auto select(ice::GfxTaskParams const& params, Source&&) noexcept -> ice::AssetStorage&
    {
        return params.assets;
    }
    template<typename Source>
    static auto select(ice::RenderTaskParams const& params, Source&&) noexcept -> ice::AssetStorage&
    {
        return params.assets;
    }
};

template<>
struct ice::detail::ArgMapper<ice::TaskScheduler>
{
    using ShardType = ice::TaskScheduler;

    template<typename Source>
    static auto select(ice::LogicTaskParams const& params, Source&&) noexcept -> ice::TaskScheduler
    {
        return params.scheduler;
    }
    template<typename Source>
    static auto select(ice::GfxTaskParams const& params, Source&&) noexcept -> ice::TaskScheduler
    {
        return params.scheduler;
    }
    template<typename Source>
    static auto select(ice::RenderTaskParams const& params, Source&&) noexcept -> ice::TaskScheduler
    {
        return params.scheduler;
    }
};

template<>
struct ice::detail::ArgMapper<ice::gfx::GfxContext&>
{
    using ShardType = ice::gfx::GfxContext&;

    template<typename Source>
    static auto select(ice::GfxTaskParams const& params, Source&&) noexcept -> ice::gfx::GfxContext&
    {
        return params.gfx;
    }
};

// Handling of custom types in trait methods.

template<>
struct ice::detail::ArgMapper<ice::Asset>
{
    using ShardType = ice::AssetHandle*;

    template<typename ParamsType>
    static auto select(ParamsType const&, ice::AssetHandle* handle) noexcept -> ice::Asset { return ice::Asset{ handle }; }
};

template<>
struct ice::detail::ArgMapper<ice::ResourceHandle>
{
    using ShardType = ice::Resource*;

    template<typename ParamsType>
    static auto select(ParamsType const&, ice::Resource* handle) noexcept -> ice::ResourceHandle { return ice::ResourceHandle{ handle }; }
};

template<>
struct ice::detail::ArgMapper<ice::URI>
{
    using ShardType = char const*;

    template<typename ParamsType>
    static auto select(ParamsType const&, char const* uri_raw) noexcept -> ice::URI
    {
        return ice::URI{ uri_raw };
    }
};

template<>
struct ice::detail::ArgMapper<ice::String>
{
    using ShardType = char const*;

    template<typename ParamsType>
    static auto select(ParamsType const&, char const* cstr) noexcept -> ice::String
    {
        return ice::String{ cstr };
    }
};

template<ice::concepts::NamedDataType NamedType>
struct ice::detail::ArgMapper<NamedType const&>
{
    template<typename ParamsType>
    static auto select(ParamsType const&, ice::gfx::GfxFrameUpdate const& params) noexcept -> NamedType const&
    {
        return params.frame.data().read<NamedType>(NamedType::Identifier);
    }
};

template<ice::concepts::NamedDataType NamedType>
struct ice::detail::ArgMapper<ice::Span<NamedType const>>
{
    template<typename ParamsType>
    static auto select(ParamsType const&, ice::gfx::GfxFrameUpdate const& params) noexcept -> ice::Span<NamedType const>
    {
        return params.frame.data().read<ice::Span<NamedType const> const>(NamedType::Identifier);
    }
};

template<>
inline auto ice::detail::map_task_arg<ice::EngineSchedulers>(
    ice::WorldStateParams const& params
) noexcept -> ice::EngineSchedulers
{
    return params.thread;
}

template<>
inline auto ice::detail::map_task_arg<ice::Engine&>(
    ice::WorldStateParams const& params
) noexcept -> ice::Engine&
{
    return params.engine;
}

// ice::EngineFrameUpdate

template<>
inline auto ice::detail::map_task_arg<ice::EngineSchedulers>(
    ice::EngineFrameUpdate const& params
) noexcept -> ice::EngineSchedulers
{
    return params.thread;
}

template<>
inline auto ice::detail::map_task_arg<ice::DataStorage&>(
    ice::EngineFrameUpdate const& params
) noexcept -> ice::DataStorage&
{
    return params.frame.data();
}

template<ice::concepts::NamedDataType NamedType>
struct ice::detail::ArgMapper<NamedType&>
{
    static auto select(ice::EngineFrameUpdate const& params) noexcept -> NamedType&
    {
        return params.frame.data().read_or_store<NamedType>(NamedType::Identifier);
    }
};

// ice::gfx::GfxFrameUpdate

template<>
inline auto ice::detail::map_task_arg<ice::gfx::GfxContext&>(
    ice::gfx::GfxFrameUpdate const& params
) noexcept -> ice::gfx::GfxContext&
{
    return params.context;
}

template<>
inline auto ice::detail::map_task_arg<ice::gfx::GfxFrameStages&>(
    ice::gfx::GfxFrameUpdate const& params
) noexcept -> ice::gfx::GfxFrameStages&
{
    return params.stages;
}

template<>
inline auto ice::detail::map_task_arg<ice::render::RenderDevice&>(
    ice::gfx::GfxFrameUpdate const& params
) noexcept -> ice::render::RenderDevice&
{
    return params.context.device();
}

template<>
inline auto ice::detail::map_task_arg<ice::DataStorage&>(
    ice::gfx::GfxFrameUpdate const& params
) noexcept -> ice::DataStorage&
{
    return params.context.data();
}

// ice::gfx::GfxStateChange

template<>
inline auto ice::detail::map_task_arg<ice::gfx::GfxContext&>(
    ice::gfx::GfxStateChange const& params
) noexcept -> ice::gfx::GfxContext&
{
    return params.context;
}

template<>
inline auto ice::detail::map_task_arg<ice::render::RenderDevice&>(
    ice::gfx::GfxStateChange const& params
) noexcept -> ice::render::RenderDevice&
{
    return params.context.device();
}

template<>
inline auto ice::detail::map_task_arg<ice::DataStorage&>(
    ice::gfx::GfxStateChange const& params
) noexcept -> ice::DataStorage&
{
    return params.context.data();
}
