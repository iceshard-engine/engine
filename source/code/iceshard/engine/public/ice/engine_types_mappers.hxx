#pragma once
#include <ice/engine_runner.hxx>
#include <ice/gfx/gfx_runner.hxx>
#include <ice/gfx/gfx_context.hxx>
#include <ice/gfx/gfx_shards.hxx>
#include <ice/world/world_updater.hxx>

namespace ice
{

    /// ice::WorldStateParams

    template<>
    inline auto detail::map_task_arg<ice::EngineSchedulers>(
        ice::WorldStateParams const& params
    ) noexcept -> ice::EngineSchedulers
    {
        return params.thread;
    }

    template<>
    inline auto detail::map_task_arg<ice::AssetStorage&>(
        ice::WorldStateParams const& params
    ) noexcept -> ice::AssetStorage&
    {
        return params.assets;
    }

    template<>
    inline auto detail::map_task_arg<ice::Engine&>(
        ice::WorldStateParams const& params
    ) noexcept -> ice::Engine&
    {
        return params.engine;
    }

    // ice::EngineFrameUpdate

    template<>
    inline auto detail::map_task_arg<ice::EngineSchedulers>(
        ice::EngineFrameUpdate const& params
    ) noexcept -> ice::EngineSchedulers
    {
        return params.thread;
    }

    template<>
    inline auto detail::map_task_arg<ice::DataStorage&>(
        ice::EngineFrameUpdate const& params
    ) noexcept -> ice::DataStorage&
    {
        return params.frame.data();
    }

    template<ice::concepts::NamedDataType NamedType>
    struct detail::ArgMapper<NamedType&>
    {
        static auto select(ice::EngineFrameUpdate const& params) noexcept -> NamedType&
        {
            return params.frame.data().read_or_store<NamedType>(NamedType::Identifier);
        }
    };

    // ice::gfx::GfxFrameUpdate

    template<>
    inline auto detail::map_task_arg<ice::gfx::GfxContext&>(
        ice::gfx::GfxFrameUpdate const& params
    ) noexcept -> ice::gfx::GfxContext&
    {
        return params.context;
    }

    template<>
    inline auto detail::map_task_arg<ice::render::RenderDevice&>(
        ice::gfx::GfxFrameUpdate const& params
    ) noexcept -> ice::render::RenderDevice&
    {
        return params.context.device();
    }

    template<>
    inline auto detail::map_task_arg<ice::DataStorage&>(
        ice::gfx::GfxFrameUpdate const& params
    ) noexcept -> ice::DataStorage&
    {
        return params.context.data();
    }

    template<ice::concepts::NamedDataType NamedType>
    struct detail::ArgMapper<NamedType const&>
    {
        static auto select(ice::gfx::GfxFrameUpdate const& params) noexcept -> NamedType const&
        {
            return params.frame.data().read<NamedType>(NamedType::Identifier);
        }
    };

    template<ice::concepts::NamedDataType NamedType>
    struct detail::ArgMapper<ice::Span<NamedType const>>
    {
        static auto select(ice::gfx::GfxFrameUpdate const& params) noexcept -> ice::Span<NamedType const>
        {
            return params.frame.data().read<ice::Span<NamedType const> const>(NamedType::Identifier);
        }
    };

    // ice::gfx::GfxStateChange

    template<>
    inline auto detail::map_task_arg<ice::gfx::GfxContext&>(
        ice::gfx::GfxStateChange const& params
    ) noexcept -> ice::gfx::GfxContext&
    {
        return params.context;
    }

    template<>
    inline auto detail::map_task_arg<ice::render::RenderDevice&>(
        ice::gfx::GfxStateChange const& params
    ) noexcept -> ice::render::RenderDevice&
    {
        return params.context.device();
    }

    template<>
    inline auto detail::map_task_arg<ice::DataStorage&>(
        ice::gfx::GfxStateChange const& params
    ) noexcept -> ice::DataStorage&
    {
        return params.context.data();
    }

} // namespace ice
