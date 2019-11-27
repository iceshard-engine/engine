#pragma once
#include <resource/uri.hxx>
#include <functional>

namespace resource
{

    //! \brief A resource object.
    class Resource;

    //! \brief This class is used to handle specific schemes in the resource system.
    //!
    //! \detail A single module can be used for more than one resource scheme.
    //!     Additionally a resource module shouldn't be accessed after it has been registered.
    //!     This is required due to ensure proper encapsulation.
    //!
    //! \remarks A module is obligated to keep all Resource objects alive
    //!     unless explicitly asked to release them.
    class ResourceModule
    {
    public:
        virtual ~ResourceModule() noexcept = default;

        //! \todo documentation.
        virtual auto find(const URI& location) noexcept -> Resource* = 0;

        //! \todo documentation.
        virtual auto open(
            [[maybe_unused]] URI const& location,
            [[maybe_unused]] std::function<void(Resource*)> callback) noexcept -> OutputResource* { return nullptr; }

        //! \todo documentation.
        virtual auto mount(const URI& location, std::function<void(Resource*)> callback) noexcept -> uint32_t = 0;
    };


} // namespace resource
