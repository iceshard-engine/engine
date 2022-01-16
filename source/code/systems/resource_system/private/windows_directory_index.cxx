//#include <ice/resource_index.hxx>
//#include <ice/resource_query.hxx>
//#include <ice/resource_meta.hxx>
//#include <ice/memory/proxy_allocator.hxx>
//#include <ice/memory/stack_allocator.hxx>
//#include <ice/memory/memory_globals.hxx>
//#include <ice/os/windows.hxx>
//#include <ice/pod/array.hxx>
//#include <ice/pod/hash.hxx>
//#include <ice/buffer.hxx>
//#include <filesystem>
//
//#include <ice/heap_string.hxx>
//
//#include "path_utils.hxx"
//
//#if ISP_WINDOWS
//
//namespace ice
//{
//
//    namespace detail
//    {
//
//        struct GuardedMemory final
//        {
//            GuardedMemory(ice::Allocator& alloc, ice::u32 size) noexcept
//                : alloc{ alloc }
//                , memory{ alloc.allocate(size, 4), size, 4 }
//            {
//            }
//            ~GuardedMemory() noexcept
//            {
//                alloc.deallocate(memory.location);
//            }
//
//            ice::Allocator& alloc;
//            Memory memory;
//        };
//
//        void load_file_to_buffer(ice::String path, ice::Buffer& file_data) noexcept
//        {
//            FILE* file_native = nullptr;
//            fopen_s(&file_native, ice::string::data(path), "rb");
//
//            if (file_native)
//            {
//                ice::memory::StackAllocator_4096 kib4_alloc;
//                GuardedMemory file_chunk{ kib4_alloc, 1024u * 4u };
//
//                auto characters_read = fread_s(file_chunk.memory.location, file_chunk.memory.size, sizeof(char), file_chunk.memory.size, file_native);
//                while (characters_read > 0)
//                {
//                    ice::buffer::append(file_data, file_chunk.memory.location, static_cast<ice::u32>(characters_read), 4);
//                    characters_read = fread_s(file_chunk.memory.location, file_chunk.memory.size, sizeof(char), file_chunk.memory.size, file_native);
//                }
//
//                fclose(file_native);
//            }
//        }
//
//        class FileResource final : public ice::Resource
//        {
//        public:
//            FileResource(ice::HeapString<> file_path, ice::HeapString<> meta_path) noexcept
//                : ice::Resource{ }
//                , _file_path{ ice::move(file_path) }
//                , _meta_path{ ice::move(meta_path) }
//                , _uri{ ice::scheme_file, _file_path, ice::stringid_invalid }
//            { }
//
//            auto native_path() const noexcept -> ice::String
//            {
//                return _file_path;
//            }
//
//            auto name() const noexcept -> ice::String override
//            {
//                return ice::path::filename(_file_path);
//            }
//
//            auto location() const noexcept -> ice::URI const& override
//            {
//                return _uri;
//            }
//
//            auto metadata() const noexcept -> ice::Metadata const& override
//            {
//                return _metadata_view;
//            }
//
//            auto data() noexcept -> ice::Data override
//            {
//                return {};
//            }
//
//        private:
//            ice::HeapString<> _file_path;
//            ice::HeapString<> _meta_path;
//            ice::URI _uri;
//
//            ice::Metadata _metadata_view;
//        };
//
//        class DirectoryResource final : public ice::Resource
//        {
//        public:
//            DirectoryResource(
//                ice::HeapString<> file_path,
//                ice::HeapString<> meta_path,
//                ice::String file_relative,
//                ice::URI const& uri
//            ) noexcept
//                : ice::Resource{ }
//                , _file_path{ ice::move(file_path) }
//                , _meta_path{ ice::move(meta_path) }
//                , _file_relative{ file_relative }
//                , _uri{ uri }
//                , _metadata{ ice::memory::default_allocator() }
//                , _resource_data{ ice::memory::default_allocator() }
//            {
//                if (_metadata_loaded == false && ice::string::empty(_meta_path) == false)
//                {
//                    ice::Buffer temp_bufer{ ice::memory::default_scratch_allocator() };
//                    load_file_to_buffer(_meta_path, temp_bufer);
//                    ice::meta_deserialize(temp_bufer, _metadata);
//                    _metadata_view = _metadata;
//                    _metadata_loaded = true;
//                }
//            }
//
//            auto native_path() const noexcept -> ice::String
//            {
//                return _file_path;
//            }
//
//            auto directory_path() const noexcept -> ice::String
//            {
//                return ice::string::substr(_file_path, 0, ice::string::size(_file_path) - ice::string::size(_file_relative));
//            }
//
//            auto name() const noexcept -> ice::String override
//            {
//                return _file_relative;
//            }
//
//            auto location() const noexcept -> ice::URI const& override
//            {
//                return _uri;
//            }
//
//            auto metadata() const noexcept -> ice::Metadata const& override
//            {
//                return _metadata_view;
//            }
//
//            auto data() noexcept -> ice::Data override
//            {
//                if (ice::buffer::empty(_resource_data))
//                {
//                    load_file_to_buffer(_file_path, _resource_data);
//                }
//                return _resource_data;
//            }
//
//        private:
//            ice::HeapString<> _file_path;
//            ice::HeapString<> _meta_path;
//            ice::String _file_relative;
//            ice::URI _uri;
//
//            ice::MutableMetadata _metadata;
//            ice::Metadata _metadata_view;
//            ice::Buffer _resource_data;
//            bool _metadata_loaded = false;
//        };
//
//    } // namespace detail
//
//    class WindowsIndex final : public ice::ResourceIndex
//    {
//    public:
//        WindowsIndex(ice::Allocator& alloc, ice::String base_path) noexcept;
//        ~WindowsIndex() noexcept override;
//
//        auto find_relative(
//            ice::Resource const& root_resource,
//            ice::String path
//        ) noexcept -> ice::Resource* override;
//
//        bool query_changes(ice::ResourceQuery& query) noexcept override;
//        bool mount(ice::URI const& uri) noexcept override;
//
//        auto request(ice::URI const& uri) noexcept -> ice::Resource* override;
//
//        bool release(ice::URI const& uri) noexcept override;
//
//    protected:
//        bool mount_file_resource(ice::HeapString<>& path) noexcept;
//        bool mount_directory_resource(ice::HeapString<>& path) noexcept;
//
//    private:
//        ice::Allocator& _allocator;
//        ice::memory::ProxyAllocator _path_allocator;
//        ice::HeapString<> _base_path;
//
//        ice::pod::Hash<ice::Resource*> _resources;
//
//        ice::pod::Array<ice::ResourceEvent> _events;
//        ice::pod::Array<ice::Resource*> _event_objects;
//    };
//
//    WindowsIndex::WindowsIndex(ice::Allocator& alloc, ice::String base_path) noexcept
//        : ice::ResourceIndex{ }
//        , _allocator{ alloc }
//        , _path_allocator{ alloc, "file_path_alloc" }
//        , _base_path{ _path_allocator, base_path }
//        , _resources{ alloc }
//        , _events{ alloc }
//        , _event_objects{ alloc }
//    {
//        if (_base_path == ".")
//        {
//            _base_path = std::filesystem::current_path().generic_string();
//        }
//    }
//
//    WindowsIndex::~WindowsIndex() noexcept
//    {
//        for (auto& entry : _resources)
//        {
//            _allocator.destroy(entry.value);
//        }
//    }
//
//    auto WindowsIndex::find_relative(
//        ice::Resource const& root_resource,
//        ice::String path
//    ) noexcept -> ice::Resource*
//    {
//        using ice::detail::FileResource;
//        using ice::detail::DirectoryResource;
//
//        URI const& root_uri = root_resource.location();
//
//        auto* entry = ice::pod::multi_hash::find_first(_resources, ice::hash(root_uri.fragment));
//        while (entry != nullptr)
//        {
//            ice::URI const& possible_uri = entry->value->location();
//            if (possible_uri.scheme != root_uri.scheme || possible_uri.path != root_uri.path)
//            {
//                entry = ice::pod::multi_hash::find_next(_resources, entry);
//            }
//            else
//            {
//                break;
//            }
//        }
//
//        if (entry == nullptr)
//        {
//            return nullptr;
//        }
//
//        ice::HeapString file_path{ _allocator };
//        ice::URI predicted_uri = ice::uri_invalid;
//        if (root_uri.scheme == ice::stringid_hash(ice::scheme_file))
//        {
//            FileResource const& file_res = static_cast<FileResource const&>(root_resource);
//
//            file_path = file_res.native_path();
//            ice::path::replace_filename(file_path, path);
//            ice::path::normalize(file_path);
//
//            ice::String file_name = ice::path::filename(file_path);
//
//            predicted_uri = ice::URI{
//                ice::scheme_file,
//                file_path,
//                ice::stringid_invalid
//            };
//        }
//        else if (root_uri.scheme == ice::stringid_hash(ice::scheme_directory))
//        {
//            DirectoryResource const& file_res = static_cast<DirectoryResource const&>(root_resource);
//
//            file_path = file_res.native_path();
//            ice::path::replace_filename(file_path, path);
//            ice::path::normalize(file_path);
//
//            ice::String directory_path = file_res.directory_path();
//            ice::String file_relative = ice::string::substr(file_path, ice::string::size(directory_path));
//
//            predicted_uri = ice::URI{
//                ice::scheme_directory,
//                directory_path,
//                ice::stringid(file_relative)
//            };
//        }
//
//        Resource* result = nullptr;
//        if (predicted_uri.scheme != ice::stringid_hash(ice::scheme_invalid))
//        {
//            result = request(predicted_uri);
//        }
//
//        return result;
//    }
//
//    bool WindowsIndex::query_changes(ice::ResourceQuery& query) noexcept
//    {
//        if (ice::pod::array::any(_events))
//        {
//            query.events = ice::move(_events);
//            query.objects = ice::move(_event_objects);
//            return true;
//        }
//        return false;
//    }
//
//    bool WindowsIndex::mount(ice::URI const& uri) noexcept
//    {
//        if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_file))
//        {
//            ice::HeapString<> file_path{ _path_allocator, _base_path };
//            ice::path::join(file_path, uri.path);
//            return mount_file_resource(file_path);
//        }
//        else if (ice::stringid_hash(uri.scheme) == ice::stringid_hash(ice::scheme_directory))
//        {
//            ice::HeapString<> directory_path{ _path_allocator, _base_path };
//            ice::path::join(directory_path, uri.path);
//            return mount_directory_resource(directory_path);
//        }
//        return false;
//    }
//
//    auto WindowsIndex::request(ice::URI const& uri) noexcept -> ice::Resource*
//    {
//        ice::Resource* result = nullptr;
//
//        auto* entry = ice::pod::multi_hash::find_first(_resources, ice::hash(uri.fragment));
//        while (entry != nullptr && result == nullptr)
//        {
//            ice::URI const& possible_uri = entry->value->location();
//            if (possible_uri.scheme == uri.scheme && possible_uri.path == uri.path)
//            {
//                result = entry->value;
//            }
//
//            entry = ice::pod::multi_hash::find_next(_resources, entry);
//        }
//
//        return result;
//    }
//
//    bool WindowsIndex::release(ice::URI const& /*uri*/) noexcept
//    {
//        return false;
//    }
//
//    bool WindowsIndex::mount_file_resource(ice::HeapString<>& file_path) noexcept
//    {
//        if (std::filesystem::is_regular_file(ice::String{ file_path }) == false)
//        {
//            ice::pod::array::push_back(_events, ice::ResourceEvent::MountError);
//            ice::pod::array::push_back(_event_objects, nullptr);
//            return false;
//        }
//
//        ice::String extension = ice::path::extension(file_path);
//        if (extension == ".isrm")
//        {
//            return false;
//        }
//
//        ice::path::normalize(file_path);
//
//        ice::HeapString<> meta_path = file_path;
//        ice::string::push_back(meta_path, ".isrm");
//
//        if (std::filesystem::is_regular_file(ice::String{ meta_path }) == false)
//        {
//            // Releases the string entierly
//            ice::string::set_capacity(meta_path, 0);
//        }
//
//        ice::Resource* resource = _allocator.make<detail::FileResource>(
//            ice::move(file_path),
//            ice::move(meta_path)
//        );
//
//        ice::pod::multi_hash::insert(_resources, ice::hash(resource->location().fragment), resource);
//        ice::pod::array::push_back(_events, ice::ResourceEvent::Added);
//        ice::pod::array::push_back(_event_objects, resource);
//        return true;
//    }
//
//    bool WindowsIndex::mount_directory_resource(ice::HeapString<>& dir_path) noexcept
//    {
//        if (std::filesystem::is_directory(ice::String{ dir_path }) == false)
//        {
//            ice::pod::array::push_back(_events, ice::ResourceEvent::MountError);
//            ice::pod::array::push_back(_event_objects, nullptr);
//            return false;
//        }
//
//        ice::path::normalize(dir_path);
//
//        // Traverse the directory
//        std::filesystem::recursive_directory_iterator directory_iterator{ ice::String{ dir_path } };
//        for (auto const& native_entry : directory_iterator)
//        {
//            if (std::filesystem::is_regular_file(native_entry) == false)
//            {
//                continue;
//            }
//
//            ice::HeapString<> file_path{ _path_allocator, native_entry.path().generic_string() };
//            ice::path::normalize(file_path);
//
//            ice::String extension = ice::path::extension(file_path);
//            if (extension == ".isrm")
//            {
//                continue;
//            }
//
//            ice::String relative_path = ice::string::substr(file_path, ice::string::length(dir_path));
//            ice::String directory_path = ice::string::substr(file_path, 0, ice::string::length(dir_path));
//
//            Resource* resource = nullptr;
//            if (extension == ".isr")
//            {
//                // Not implemented yet
//                continue;
//            }
//            else
//            {
//                ice::HeapString<> meta_path = file_path;
//                ice::string::push_back(meta_path, ".isrm");
//
//                if (std::filesystem::is_regular_file(ice::String{ meta_path }) == false)
//                {
//                    // Releases the string entierly
//                    ice::string::set_capacity(meta_path, 0);
//                }
//
//                ice::URI resource_uri{
//                    ice::scheme_directory,
//                    directory_path,
//                    ice::stringid(relative_path)
//                };
//
//                resource = _allocator.make<detail::DirectoryResource>(
//                    ice::move(file_path),
//                    ice::move(meta_path),
//                    ice::move(relative_path),
//                    resource_uri
//                );
//            }
//
//            if (resource != nullptr)
//            {
//                ice::pod::multi_hash::insert(_resources, ice::hash(resource->location().fragment), resource);
//                ice::pod::array::push_back(_events, ice::ResourceEvent::Added);
//                ice::pod::array::push_back(_event_objects, resource);
//            }
//        }
//
//        return true;
//    }
//
//    auto create_filesystem_index(ice::Allocator& alloc, ice::String base_path) noexcept -> ice::UniquePtr<ice::ResourceIndex>
//    {
//        return ice::make_unique<ice::ResourceIndex, ice::WindowsIndex>(alloc, alloc, base_path);
//    }
//
//} // namespace ice
//
//#endif // ISP_WINDOWS
