#pragma once
#include <ice/resource.hxx>
#include <ice/pod/array.hxx>

namespace ice
{

    enum class ResourceEvent
    {
        Added,
        Updated,
        Replaced,
        Removed,
        MountError,
    };

    struct ResourceQuery
    {
        ice::pod::Array<ice::ResourceEvent> events{ ice::memory::null_allocator() };
        ice::pod::Array<ice::Resource*> objects{ ice::memory::null_allocator() };
    };

} // namespace ice
