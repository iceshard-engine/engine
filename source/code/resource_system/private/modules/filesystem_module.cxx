#include <resource/resource.hxx>
#include <resource/modules/filesystem_module.hxx>

#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/string.hxx>

#include <core/pod/array.hxx>
#include <core/data/chunk.hxx>
#include <core/data/buffer.hxx>

#include <filesystem>
#include <cstdio>

namespace resource
{
    namespace detail
    {

        class FileResource final : public Resource
        {
        public:
            FileResource(core::allocator& alloc, const URI& uri, core::StringView<> native_path) noexcept
                : _native_path{ alloc, native_path._data }
                , _path{ alloc, uri.path._data }
                , _uri{ uri.scheme, _path, uri.fragment }
                , _data{ alloc }
            { }

            ~FileResource() override = default;

            //! \brief The resource identifier.
            //! \remark This value can be seen as the absolute location to a specific resource.
            auto location() const noexcept -> const URI& override
            {
                return _uri;
            }

            //! \brief Returns the associated resource data.
            auto data() noexcept -> core::data_view override
            {
                if (core::buffer::empty(_data))
                {
                    FILE* file_native = nullptr;
                    fopen_s(&file_native, core::string::begin(_native_path), "rb");

                    if (file_native)
                    {
                        core::memory::stack_allocator_4096 kib4_alloc;
                        core::data_chunk file_chunk{ kib4_alloc, 1024u * 4u };

                        auto characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), file_chunk.size(), file_native);
                        while (characters_read > 0)
                        {
                            core::buffer::append(_data, file_chunk.data(), static_cast<uint32_t>(characters_read));
                            characters_read = fread_s(file_chunk.data(), file_chunk.size(), sizeof(char), file_chunk.size(), file_native);
                        }

                        fclose(file_native);
                    }
                }
                return _data;
            }

        private:
            //! \brief The native filesystem path.
            core::String<> _native_path;

            //! \brief The resource path.
            core::String<> _path;

            //! \brief The resource identifier.
            URI _uri;

            //! \brief The loaded file buffer.
            core::Buffer _data;
        };


        namespace array = core::pod::array;

        void mount_directory(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, std::function<void(Resource*)> callback) noexcept
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
                if (std::filesystem::is_regular_file(native_entry))
                {
                    auto filepath = native_entry.path();
                    auto filename = filepath.filename().generic_string();

                    auto fullpath = std::filesystem::canonical(filepath).generic_string();

                    auto relative_path = std::filesystem::relative(fullpath, path);
                    auto relative_path_string = relative_path.generic_string();

                    auto* dir_entry_object = alloc.make<FileResource>(alloc, URI{ scheme_directory, path.generic_string().c_str(), core::cexpr::stringid(relative_path_string.c_str()) }, fullpath.c_str());
                    array::push_back(entry_list, static_cast<Resource*>(dir_entry_object));
                    callback(dir_entry_object);
                }
            }
        }

        void mount_file(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, std::function<void(Resource*)> callback) noexcept
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
                auto filename = filepath.filename().generic_string();

                auto fullpath = std::filesystem::canonical(filepath).generic_string();

                auto* file_entry_object = alloc.make<FileResource>(alloc, URI{ scheme_file, fullpath.c_str() }, fullpath.c_str());
                array::push_back(entry_list, static_cast<Resource*>(file_entry_object));
                callback(file_entry_object);
            }
        }

    } // namespace detail

    FileSystem::FileSystem(core::allocator& alloc, core::StringView<> basedir) noexcept
        : _basedir{ alloc, core::string::begin(basedir) }
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

            auto uri_path = std::filesystem::canonical(_basedir._data / std::filesystem::path{ core::string::begin(uri.path) }).generic_string();
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
            detail::mount_file(_allocator, std::filesystem::path{ _basedir._data } / core::string::begin(uri.path), _resources, callback);
        }
        else if (uri.scheme == resource::scheme_directory)
        {
            detail::mount_directory(_allocator, std::filesystem::path{ _basedir._data } / core::string::begin(uri.path), _resources, callback);
        }
        return 0;
    }

} // namespace resource
