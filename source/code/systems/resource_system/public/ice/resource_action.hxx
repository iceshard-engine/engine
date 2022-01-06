#pragma once
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>

namespace ice
{

    class Resource_v2;
    class ResourceProvider_v2;

    enum class ResourceStatus_v2 : ice::u32;

    struct ResourceAction_v2
    {
        struct Result
        {
            ice::ResourceStatus_v2 resource_status;
            ice::Resource_v2 const* resource;
            ice::Data data;
        };

        enum class Type : ice::u32
        {
            Load,
            Unload,

            // Optional
            Reload,
            Save,
        };

        using Handler = auto(
            ice::Userdata userdata, // #TODO: Create something of a ice::Ptr and/or ice::CheckedPtr
            ice::ResourceProvider_v2& provider,
            ice::Resource_v2 const& resource,
            void* context_data // #TODO: Create something of a ice::Ptr and/or ice::CheckedPtr
        ) noexcept -> ice::Task<ResourceAction_v2::Result>;

        ice::ResourceAction_v2::Type type;
        ice::ResourceAction_v2::Handler* handler;
        ice::Userdata userdata;
    };

} // namespace ice
