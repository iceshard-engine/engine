#pragma once
#include <resource/uri.hxx>
#include <core/string_view.hxx>
#include <core/data/view.hxx>

namespace resource
{

    //! \brief Describes a single resource which can be fetched for data.
    class Resource
    {
    public:
        virtual ~Resource() noexcept = default;

        //! \brief The resource identifier.
        //! \remarks This value can be seen as the absolute location to a specific resource.
        virtual auto location() const noexcept -> const URI& = 0;

        //! \brief Resource data.
        virtual auto data() noexcept -> core::data_view = 0;

        //! \brief Resource metadata.
        virtual auto metadata() noexcept -> core::data_view = 0;

        //! \brief String value used to create the resource name.
        //! \remarks The value should return a valid file name with or without extension.
        virtual auto name() const noexcept -> core::StringView<> = 0;
    };

    class OutputResource
    {
    public:
        virtual void write(core::data_view wdata) noexcept = 0;

        virtual void flush() noexcept = 0;
    };

} // namespace resource
