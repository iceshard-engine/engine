/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/profiler.hxx>
#include <ice/string.hxx>
#include <ice/render/render_declarations.hxx>

namespace ice::render::detail
{

#if IPT_ENABLED

    #define IPR_COLLECT_ZONES( api, cmds ) api.profiling_collect_zones(cmds)

    #define IPR_ZONE_NAMED( api, varname, cmds, name ) static constexpr tracy::SourceLocationData TracyConcat(__tracy_gpu_source_location,TracyLine) { name, TracyFunction,  TracyFile, (uint32_t)TracyLine, 0 }; ice::render::detail::ProfilingZone varname = api.profiling_zone(cmds, &TracyConcat(__tracy_gpu_source_location,TracyLine), name)
    #define IPR_ZONE( api, cmds, name ) IPR_ZONE_NAMED( api, ___tracy_gpu_zone, cmds, name )


    class ProfilingZone final
    {
    public:
        struct Internal;
        using InternalDeleteFn = void(*)(Internal* ptr) noexcept;

        inline ProfilingZone() noexcept;
        inline ProfilingZone(Internal* internal, InternalDeleteFn fn_delete) noexcept;
        inline ~ProfilingZone() noexcept;

        inline ProfilingZone(ProfilingZone&& other) noexcept;
        inline auto operator=(ProfilingZone&& other) noexcept -> ProfilingZone&;

        inline ProfilingZone(ProfilingZone const& other) noexcept = delete;
        inline auto operator=(ProfilingZone const& other) noexcept -> ProfilingZone& = delete;

    private:
        Internal* _internal;
        InternalDeleteFn _fn_delete;
    };

    inline ProfilingZone::ProfilingZone() noexcept
        : _internal{ nullptr }
        , _fn_delete{ nullptr }
    { }

    inline ProfilingZone::ProfilingZone(Internal* internal, InternalDeleteFn fn_delete) noexcept
        : _internal{ internal }
        , _fn_delete{ fn_delete }
    { }

    inline ProfilingZone::~ProfilingZone() noexcept
    {
        if (_fn_delete)
        {
            _fn_delete(_internal);
        }
    }

    inline ProfilingZone::ProfilingZone(ProfilingZone&& other) noexcept
        : _internal{ ice::exchange(other._internal, nullptr) }
        , _fn_delete{ ice::exchange(other._fn_delete, nullptr) }
    { }

    inline auto ProfilingZone::operator=(ProfilingZone&& other) noexcept -> ProfilingZone&
    {
        if (&other != this)
        {
            if (_fn_delete)
            {
                _fn_delete(_internal);
            }

            _internal = ice::exchange(other._internal, nullptr);
            _fn_delete = ice::exchange(other._fn_delete, nullptr);
        }
        return *this;
    }

#else

    class ProfilingZone final { };

    #define IPR_COLLECT_ZONES( api, cmds )

    #define IPR_ZONE_NAMED( api, varname, cmds, name )
    #define IPR_ZONE( api, cmds, name )

#endif



} // namespace ice::render
