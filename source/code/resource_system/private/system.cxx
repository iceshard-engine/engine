#include <resource/system.hxx>

#include <core/memory.hxx>
#include <core/pod/hash.hxx>

namespace resource
{

namespace hash = core::pod::hash;

ResourceSystem::ResourceSystem(core::allocator& alloc) noexcept
    : _allocator{ "resource-system", alloc }
    , _named_resources{ alloc }
    , _scheme_handlers{ alloc }
{
    hash::reserve(_named_resources, 200);
    hash::reserve(_scheme_handlers, 10);
}

void ResourceSystem::add_module(core::memory::unique_pointer<ResourceModule> module_ptr, const core::pod::Array<core::cexpr::stringid_type>& schemes) noexcept
{
    auto* module_object = module_ptr.get();
    for (auto& scheme : schemes)
    {
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

auto ResourceSystem::mount(const URI& location) noexcept -> uint32_t
{
    uint32_t resource_count = 0;

    auto* module_object = hash::get<ResourceModule*>(_scheme_handlers, static_cast<uint64_t>(location.scheme.hash_value), nullptr);
    if (module_object != nullptr)
    {
        // Mount the location and save all returned names.
        resource_count = module_object->mount(location, [&](Resource* res) noexcept
            {
                const auto& name = resource::get_name(res->location());
                const auto& name_hash = static_cast<uint64_t>(name.name.hash_value);

                if (hash::has(_named_resources, name_hash))
                {
                    auto* old_res = hash::get<Resource*>(_named_resources, name_hash, nullptr);
                    fmt::print("Replacing default file '{}' with '{}'! {}\n", old_res->location(), res->location(), name);
                }

                // Save the resource under it's name.
                hash::set(_named_resources, name_hash, res);
            });
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

} // namespace resource
