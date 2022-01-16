#pragma once
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>

namespace ice
{

    class ResourceProvider_v2;

    enum class ResourceStatus_v2 : ice::u32;

    struct Resource_v2;
    struct ResourceHandle;

    struct ResourceActionResult
    {
        ice::ResourceStatus_v2 resource_status;
        ice::Resource_v2 const* resource;
        ice::Data data;
    };

} // namespace ice
