#include <resource/resource.hxx>
#include <resource/modules/dynlib_module.hxx>
#include "module_messages.hxx"

#include <core/allocators/proxy_allocator.hxx>
#include <core/allocators/stack_allocator.hxx>
#include <core/cexpr/stringid.hxx>
#include <core/string.hxx>

#include <core/pod/array.hxx>
#include <core/data/chunk.hxx>
#include <core/data/buffer.hxx>
#include <core/platform/windows.hxx>

#include <filesystem>
#include <cstdio>

namespace resource
{
    namespace detail
    {
        namespace array = core::pod::array;

        class DynamicLibraryResource final : public Resource
        {
        public:
            DynamicLibraryResource(core::allocator& alloc, const URI& uri, core::StringView native_filename) noexcept
                : _native_path{ alloc, uri.path }
                , _native_filename{ alloc, native_filename }
                , _uri{ uri.scheme, _native_path, uri.fragment }
                , _data{ alloc }
            {
            }

            ~DynamicLibraryResource() override = default;

            //! \brief The resource identifier.
            //! \remark This value can be seen as the absolute location to a specific resource.
            auto location() const noexcept -> const URI& override
            {
                return _uri;
            }

            //! \brief Returns the associated resource data.
            auto data() noexcept -> core::data_view override
            {
                return { };
            }

            auto metadata() noexcept -> core::data_view override
            {
                return { };
            }

            auto name() const noexcept -> core::StringView override
            {
                return _native_filename;
            }

        private:
            //! \brief The native filesystem path.
            core::String<> _native_path;
            core::String<> _native_filename;

            //! \brief The resource identifier.
            URI _uri;

            //! \brief The loaded file buffer.
            core::Buffer _data;
        };

        void mount_modules(core::allocator& alloc, std::filesystem::path path, core::pod::Array<Resource*>& entry_list, core::MessageBuffer& messages) noexcept
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
                if (std::filesystem::is_regular_file(native_entry) && native_entry.path().extension() == ".dll")
                {
                    auto filepath = native_entry.path();
                    auto filename = filepath.filename().generic_string();

                    auto fullpath = std::filesystem::canonical(filepath).generic_string();

                    bool found_entry = false;
                    for (const auto& entry : entry_list)
                    {
                        found_entry |= core::string::equals(entry->location().path, fullpath.c_str());
                    }

                    if (found_entry == false)
                    {
                        auto* module_entry_object = alloc.make<DynamicLibraryResource>(
                            alloc,
                            URI{ scheme_dynlib, fullpath },
                            filename.c_str());
                        array::push_back(entry_list, static_cast<Resource*>(module_entry_object));

                        core::message::push(messages, resource::message::ModuleResourceMounted{ module_entry_object });
                    }
                }
            }
        }

        auto get_application_dir() noexcept -> std::string
        {
            core::StackString<256> buffer = "";
            GetModuleFileName(NULL, core::string::begin(buffer), core::string::capacity(buffer));

            return std::filesystem::canonical(core::string::begin(buffer)).parent_path().generic_string();
        }

        auto get_working_directory() noexcept -> std::string
        {
            core::StackString<256> buffer = "";
            GetCurrentDirectory(core::string::capacity(buffer), core::string::begin(buffer));

            return std::filesystem::canonical(core::string::begin(buffer)).generic_string();
        }

    } // namespace detail

    DynLibSystem::DynLibSystem(core::allocator& alloc) noexcept
        : _allocator{ "dynlib-system", alloc }
        , _resources{ _allocator }
        , _app_dir{ detail::get_application_dir() }
        , _working_dir{ detail::get_working_directory() }
    {
        core::pod::array::reserve(_resources, 20);
    }

    DynLibSystem::~DynLibSystem() noexcept
    {
        for (auto* entry : _resources)
        {
            _allocator.destroy(entry);
        }
        core::pod::array::clear(_resources);
    }

    auto DynLibSystem::find(URI const&) noexcept -> Resource*
    {
        return nullptr;
    }

    auto DynLibSystem::mount(URI const& uri, core::MessageBuffer& messages) noexcept -> uint32_t
    {
        IS_ASSERT(uri.scheme == resource::scheme_dynlib, "Invalid URI scheme used mounting in the dynlib system.");

        core::StackString<64> config_directory{ core::string::data(uri.path) };
        config_directory += "/";
        config_directory += to_string(core::build::platform::current_platform.architecture);
        config_directory += "-";
        config_directory += to_string(core::build::configuration::current_config);

        // Mount DLL's in the application dir.
        if (initial_mount_finished == false)
        {
            detail::mount_modules(_allocator, _app_dir / std::filesystem::path{ ".." }, _resources, messages);
            initial_mount_finished = true;
        }

        detail::mount_modules(_allocator, std::filesystem::path{ _working_dir } / core::string::begin(config_directory), _resources, messages);
        return 0;
    }

} // namespace resource
