#pragma once
#include <resource/uri.hxx>
#include <resource/resource.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/pod/collections.hxx>

namespace resource
{


//! \brief Describes a resource system which is responsible for holding the state of all loaded resources.
class ResourceSystem
{
public:
    virtual ~ResourceSystem() noexcept = default;

    //! \brief Searches for the specific resource.
    virtual auto find(const URI& uri) noexcept -> Resource* = 0;

    //! \brief Searches for the default resource with the given name.
    virtual auto find(const URN& urn) noexcept -> Resource* = 0;

    //! \brief Mounts all resources found under the given URI.
    virtual auto mount(const URI& uri) noexcept -> uint32_t = 0;
};


//! \brief Returns the default resource system object, which does not contain any resources.
auto default_resource_system() noexcept -> ResourceSystem&;

//! \brief Returns the current resource system.
auto get_system() noexcept -> ResourceSystem&;

//! \brief Selects the given resource system as the primary one.
void set_system(ResourceSystem& system) noexcept;


} // namespace resource
