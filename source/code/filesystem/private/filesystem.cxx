#include <filesystem/filesystem.hxx>
#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/scratch_allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/string.hxx>

#include <core/pod/array.hxx>
#include <core/pod/hash.hxx>

#include <unordered_map>
#include <filesystem>

namespace resource
{
namespace detail
{

namespace hash = core::pod::hash;
namespace array = core::pod::array;

void mount_directory(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, core::pod::Hash<Resource*>& entry_map) noexcept
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
            auto fileid = core::cexpr::stringid(filename.c_str());

            auto* entry_object = alloc.make<Resource>(URI{ scheme_file, { alloc, fullpath.c_str() } });
            array::push_back(entry_list, entry_object);

            auto fileid_hash = static_cast<uint64_t>(fileid.hash_value);
            if (hash::has(entry_map, fileid_hash))
            {
                auto* entry = hash::get<Resource*>(entry_map, fileid_hash, nullptr);
                fmt::print("INFO: Replacing default file '{}' with '{}'. [{}]\n", entry->location(), entry_object->location(), fileid);
            }

            hash::set(entry_map, fileid_hash, entry_object);
        }
    }
}

} // namespace detail

FileSystem::FileSystem(core::allocator& alloc, std::string_view basedir) noexcept
    : _basedir{ basedir }
    , _allocator{ "resource-system", alloc }
    , _resources{ _allocator }
    , _default_resources{ _allocator }
{
    core::pod::array::reserve(_resources, 100);
    core::pod::hash::reserve(_default_resources, 100);
}

FileSystem::~FileSystem() noexcept
{
    core::pod::hash::clear(_default_resources);
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

        if (!core::string::equals(res_uri.path, uri.path))
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

auto FileSystem::find(const URN& urn) noexcept -> Resource*
{
    return core::pod::hash::get<Resource*>(_default_resources, static_cast<uint64_t>(urn.name.hash_value), nullptr);
}

auto FileSystem::mount(const URI& uri) noexcept -> uint32_t
{
    if (uri.scheme == resource::scheme_resource)
    {
        // nothing
    }
    else if (uri.scheme == resource::scheme_file)
    {
        // single file mount
    }
    else if (uri.scheme == resource::scheme_directory)
    {
        detail::mount_directory(_allocator, std::filesystem::path{ _basedir } / core::string::begin(uri.path), _resources, _default_resources);
        // directory recursive mount
    }
    else if (uri.scheme == resource::scheme_pack)
    {
        // pack file mount /* not implemented */
    }
    return 0;
}

namespace detail
{

//! \brief Native file system file Registry.
struct Registry
{
    Registry(core::allocator& backing_alloc) noexcept
        : _registry_allocator{ backing_alloc, 1024 * 1024 * 4 /* 4MiB of data */ }
        , _registry_stats{ "filesystem-registry", _registry_allocator }
        , _entries{ backing_alloc }
        , _default_entries{ backing_alloc }
    {
        core::pod::array::reserve(_entries, 100);
        core::pod::hash::reserve(_default_entries, 100);
    }

    ~Registry() noexcept
    {
        core::pod::hash::clear(_default_entries);
        for (auto* entry : _entries)
        {
            _registry_stats.destroy(entry);
        }
        core::pod::array::clear(_entries);
    }

    //! \brief The sub allocator for the file Registry.
    core::memory::scratch_allocator _registry_allocator;

    //! \brief The stats allocator for the Registry.
    core::memory::proxy_allocator _registry_stats;

    //! \brief A single entry in the Registry.
    struct Entry
    {
        Entry(core::allocator& alloc, core::cexpr::stringid_argument_type fileid, std::string_view file_path) noexcept
            : _path{ alloc, file_path.data() }
            , _fileid{ fileid }
        { }

        ~Entry() noexcept
        {
            core::string::set_capacity(_path, 0);
        }

        //! \brief Path of a single file entry.
        core::String<> _path;

        //! \brief The file identifier.
        core::cexpr::stringid_type _fileid;
    };

    //! \brief Array of all entries in the filesystem.
    core::pod::Array<Entry*> _entries;

    //! \brief Map of default (last loaded) entries.
    core::pod::Hash<Entry*> _default_entries;
};


//! \brief Internal file system State.
struct State
{
    State(core::allocator& backing_alloc, std::string_view basedir) noexcept
        : _allocator{ "filesystem", backing_alloc }
        , _basedir{ std::filesystem::canonical(basedir).generic_string().data() }
        , _registry{ _allocator }
    { }

    ~State() noexcept = default;

    //! \brief The file system allocator object
    core::memory::proxy_allocator _allocator;

    //! \brief The base directory where the file system is initialized.
    core::StackString<128> _basedir;

    //! \brief Native file registry
    Registry _registry;
};

} // namespace detail


namespace hash = core::pod::hash;
namespace array = core::pod::array;


//! \brief The file system State object.
detail::State* _global_state;


void init(core::allocator& alloc, std::string_view basedir) noexcept
{
    bool filesystem_initialized = _global_state != nullptr;
    IS_ASSERT(filesystem_initialized == false, "The file system is already initialized!");

    _global_state = alloc.make<detail::State>(alloc, basedir);
}

void shutdown() noexcept
{
    bool filesystem_initialized = _global_state != nullptr;
    IS_ASSERT(filesystem_initialized == true, "The file system was not initialized!");

    auto& backing_alloc = _global_state->_allocator.backing_allocator();
    backing_alloc.destroy(_global_state);
    _global_state = nullptr;
}

void mount(std::string_view path) noexcept
{
    bool filesystem_initialized = _global_state != nullptr;
    IS_ASSERT(filesystem_initialized == true, "The file system was not initialized!");

    auto& alloc = _global_state->_registry._registry_stats;

    auto& entry_list = _global_state->_registry._entries;
    auto& entry_map = _global_state->_registry._default_entries;

    // Build the path
    std::filesystem::path full_path{ _global_state->_basedir._data };
    full_path = std::filesystem::canonical(full_path / path);

    // Traverse the directory
    std::filesystem::recursive_directory_iterator directory_iterator{ full_path };
    for (auto&& native_entry : directory_iterator)
    {
        if (std::filesystem::is_regular_file(native_entry))
        {
            auto filepath = native_entry.path();
            auto filename = filepath.filename().generic_string();

            auto fullpath = std::filesystem::canonical(filepath).generic_string();
            auto fileid = core::cexpr::stringid(filename.c_str());

            auto* entry_object = alloc.make<detail::Registry::Entry>(alloc, fileid, fullpath);
            array::push_back(entry_list, entry_object);

            auto fileid_hash = static_cast<uint64_t>(entry_object->_fileid.hash_value);
            if (hash::has(entry_map, fileid_hash))
            {
                auto* entry = hash::get<detail::Registry::Entry*>(entry_map, fileid_hash, nullptr);
                fmt::print("INFO: Replacing default file '{}' with '{}'. [{}]\n", entry->_path, fullpath, fileid);
            }

            hash::set(entry_map, fileid_hash, entry_object);
        }
    }

    fmt::print("File System Registry allocated: {} bytes with {} allocations.\n",
        _global_state->_registry._registry_stats.total_allocated(),
        _global_state->_registry._registry_stats.allocation_count()
    );
}

} // namespace filesystem
