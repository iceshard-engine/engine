/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/ecs/ecs_types.hxx>

namespace ice::ecs::detail
{

    // Callback that can provide data up to 2 components
    using EntityDestructorCallback = void(*)(void* ctx, ice::u32, ice::ecs::Entity const*, void* c0, void* c1) noexcept;

    struct EntityDestructor
    {
        void* userdata;
        ice::StringID identifier;
        ice::StringID components[2]{ ice::StringID_Invalid, ice::StringID_Invalid };
        ice::ecs::detail::EntityDestructorCallback fn;
    };

} // namespace ice::ecs::detail
