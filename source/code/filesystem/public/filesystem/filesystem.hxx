#pragma once
#include <resource/system.hxx>
#include <core/memory.hxx>
#include <core/stack_string.hxx>

//! \brief The filesystem namespace.
namespace resource
{


//! \brief A resource system build upon a native file system.
class FileSystem : public ResourceSystem
{
public:
    FileSystem(core::allocator& alloc, std::string_view basedir) noexcept;
    ~FileSystem() noexcept;

    //! \brief Searches for the specific resource.
    auto find(const URN& urn) noexcept->Resource* override;

    //! \brief Searches for the default resource with the given name.
    auto find(const URI& uri) noexcept->Resource* override;

    //! \brief Mounts all resources found under the given URI.
    auto mount(const URI& uri) noexcept->uint32_t override;

private:
    const std::string _basedir;

    //! \brief The resource allocator.
    core::memory::proxy_allocator _allocator;

    //! \brief A list of all mounted resources.
    core::pod::Array<Resource*> _resources;

    //! \brief A map of default mounted resources.
    core::pod::Hash<Resource*> _default_resources;
};


} // namespace filesystem
