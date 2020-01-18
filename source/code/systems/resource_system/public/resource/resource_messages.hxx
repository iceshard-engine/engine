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
        static constexpr auto message_type = "Resource.Added"_sid;

        Resource* resource;
    };

    struct ResourceUpdated
    {
        static constexpr auto message_type = "Resource.Updated"_sid;

        Resource* resource;
    };

} // namespace resource::message
