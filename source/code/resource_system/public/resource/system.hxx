#pragma once
#include <core/pointer.hxx>
#include <core/pod/collections.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/message/buffer.hxx>

#include <resource/uri.hxx>
#include <resource/resource.hxx>
#include <resource/module.hxx>
#include <resource/resource_messages.hxx>

#include <vector>

namespace resource
{


    //! \brief Describes a resource system which is responsible for holding the state of all loaded resources.
    class ResourceSystem final
    {
    public:
        ResourceSystem(core::allocator& alloc) noexcept;
        virtual ~ResourceSystem() noexcept;

        //! \todo documentation.
        void add_module(core::memory::unique_pointer<ResourceModule> module_obj, const core::pod::Array<core::cexpr::stringid_type>& schemes) noexcept;

        //! \todo documentation.
        auto find(const URI& location) noexcept -> Resource*;

        //! \todo documentation.
        auto find(const URN& name) noexcept -> Resource*;

        //! \todo documentation
        auto open(const URI& location) noexcept -> OutputResource*;

        //! \todo documentation
        auto open(const URN& name) noexcept -> OutputResource*;

        //! \todo documentation.
        auto mount(const URI& location) noexcept -> uint32_t;

        //! \todo documentation.
        auto mount(const URN& name) noexcept -> uint32_t;

        //! \todo documentation.
        auto messages() noexcept -> core::MessageBuffer const& { return _messages; }

        void flush_messages() noexcept;

        void update_resources() noexcept;

    private:
        core::memory::proxy_allocator _allocator;

        //! \brief Internal resources.
        core::pod::Array<Resource*> _internal_resources;

        //! \brief Hash map of named default resources.
        core::pod::Hash<Resource*> _named_resources;
        core::pod::Hash<OutputResource*> _named_output_resources;

        //! \brief Hash map of scheme to module associations.
        core::pod::Hash<ResourceModule*> _scheme_handlers;

        //! \brief Vector of all registered modules.
        std::vector<core::memory::unique_pointer<ResourceModule>> _modules{ };

        //! \brief A buffer of messages.
        core::MessageBuffer _messages;
    };


} // namespace resource
