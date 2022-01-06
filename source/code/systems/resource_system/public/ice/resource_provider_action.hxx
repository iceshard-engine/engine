#pragma once
#include <ice/data.hxx>
#include <ice/task.hxx>
#include <ice/userdata.hxx>

namespace ice
{

    class Resource_v2;
    class ResourceProvider_v2;

    struct ResourceProviderAction_v2
    {
        enum class Result : ice::u32
        {
            Success,
            Failure,
            Skipped
        };

        enum class Type : ice::u32
        {
            Refresh,

            // Optional
            Reset
        };

        using Handler = auto(
            ice::Userdata userdata, // #TODO: Create something of a ice::Ptr and/or ice::CheckedPtr
            ice::ResourceProvider_v2& provider
        ) noexcept -> ice::Task<ResourceProviderAction_v2::Result>;

        ice::ResourceProviderAction_v2::Type type;
        ice::ResourceProviderAction_v2::Handler* handler;
        ice::Userdata userdata;
    };

} // namespace ice
