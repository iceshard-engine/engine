#pragma once
#include <core/allocator.hxx>
#include <core/string_view.hxx>
#include <core/message/operations.hxx>
#include <resource/uri.hxx>
#include <resource/resource.hxx>

namespace resource::message
{

    struct ModuleResourceMounted
    {
        static constexpr auto message_type = "Module.Resource.Added"_sid;

        //! \brief The resource object.
        Resource* resource_object;
    };

    struct ModuleResourceUpdated
    {
        static constexpr auto message_type = "Module.Resource.Updated"_sid;

        //! \brief The resource location.
        Resource* resource_object;
    };

} // namespace resource::message
