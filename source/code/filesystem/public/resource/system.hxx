#pragma once
#include <resource/uri.hxx>
#include <resource/resource.hxx>

namespace resource
{


//! \brief Describes a resource system which is responsible for holding the state of all loaded resources.
class ResourceSystem
{
public:
    auto find(const URI& uri) noexcept -> Resource*;
    auto find(const URN& urn) noexcept -> Resource*;

private:
};


//! \brief Returns the default resource system object, which does not contain any resources.
auto default_resource_system() noexcept -> ResourceSystem&;

//! \brief Returns the current resource system.
auto get_system() noexcept -> ResourceSystem&;

//! \brief Selects the given resource system as the primary one.
void set_system(ResourceSystem& system) noexcept;


} // namespace resource
