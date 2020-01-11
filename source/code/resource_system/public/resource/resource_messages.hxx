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

        Resource* resource;
    };

    struct ResourceUpdated
    {
        static constexpr auto message_type = core::cexpr::stringid_cexpr("Resource.Updated");

        Resource* resource;
    };

} // namespace resource::message
