#include <ice/resource_index.hxx>
#include <ice/platform/windows.hxx>
#include <ice/memory/proxy_allocator.hxx>
#include <ice/pod/array.hxx>
#include <ice/pod/hash.hxx>
#include <filesystem>

#include "path_utils.hxx"

#if ISP_WINDOWS

namespace ice
{

    namespace detail
    {

        class FileResource final : public ice::Resource
        {
        public:
            FileResource(ice::HeapString<> file_path, ice::HeapString<> meta_path) noexcept
                : ice::Resource{ }
                , _file_path{ ice::move(file_path) }
                , _meta_path{ ice::move(meta_path) }
                , _uri{ ice::scheme_file, _file_path, ice::stringid_invalid }
            { }

            auto name() const noexcept -> ice::String override
            {
                return ice::path::filename(_file_path);
            }

            auto location() const noexcept -> ice::URI const& override
            {
                return _uri;
            }

            auto data() noexcept -> ice::Data override
            {
                return ice::Data{};
            }

            auto metadata() noexcept -> ice::Data override
            {
                return ice::Data{};
            }

        private:
            ice::HeapString<> _file_path;
            ice::HeapString<> _meta_path;
            ice::URI _uri;
        };

        class DirectoryResource final : public ice::Resource
        {
        public:
            DirectoryResource(
                ice::HeapString<> file_path,
                ice::HeapString<> meta_path,
                ice::String file_relative,
                ice::URI const& uri
            ) noexcept
                : ice::Resource{ }
                , _file_path{ ice::move(file_path) }
                , _meta_path{ ice::move(meta_path) }
                , _file_relative{ file_relative }
                , _uri{ uri }
            { }

            auto name() const noexcept -> ice::String override
            {
                return _file_relative;
            }

            auto location() const noexcept -> ice::URI const& override
            {
                return _uri;
            }

            auto data() noexcept -> ice::Data override
            {
                return ice::Data{};
            }

            auto metadata() noexcept -> ice::Data override
            {
                return ice::Data{};
            }

        private:
            ice::HeapString<> _file_path;
            ice::HeapString<> _meta_path;
            ice::String _file_relative;
            ice::URI _uri;
        };

    } // namespace detail

    class WindowsIndex final : public ice::ResourceIndex
    {
    public:
        WindowsIndex(ice::Allocator& alloc, ice::String base_path) noexcept;
        ~WindowsIndex() noexcept override;

        bool query_changes(ice::ResourceQuery& query) noexcept override;
        bool mount(ice::URI const& uri) noexcept override;
        auto request(ice::URI const& uri) noexcept -> ice::Resource* override;
        bool release(ice::URI const& uri) noexcept override;

    protected:
        bool mount_file_resource(ice::HeapString<>& path) noexcept;
        bool mount_directory_resource(ice::HeapString<>& path) noexcept;

    private:
        ice::Allocator& _allocator;
        ice::memory::ProxyAllocator _path_allocator;
        ice::HeapString<> _base_path;

        ice::pod::Hash<ice::Resource*> _resources;

        ice::pod::Array<ice::ResourceEvent> _events;
        ice::pod::Array<ice::Resource const*> _event_objects;
    };

    WindowsIndex::WindowsIndex(ice::Allocator& alloc, ice::String base_path) noexcept
        : ice::ResourceIndex{ }
        , _allocator{ alloc }
        , _path_allocator{ alloc, "file_path_alloc" }
        , _base_path{ _path_allocator, base_path }
        , _resources{ alloc }
        , _events{ alloc }
        , _event_objects{ alloc }
    {
        if (_base_path == ".")
        {
            _base_path = std::filesystem::current_path().generic_string();
        }
    }

    WindowsIndex::~WindowsIndex() noexcept
    {
        for (auto& entry : _resources)
        {
            _allocator.destroy(entry.value);
        }
    }

    bool WindowsIndex::query_changes(ice::ResourceQuery& query) noexcept
    {
        if (ice::pod::array::any(_events))
        {
            query.events = _events;
            query.objects = _event_objects;
            return true;
        }
        return false;
    }

    bool WindowsIndex::mount(ice::URI const& uri) noexcept
    {
        if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_file))
        {
            ice::HeapString<> file_path{ _path_allocator, _base_path };
            ice::path::join(file_path, uri.path);
            return mount_file_resource(file_path);
        }
        else if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_directory))
        {
            ice::HeapString<> directory_path{ _path_allocator, _base_path };
            ice::path::join(directory_path, uri.path);
            return mount_directory_resource(directory_path);
        }
        return false;
    }

    auto WindowsIndex::request(ice::URI const& uri) noexcept -> ice::Resource*
    {
        ice::Resource* result = nullptr;

        auto* entry = ice::pod::multi_hash::find_first(_resources, ice::hash(uri.fragment));
        while (entry != nullptr && result == nullptr)
        {
            ice::URI const& possible_uri = entry->value->location();
            if (possible_uri.scheme == uri.scheme && possible_uri.path == uri.path)
            {
                result = entry->value;
            }

            entry = ice::pod::multi_hash::find_next(_resources, entry);
        }

        return result;
    }

    bool WindowsIndex::release(ice::URI const& /*uri*/) noexcept
    {
        return false;
    }

    bool WindowsIndex::mount_file_resource(ice::HeapString<>& file_path) noexcept
    {
        if (std::filesystem::is_regular_file(ice::String{ file_path }) == false)
        {
            ice::pod::array::push_back(_events, ice::ResourceEvent::MountError);
            ice::pod::array::push_back(_event_objects, nullptr);
            return false;
        }

        ice::String extension = ice::path::extension(file_path);
        if (extension == ".isrm")
        {
            return false;
        }

        ice::path::normalize(file_path);

        ice::HeapString<> meta_path = file_path;
        ice::string::push_back(meta_path, ".isrm");

        if (std::filesystem::is_regular_file(ice::String{ meta_path }) == false)
        {
            // Releases the string entierly
            ice::string::set_capacity(meta_path, 0);
        }

        ice::Resource* resource = _allocator.make<detail::FileResource>(
            ice::move(file_path),
            ice::move(meta_path)
        );

        ice::pod::multi_hash::insert(_resources, ice::hash(resource->location().fragment), resource);
        ice::pod::array::push_back(_events, ice::ResourceEvent::Added);
        ice::pod::array::push_back(_event_objects, resource);
        return true;
    }

    bool WindowsIndex::mount_directory_resource(ice::HeapString<>& dir_path) noexcept
    {
        if (std::filesystem::is_directory(ice::String{ dir_path }) == false)
        {
            ice::pod::array::push_back(_events, ice::ResourceEvent::MountError);
            ice::pod::array::push_back(_event_objects, nullptr);
            return false;
        }

        ice::path::normalize(dir_path);

        // Traverse the directory
        std::filesystem::recursive_directory_iterator directory_iterator{ ice::String{ dir_path } };
        for (auto const& native_entry : directory_iterator)
        {
            if (std::filesystem::is_regular_file(native_entry) == false)
            {
                continue;
            }

            ice::HeapString<> file_path{ _path_allocator, native_entry.path().generic_string() };
            ice::path::normalize(file_path);

            ice::String extension = ice::path::extension(file_path);
            if (extension == ".isrm")
            {
                continue;
            }

            ice::String relative_path = ice::string::substr(file_path, ice::string::length(dir_path));
            ice::String directory_path = ice::string::substr(file_path, 0, ice::string::length(dir_path) - 1);

            Resource* resource = nullptr;
            if (extension == ".isr")
            {
                // Not implemented yet
                continue;
            }
            else
            {
                ice::HeapString<> meta_path = file_path;
                ice::string::push_back(meta_path, ".isrm");

                if (std::filesystem::is_regular_file(ice::String{ meta_path }) == false)
                {
                    // Releases the string entierly
                    ice::string::set_capacity(meta_path, 0);
                }

                ice::URI resource_uri{
                    ice::scheme_directory,
                    directory_path,
                    ice::stringid(relative_path)
                };

                resource = _allocator.make<detail::DirectoryResource>(
                    ice::move(file_path),
                    ice::move(meta_path),
                    ice::move(relative_path),
                    resource_uri
                );
            }

            if (resource != nullptr)
            {
                ice::pod::multi_hash::insert(_resources, ice::hash(resource->location().fragment), resource);
                ice::pod::array::push_back(_events, ice::ResourceEvent::Added);
                ice::pod::array::push_back(_event_objects, resource);
            }
        }

        return true;
    }

    auto create_filesystem_index(ice::Allocator& alloc, ice::String base_path) noexcept -> ice::UniquePtr<ice::ResourceIndex>
    {
        return ice::make_unique<ice::ResourceIndex, ice::WindowsIndex>(alloc, alloc, base_path);
    }

} // namespace ice

#endif // ISP_WINDOWS
