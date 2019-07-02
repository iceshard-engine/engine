#pragma once
#include <resource/uri.hxx>
#include <core/string.hxx>
#include <core/data/view.hxx>

namespace resource
{


    //! \brief Describes a single resource which can be fetched for data.
    class Resource
    {
    public:
        virtual ~Resource() noexcept = default;

        //! \brief The resource identifier.
        //! \remark This value can be seen as the absolute location to a specific resource.
        virtual auto location() const noexcept -> const URI& = 0;

        //! \brief Returns the associated resource data.
        virtual auto data() noexcept->core::data_view = 0;
    };


} // namespace resource
