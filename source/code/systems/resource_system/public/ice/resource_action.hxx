#pragma once
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>

namespace ice
{

    class Resource_v2;
    class ResourceProvider_v2;
    enum class ResourceStatus_v2 : ice::u32;

    struct ResourceActionResult
    {
        ice::ResourceStatus_v2 resource_status;
        ice::Resource_v2 const* resource;
        ice::Data data;
    };

} // namespace ice
