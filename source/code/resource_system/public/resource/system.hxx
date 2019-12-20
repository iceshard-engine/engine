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
        void add_module(core::memory::unique_pointer<ResourceModule> module_obj, core::pod::Array<core::cexpr::stringid_type> const& schemes) noexcept;

        //! \todo documentation.
        auto find(URI const& location) noexcept -> Resource*;

        //! \todo documentation.
        auto find(URN const& name) noexcept -> Resource*;

        //! \todo documentation
        auto open(URI const& location) noexcept -> OutputResource*;

        //! \todo documentation
        auto open(URN const& name) noexcept -> OutputResource*;

        //! \todo documentation.
        auto mount(URI const& location) noexcept -> uint32_t;

        //! \todo documentation.
        auto mount(URN const& name) noexcept -> uint32_t;

        //! \todo documentation.
        auto messages() noexcept -> core::MessageBuffer const& { return _messages; }

        //! \todo documentation.
        void flush_messages() noexcept;

    private:
        void handle_module_message(core::Message const& message) noexcept;

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
        std::vector<core::memory::unique_pointer<ResourceModule>> _modules{};

        //! \brief A buffer of messages.
        core::MessageBuffer _messages;
    };

} // namespace resource
