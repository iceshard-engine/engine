#pragma once
#include <resource/module.hxx>

#include <core/memory.hxx>
#include <core/stack_string.hxx>
#include <core/pod/collections.hxx>
#include <core/allocators/proxy_allocator.hxx>

//! \brief The filesystem namespace.
namespace resource
{


    //! \brief A resource system build upon a native file system.
    class FileSystem : public ResourceModule
    {
    public:
        FileSystem(core::allocator& alloc, std::string_view basedir) noexcept;
        ~FileSystem() noexcept;

        //! \brief Searches for the default resource with the given name.
        auto find(const URI& uri) noexcept -> Resource* override;

        //! \brief Mounts all resources found under the given URI.
        auto mount(const URI& uri, std::function<void(Resource*)> callback) noexcept -> uint32_t override;

    private:
        const std::string _basedir;

        //! \brief The resource allocator.
        core::memory::proxy_allocator _allocator;

        //! \brief A list of all mounted resources.
        core::pod::Array<Resource*> _resources;
    };


} // namespace filesystem
