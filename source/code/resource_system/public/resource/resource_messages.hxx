#pragma once
#include <core/allocator.hxx>
#include <core/string_view.hxx>
#include <core/message/operations.hxx>
#include <resource/uri.hxx>
#include <resource/resource.hxx>

namespace resource::message
{

    struct ResourceAdded
    {
        static constexpr auto message_type = core::cexpr::stringid_cexpr("Resource.Added");

        //! \brief The resource name.
        URN name;

        //! \brief The resource object.
        Resource* resource_object;

        //! \brief The resource native name.
        core::StringView<> native_name;
    };

    struct ResourceUpdated
    {
        static constexpr auto message_type = core::cexpr::stringid_cexpr("Resource.Updated");

        //! \brief The resource name.
        URN name;

        //! \brief The resource location.
        Resource* resource_object;

        //! \brief The resource origin.
        core::StringView<> origin;
    };

} // namespace resource::message
