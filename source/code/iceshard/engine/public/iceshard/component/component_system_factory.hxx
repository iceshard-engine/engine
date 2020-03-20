#pragma once
#include <core/allocator.hxx>

namespace iceshard
{

    class ComponentSystem;

    enum class ComponentSystemScope
    {
        Engine,
        World,
    };

    using ComponentSystemFactory = auto(*)(core::allocator&, void* userdata) noexcept -> ComponentSystem*;

} // namespace iceshard
