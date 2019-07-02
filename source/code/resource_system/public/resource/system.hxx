#pragma once
#include <resource/uri.hxx>
#include <resource/resource.hxx>
#include <resource/module.hxx>

#include <core/pointer.hxx>
#include <core/pod/collections.hxx>
#include <core/allocators/proxy_allocator.hxx>

#include <vector>

namespace resource
{


    //! \brief Describes a resource system which is responsible for holding the state of all loaded resources.
    class ResourceSystem final
    {
    public:
        ResourceSystem(core::allocator& alloc) noexcept;
        virtual ~ResourceSystem() noexcept = default;

        //! \todo documentation.
        void add_module(core::memory::unique_pointer<ResourceModule> module_obj, const core::pod::Array<core::cexpr::stringid_type>& schemes) noexcept;

        //! \todo documentation.
        auto find(const URI& location) noexcept -> Resource*;

        //! \todo documentation.
        auto find(const URN& name) noexcept -> Resource*;

        //! \todo documentation.
        auto mount(const URI& location) noexcept -> uint32_t;

        //! \todo documentation.
        auto mount(const URN& name) noexcept -> uint32_t;

    private:
        core::memory::proxy_allocator _allocator;

        //! \brief Hash map of named default resources.
        core::pod::Hash<Resource*> _named_resources;

        //! \brief Hash map of scheme to module associations.
        core::pod::Hash<ResourceModule*> _scheme_handlers;

        //! \brief Vector of all registered modules.
        std::vector<core::memory::unique_pointer<ResourceModule>> _modules{ };
    };


} // namespace resource
