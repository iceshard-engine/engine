#include <resource/resource.hxx>
#include <resource/resource_meta.hxx>
#include <resource/modules/filesystem_module.hxx>
#include "module_messages.hxx"

#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/string.hxx>

#include <core/pod/array.hxx>
#include <core/data/chunk.hxx>
#include <core/data/buffer.hxx>
#include <core/path_utils.hxx>

#include <filesystem>
#include <cstdio>

namespace resource
{
    namespace detail
    {
        enum class FileAccessType
        {
            ReadOnly,
            ReadWrite,
        };

        bool load_resource_to_buffer(core::StringView path, core::Buffer& file_data, uint32_t* data_offset) noexcept
        {
            FILE* file_native = nullptr;
            fopen_s(&file_native, core::string::data(path), "rb");

            if (file_native)
            {
                core::memory::stack_allocator_4096 kib4_alloc;
                core::data_chunk file_chunk{ kib4_alloc, 1024u * 4u };

                uint32_t header_size = 0;
                auto characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), 8, file_native);
                if (characters_read != 8)
                {
                    fclose(file_native);
                    return false;
                }

                // Read the header size
                header_size = *(reinterpret_cast<uint32_t*>(file_chunk.data()) + 1);

                if (data_offset == nullptr)
                {
                    header_size -= 8; // Remove the first 8 read bytes

                    while (header_size > 0)
                    {
                        core::buffer::append(file_data, file_chunk.data(), static_cast<uint32_t>(characters_read));
                        characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), header_size, file_native);
                        header_size -= characters_read;
                    }

                    core::buffer::append(file_data, file_chunk.data(), static_cast<uint32_t>(characters_read));
                }
                else
                {
                    *data_offset = header_size;

                    while (characters_read > 0)
                    {
                        core::buffer::append(file_data, file_chunk.data(), static_cast<uint32_t>(characters_read));
                        characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), file_chunk.size(), file_native);
                    }
                }

                fclose(file_native);

                if (core::buffer::size(file_data) >= 4)
                {
                    auto* res_head = reinterpret_cast<char const*>(core::buffer::begin(file_data));
                    return core::string::equals("ISRA", { res_head, 4 });
                }
            }

            return false;
        }

        void load_file_to_buffer(core::StringView path, core::Buffer& file_data) noexcept
        {
            FILE* file_native = nullptr;
            fopen_s(&file_native, core::string::data(path), "rb");

            if (file_native)
            {
                core::memory::stack_allocator_4096 kib4_alloc;
                core::data_chunk file_chunk{ kib4_alloc, 1024u * 4u };

                auto characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), file_chunk.size(), file_native);
                while (characters_read > 0)
                {
                    core::buffer::append(file_data, file_chunk.data(), static_cast<uint32_t>(characters_read));
                    characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), file_chunk.size(), file_native);
                }

                fclose(file_native);
            }
        }

        class BakedFileResource : public Resource
        {
        public:
            BakedFileResource(
                core::allocator& alloc,
                URI const& uri,
                core::StringView native_path,
                core::StringView native_name
            ) noexcept
                : Resource{ }
                , _native_path{ alloc, native_path }
                , _native_name{ alloc, native_name }
                , _path{ alloc, uri.path }
                , _uri{ uri.scheme, _path, uri.fragment }
                , _data{ alloc }
            {
            }

            ~BakedFileResource() override = default;

            //! \brief The resource identifier.
            //! \remark This value can be seen as the absolute location to a specific resource.
            auto location() const noexcept -> URI const& override final
            {
                return _uri;
            }

            //! \brief Returns the associated resource data.
            auto data() noexcept -> core::data_view override
            {
                if (_partial_load == true)
                {
                    core::buffer::clear(_data);
                    load_resource_to_buffer(_native_path, _data, &_data_offset);

                    _partial_load = false;
                    _metadata = resource::load_meta_view(
                        core::data_view{
                            core::memory::utils::pointer_add(core::buffer::data(_data), 8),
                            core::buffer::size(_data) - 8
                        }
                    );
                    _data_view = {
                        core::memory::utils::pointer_add(core::buffer::data(_data), _data_offset),
                        core::buffer::size(_data) - _data_offset
                    };
                }
                return _data_view;
            }

            auto metadata() noexcept -> resource::ResourceMetaView override
            {
                if (_meta_loaded == false && _partial_load == true)
                {
                    load_resource_to_buffer(_native_path, _data, nullptr);
                    _metadata = resource::load_meta_view(
                        core::data_view{
                            core::memory::utils::pointer_add(core::buffer::data(_data), 8),
                            core::buffer::size(_data) - 8
                        }
                    );
                }
                return _metadata;
            }

            auto name() const noexcept -> core::StringView override
            {
                return _native_name;
            }

        private:
            core::String<> _native_path;
            core::String<> _native_name;

            //! \brief The resource path.
            core::String<> _path;

            //! \brief The resource identifier.
            URI _uri;

            //! \brief The loaded file buffer.
            core::Buffer _data;

            core::data_view _data_view{ nullptr, 0 };
            resource::ResourceMetaView _metadata;

            bool _meta_loaded = false;
            bool _partial_load = true;
            uint32_t _data_offset = 0;
        };

        class FileResource : public Resource
        {
        public:
            FileResource(
                core::allocator& alloc,
                URI const& uri,
                core::StringView native_path,
                core::StringView native_meta_path,
                core::StringView native_name) noexcept
                : _native_path{ alloc, native_path }
                , _native_path_metadata{ alloc, native_meta_path }
                , _native_name{ alloc, native_name }
                , _path{ alloc, uri.path }
                , _uri{ uri.scheme, _path, uri.fragment }
                , _metadata{ alloc }
                , _data{ alloc }
            {
            }

            ~FileResource() override = default;

            //! \brief The resource identifier.
            //! \remark This value can be seen as the absolute location to a specific resource.
            auto location() const noexcept -> const URI& override final
            {
                return _uri;
            }

            //! \brief Returns the associated resource data.
            auto data() noexcept -> core::data_view override
            {
                if (core::buffer::empty(_data))
                {
                    load_file_to_buffer(_native_path, _data);
                }
                return _data;
            }

            auto metadata() noexcept -> resource::ResourceMetaView override
            {
                if (_metadata_loaded == false && core::string::empty(_native_path_metadata) == false)
                {
                    core::Buffer temp_bufer{ core::memory::globals::default_scratch_allocator() };
                    load_file_to_buffer(_native_path_metadata, temp_bufer);
                    resource::deserialize_meta(temp_bufer, _metadata);
                    _metadata_loaded = true;
                }
                return resource::create_meta_view(_metadata);
            }

            auto name() const noexcept -> core::StringView override
            {
                return _native_name;
            }

            virtual auto access_type() noexcept -> FileAccessType { return FileAccessType::ReadOnly; }

        protected:
            //! \brief The native filesystem path.
            core::String<> _native_path;
            core::String<> _native_path_metadata;
            core::String<> _native_name;

            //! \brief The resource path.
            core::String<> _path;

            //! \brief The resource identifier.
            URI _uri;

            //! \brief The loaded metadata.
            bool _metadata_loaded = false;
            resource::ResourceMeta _metadata;

            //! \brief The loaded file buffer.
            core::Buffer _data;
        };

        class FileResourceWritable : public FileResource, public OutputResource
        {
        public:
            FileResourceWritable(
                core::allocator& alloc,
                URI const& uri,
                core::StringView native_path,
                core::StringView native_name) noexcept
                : FileResource{ alloc, uri, native_path, "", native_name }
            {
            }

            ~FileResourceWritable() noexcept
            {
                if (_file_native != nullptr)
                {
                    fclose(_file_native);
                }
            }

            //! \brief Returns the associated resource data.
            auto data() noexcept -> core::data_view override final
            {
                if (_file_native != nullptr)
                {
                    core::buffer::clear(_data);

                    fpos_t current_pos;
                    fgetpos(_file_native, &current_pos);
                    rewind(_file_native);

                    {
                        core::memory::stack_allocator_4096 kib4_alloc;
                        core::data_chunk file_chunk{ kib4_alloc, 1024u * 4u };

                        auto characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), file_chunk.size(), _file_native);
                        while (characters_read > 0)
                        {
                            core::buffer::append(_data, file_chunk.data(), static_cast<uint32_t>(characters_read));
                            characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), file_chunk.size(), _file_native);
                        }
                    }

                    fsetpos(_file_native, &current_pos);
                }

                return FileResource::data();
            }

            void write(core::data_view wdata) noexcept override
            {
                if (wdata.size() == 0)
                {
                    return;
                }

                if (_file_native == nullptr)
                {
                    fopen_s(&_file_native, core::string::begin(_native_path), "w+b");
                }

                if (_file_native != nullptr)
                {
                    fwrite(wdata.data(), sizeof(char), wdata.size(), _file_native);
                }
            }

            void flush() noexcept override
            {
                if (_file_native != nullptr)
                {
                    fflush(_file_native);
                }
            }

            virtual auto access_type() noexcept -> FileAccessType override { return FileAccessType::ReadWrite; }

        private:
            FILE* _file_native = nullptr;
        };

        namespace array = core::pod::array;

        void mount_directory(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, core::MessageBuffer& messages) noexcept
        {
            if (std::filesystem::is_directory(path) == false)
            {
                // #todo Warning
                return;
            }

            // Build the path
            path = std::filesystem::canonical(path);

            // Traverse the directory
            std::filesystem::recursive_directory_iterator directory_iterator{ path };
            for (auto&& native_entry : directory_iterator)
            {
                if (std::filesystem::is_regular_file(native_entry) == false)
                {
                    continue;
                }

                auto filepath = native_entry.path();
                auto fileextension = filepath.extension();
                if (fileextension == ".isrm")
                {
                    // #todo skip meta files by default, maybe add a most mount check for meta files without associated resources.
                    continue;
                }

                auto fullpath = std::filesystem::absolute(filepath).generic_string();
                auto relative_path_string = fullpath.substr(path.generic_string().length() + 1);

                Resource* result_object = nullptr;

                if (fileextension == ".isr")
                {
                    result_object = alloc.make<BakedFileResource>(
                        alloc,
                        URI{ scheme_directory, path.generic_string(), core::stringid(relative_path_string.c_str()) },
                        fullpath,
                        relative_path_string.c_str()
                    );
                }
                else
                {
                    auto fullpath_meta = std::filesystem::path{ fullpath }.concat(".isrm");
                    if (std::filesystem::is_regular_file(fullpath_meta) == false)
                    {
                        fullpath_meta = std::filesystem::path{};
                    }

                    result_object = alloc.make<FileResource>(
                        alloc,
                        URI{ scheme_directory, path.generic_string(), core::stringid(relative_path_string.c_str()) },
                        fullpath,
                        fullpath_meta.generic_string(),
                        relative_path_string.c_str()
                    );
                }

                array::push_back(entry_list, result_object);

                core::message::push(messages, resource::message::ModuleResourceMounted{ result_object });
            }
        }

        void mount_file(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, core::MessageBuffer& messages) noexcept
        {
            if (std::filesystem::is_regular_file(path) == false)
            {
                // #todo Warning
                return;
            }

            // Build the path
            auto canonical_path = std::filesystem::canonical(path);

            if (std::filesystem::is_regular_file(canonical_path))
            {
                auto filepath = std::move(canonical_path);
                auto fileextension = filepath.extension();
                if (fileextension == ".isrm")
                {
                    // #todo skip meta files by default, maybe add a most mount check for meta files without associated resources.
                    return;
                }

                auto fullpath = std::filesystem::canonical(filepath).generic_string();
                auto fullpath_meta = std::filesystem::path{ fullpath }.replace_extension(".isrm");
                if (std::filesystem::is_regular_file(fullpath_meta) == false)
                {
                    fullpath_meta = std::filesystem::path{};
                }

                auto* file_entry_object = alloc.make<FileResource>(
                    alloc,
                    URI{ scheme_file, fullpath },
                    fullpath,
                    fullpath_meta.generic_string(),
                    filepath.filename().generic_string());
                array::push_back(entry_list, static_cast<Resource*>(file_entry_object));

                core::message::push(messages, resource::message::ModuleResourceMounted{ file_entry_object });
            }
        }

        auto mount_writable_file(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, core::MessageBuffer& messages) noexcept -> FileResourceWritable*
        {
            auto directory_path = path.parent_path();
            if (std::filesystem::is_directory(directory_path) == false)
            {
                std::filesystem::create_directories(directory_path);
                IS_ASSERT(
                    std::filesystem::is_directory(directory_path),
                    "Failed to create directories for writable file! Path missing: {}",
                    directory_path.generic_string());
            }

            FILE* native_handle;
            fopen_s(&native_handle, path.generic_string().c_str(), "wb");

            IS_ASSERT(native_handle != nullptr, "Failed to open writable file '{}'!", path.generic_string());
            if (native_handle != nullptr)
            {
                fclose(native_handle);
            }

            // Build the path
            if (std::filesystem::is_regular_file(path))
            {
                auto filepath = std::filesystem::canonical(path);
                auto filename = filepath.filename().generic_string();

                auto fullpath = std::filesystem::canonical(filepath).generic_string();

                auto* file_entry_object = alloc.make<FileResourceWritable>(
                    alloc,
                    URI{ scheme_file, fullpath },
                    fullpath.c_str(),
                    filename.c_str());
                array::push_back(entry_list, static_cast<Resource*>(file_entry_object));

                core::message::push(messages, resource::message::ModuleResourceMounted{ file_entry_object });
                return file_entry_object;
            }

            return nullptr;
        }

    } // namespace detail

    FileSystem::FileSystem(core::allocator& alloc, core::StringView basedir) noexcept
        : _basedir{ alloc, basedir }
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
        core::memory::stack_allocator_512 stack_alloc;
        core::String uri_path{ stack_alloc };
        core::string::reserve(uri_path, 512);

        Resource* result{ nullptr };
        for (auto* res : _resources)
        {
            auto& res_uri = res->location();
            if (res_uri.scheme != uri.scheme)
            {
                continue;
            }

            uri_path = _basedir._data;
            core::path::join(uri_path, uri.path);
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

    auto FileSystem::open(URI const& uri, core::MessageBuffer& messages) noexcept -> OutputResource*
    {
        OutputResource* result{ nullptr };
        for (auto* res_obj : _resources)
        {
            auto* res = static_cast<detail::FileResource*>(res_obj);

            auto& res_uri = res->location();
            if (res_uri.scheme != uri.scheme)
            {
                continue;
            }

            auto uri_path = _basedir._data / std::filesystem::path{ core::string::begin(uri.path) };
            if (std::filesystem::is_regular_file(uri_path))
            {
                uri_path = std::filesystem::canonical(uri_path);
            }

            if (!core::string::equals(res_uri.path, uri_path.generic_string()))
            {
                continue;
            }

            if (res_uri.fragment != uri.fragment)
            {
                continue;
            }

            if (res->access_type() != detail::FileAccessType::ReadWrite)
            {
                continue;
            }

            result = static_cast<detail::FileResourceWritable*>(res);
        }

        if (result == nullptr)
        {
            IS_ASSERT(uri.scheme == resource::scheme_file, "Cannot open resource types for writing other than {}! got: {}", resource::scheme_file, uri.scheme);
            result = detail::mount_writable_file(_allocator, _basedir._data / std::filesystem::path{ uri.path }, _resources, messages);
        }

        return result;
    }

    auto FileSystem::mount(URI const& uri, core::MessageBuffer& messages) noexcept -> uint32_t
    {
        auto message_count = core::message::count(messages);

        if (uri.scheme == resource::scheme_file)
        {
            detail::mount_file(_allocator, std::filesystem::path{ _basedir._data } / uri.path, _resources, messages);
        }
        else if (uri.scheme == resource::scheme_directory)
        {
            detail::mount_directory(_allocator, std::filesystem::path{ _basedir._data } / uri.path, _resources, messages);
        }

        return core::message::count(messages) - message_count;
    }

} // namespace resource
