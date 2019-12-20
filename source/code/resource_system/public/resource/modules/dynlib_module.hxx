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
    class DynLibSystem : public ResourceModule
    {
    public:
        DynLibSystem(core::allocator& alloc) noexcept;
        ~DynLibSystem() noexcept;

        //! \brief Searches for the default resource with the given name.
        auto find(URI const& uri) noexcept -> Resource* override;

        //! \brief Mounts all resources found under the given URI.
        auto mount(URI const& uri, core::MessageBuffer& messages) noexcept -> uint32_t override;

    private:
        const std::string _app_dir;
        const std::string _working_dir;

        //! \brief The resource allocator.
        core::memory::proxy_allocator _allocator;

        //! \brief A list of all mounted found dynamic libraries.
        core::pod::Array<Resource*> _resources;

        //! \brief Initial mount finished.
        bool initial_mount_finished{ false };
    };

} // namespace resource
