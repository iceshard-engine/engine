/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/gfx/gfx_object_storage.hxx>
#include <ice/container/hashmap.hxx>

namespace ice::gfx::v2
{

    class SimpleGfxObjectStorage : public GfxObjectStorage
    {
    public:
        SimpleGfxObjectStorage(ice::Allocator& alloc) noexcept;

        bool set(ice::StringID_Arg name, ice::gfx::v2::GfxObject object) noexcept override;
        auto get(ice::StringID_Arg name) const noexcept -> ice::gfx::v2::GfxObject override;
        void destroy_all(ice::render::RenderDevice& device) noexcept;

    private:
        ice::HashMap<ice::gfx::v2::GfxObject> _objects;
    };

} // namespace ice::gfx::v2
