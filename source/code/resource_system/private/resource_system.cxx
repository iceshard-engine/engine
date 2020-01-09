#include <resource/resource_system.hxx>
#include <resource/resource_messages.hxx>
#include "modules/module_messages.hxx"

#include <core/cexpr/stringid.hxx>

#include <core/memory.hxx>
#include <core/pod/hash.hxx>

namespace resource
{
    namespace hash = core::pod::hash;

    namespace detail
    {
        class StdFileResource : public Resource, public OutputResource
        {
        public:
            StdFileResource(URI location, FILE* handle) noexcept
                : _location{ std::move(location) }
                , _handle{ handle }
            {
            }

            auto location() const noexcept -> const URI& override
            {
                return _location;
            }

            auto data() noexcept -> core::data_view override
            {
                return core::data_view{ nullptr, 0 };
            }

            auto metadata() noexcept -> core::data_view override
            {
                return core::data_view{ nullptr, 0 };
            }

            auto name() const noexcept -> core::StringView override
            {
                return { _location.path };
            }

            void write(core::data_view wdata) noexcept override
            {
                fwrite(wdata.data(), sizeof(char), wdata.size(), _handle);
            }

            void flush() noexcept override
            {
                fflush(_handle);
            }

        private:
            FILE* _handle;

            URI _location;
        };

    } // namespace detail

    ResourceSystem::ResourceSystem(core::allocator& alloc) noexcept
        : _allocator{ "resource-system", alloc }
        , _internal_resources{ alloc }
        , _named_resources{ alloc }
        , _named_output_resources{ alloc }
        , _scheme_handlers{ alloc }
        , _messages{ alloc }
    {
        hash::reserve(_named_resources, 200);
        hash::reserve(_scheme_handlers, 10);

        core::pod::array::push_back(
            _internal_resources,
            static_cast<resource::Resource*>(_allocator.make<detail::StdFileResource>(URI{ core::cexpr::stringid("internal"), "<stderr>" }, stderr)));
        core::pod::array::push_back(
            _internal_resources,
            static_cast<resource::Resource*>(_allocator.make<detail::StdFileResource>(URI{ core::cexpr::stringid("internal"), "<stdout>" }, stdout)));

        for (auto* res : _internal_resources)
        {
            const auto& name = resource::get_name(res->location());
            auto name_hash = static_cast<uint64_t>(name.name.hash_value);

            core::pod::hash::set(_named_output_resources, name_hash, static_cast<OutputResource*>(static_cast<detail::StdFileResource*>(res)));
        }
    }

    ResourceSystem::~ResourceSystem() noexcept
    {
        core::pod::hash::clear(_named_resources);
        core::pod::hash::clear(_scheme_handlers);

        _modules.clear();

        for (auto* res : _internal_resources)
        {
            _allocator.destroy(res);
        }

        core::pod::array::clear(_internal_resources);
    }

    void ResourceSystem::add_module(core::memory::unique_pointer<ResourceModule> module_ptr, const core::pod::Array<core::cexpr::stringid_type>& schemes) noexcept
    {
        auto* module_object = module_ptr.get();
        for (auto& scheme : schemes)
        {
            IS_ASSERT(
                hash::has(_scheme_handlers, static_cast<uint64_t>(scheme.hash_value)) == false, "A handler for the given scheme {} already exists!", scheme);
            hash::set(_scheme_handlers, static_cast<uint64_t>(scheme.hash_value), module_object);
        }
        _modules.push_back(std::move(module_ptr));
    }

    auto ResourceSystem::find(const URI& location) noexcept -> Resource*
    {
        auto* module_object = hash::get<ResourceModule*>(_scheme_handlers, static_cast<uint64_t>(location.scheme.hash_value), nullptr);
        return module_object != nullptr ? module_object->find(location) : nullptr;
    }

    auto ResourceSystem::find(const URN& name) noexcept -> Resource*
    {
        return hash::get<Resource*>(_named_resources, static_cast<uint64_t>(name.name.hash_value), nullptr);
    }

    auto ResourceSystem::open(const URI& location) noexcept -> OutputResource*
    {
        OutputResource* result = nullptr;
        auto* module_object = hash::get<ResourceModule*>(_scheme_handlers, static_cast<uint64_t>(location.scheme.hash_value), nullptr);
        if (module_object != nullptr)
        {
            core::MessageBuffer module_messages{ core::memory::globals::default_allocator() };
            result = module_object->open(location, module_messages);

            core::message::for_each(module_messages, std::bind(&ResourceSystem::handle_module_message, this, std::placeholders::_1));
        }
        return result;
    }

    auto ResourceSystem::open(const URN& name) noexcept -> OutputResource*
    {
        return hash::get<OutputResource*>(_named_output_resources, static_cast<uint64_t>(name.name.hash_value), nullptr);
    }

    auto ResourceSystem::mount(const URI& location) noexcept -> uint32_t
    {
        uint32_t resource_count = 0;

        auto* module_object = hash::get<ResourceModule*>(_scheme_handlers, static_cast<uint64_t>(location.scheme.hash_value), nullptr);
        if (module_object != nullptr)
        {
            core::MessageBuffer module_messages{ core::memory::globals::default_allocator() };
            resource_count = module_object->mount(location, module_messages);

            core::message::for_each(module_messages, std::bind(&ResourceSystem::handle_module_message, this, std::placeholders::_1));
        }
        return resource_count;
    }

    auto ResourceSystem::mount(const URN& name) noexcept -> uint32_t
    {
        uint32_t resource_count = 0;

        auto* resource_object = find(name);
        if (resource_object != nullptr)
        {
            resource_count = this->mount(resource_object->location());
        }
        return resource_count;
    }

    void ResourceSystem::handle_module_message(core::Message const& message) noexcept
    {
        if (message.header.type == resource::message::ModuleResourceMounted::message_type)
        {
            auto const& msg_mounted = *reinterpret_cast<resource::message::ModuleResourceMounted const*>(message.data._data);
            auto* const resource_object = msg_mounted.resource_object;

            URN provided_name = resource_object->name();
            URN calculated_name = resource::get_name(resource_object->location());
            IS_ASSERT(calculated_name.name == provided_name.name, "Resource names are not consitent! {} != {}", calculated_name, provided_name);

            const auto& name_hash = static_cast<uint64_t>(provided_name.name.hash_value);
            if (hash::has(_named_resources, name_hash))
            {
                auto* old_res = hash::get<Resource*>(_named_resources, name_hash, nullptr);
                fmt::print("Updating resource with name {}!\n> old: {}\n> new: {}\n", calculated_name, old_res->location(), resource_object->location());
            }

            hash::set(_named_resources, name_hash, resource_object);

            core::message::push(_messages, resource::message::ResourceAdded{
                    resource_object->location(),
                    resource_object->name()
                });
        }
        else
        {
            IS_ASSERT(false, "Unhandled module message type: {}!", message.header.type);
        }
    }

    void ResourceSystem::flush_messages() noexcept
    {
        core::message::clear(_messages);
    }

} // namespace resource
