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
        FileSystem(core::allocator& alloc, core::StringView basedir) noexcept;
        ~FileSystem() noexcept;

        //! \brief Searches for the default resource with the given name.
        auto find(URI const& uri) noexcept -> Resource* override;

        //! \brief Opens a file for writing and pushes filesystem messages.
        auto open(URI const& uri, core::MessageBuffer& messages) noexcept -> OutputResource* override;

        //! \brief Mounts all resources found under the given URI.
        auto mount(URI const& uri, core::MessageBuffer& messages) noexcept -> uint32_t override;

    private:
        const core::String<> _basedir;

        //! \brief The resource allocator.
        core::memory::proxy_allocator _allocator;

        //! \brief A list of all mounted resources.
        core::pod::Array<Resource*> _resources;
    };

} // namespace resource
