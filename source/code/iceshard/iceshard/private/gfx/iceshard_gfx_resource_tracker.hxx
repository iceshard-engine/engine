/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/container/hashmap.hxx>
#include <ice/gfx/gfx_resource_tracker.hxx>

namespace ice::gfx
{

    class IceGfxResourceTracker final : public GfxResourceTracker
    {
    public:
        IceGfxResourceTracker(
            ice::Allocator& alloc
        ) noexcept;

        ~IceGfxResourceTracker() noexcept override = default;

        void track_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResource resource
        ) noexcept override;

        auto find_resource(
            ice::StringID_Arg name,
            ice::gfx::GfxResource::Type type
        ) noexcept -> ice::gfx::GfxResource override;

    private:
        ice::HashMap<ice::gfx::GfxResource> _resources;
    };

} // namespace ice::gfx

