#include <resource/resource.hxx>
#include <resource/filesystem_module.hxx>

#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/string.hxx>

#include <core/pod/array.hxx>

#include <filesystem>

namespace resource
{
namespace detail
{

namespace array = core::pod::array;

void mount_directory(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, std::function<void(Resource*)> callback) noexcept
{
    // Build the path
    path = std::filesystem::canonical(path);

    // Traverse the directory
    std::filesystem::recursive_directory_iterator directory_iterator{ path };
    for (auto&& native_entry : directory_iterator)
    {
        if (std::filesystem::is_regular_file(native_entry))
        {
            auto filepath = native_entry.path();
            auto filename = filepath.filename().generic_string();

            auto fullpath = std::filesystem::canonical(filepath).generic_string();

            auto relative_path = std::filesystem::relative(fullpath, path);
            auto relative_path_string = relative_path.generic_string();

            auto* file_entry_object = alloc.make<Resource>(alloc, URI{ scheme_file, fullpath.c_str() });
            auto* dir_entry_object = alloc.make<Resource>(alloc, URI{ scheme_directory, path.generic_string().c_str(), core::cexpr::stringid(relative_path_string.c_str()) });


            auto add_resource = [&](Resource* res, bool call_callback) noexcept
            {
                array::push_back(entry_list, res);

                if (call_callback)
                {
                    callback(res);
                }
            };

            add_resource(file_entry_object, true);
            add_resource(dir_entry_object, relative_path.has_parent_path());
        }
    }
}

} // namespace detail

FileSystem::FileSystem(core::allocator& alloc, std::string_view basedir) noexcept
    : _basedir{ basedir }
    , _allocator{ "resource-system", alloc }
    , _resources{ _allocator }
{
    core::pod::array::reserve(_resources, 200);
}

FileSystem::~FileSystem() noexcept
{
    for (auto* entry : _resources)
    {
        _allocator.destroy(entry);
    }
    core::pod::array::clear(_resources);
}


auto FileSystem::find(const URI& uri) noexcept -> Resource*
{
    Resource* result{ nullptr };
    for (auto* res : _resources)
    {
        auto& res_uri = res->location();
        if (res_uri.scheme != uri.scheme)
        {
            continue;
        }

        auto uri_path = std::filesystem::canonical(_basedir / std::filesystem::path{ core::string::begin(uri.path) }).generic_string();
        if (!core::string::equals(res_uri.path, uri_path))
        {
            continue;
        }

        if (res_uri.fragment != uri.fragment)
        {
            continue;
        }

        result = res;
    }
    return result;
}

auto FileSystem::mount(const URI& uri, std::function<void(Resource*)> callback) noexcept -> uint32_t
{
    if (uri.scheme == resource::scheme_file)
    {
        // single file mount
    }
    else if (uri.scheme == resource::scheme_directory)
    {
        detail::mount_directory(_allocator, std::filesystem::path{ _basedir } / core::string::begin(uri.path), _resources, callback);
    }
    return 0;
}

} // namespace resource
