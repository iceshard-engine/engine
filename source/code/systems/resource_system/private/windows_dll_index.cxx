#include <ice/resource_index.hxx>
#include <ice/resource_query.hxx>
#include <ice/resource_meta.hxx>
#include <ice/os/windows.hxx>
#include <ice/unique_ptr.hxx>
#include <ice/resource_index.hxx>
#include <ice/pod/array.hxx>
#include "path_utils.hxx"

#include <filesystem>

#if ISP_WINDOWS

namespace ice
{

    namespace detail
    {

        class DllResource final : public Resource
        {
        public:
            DllResource(ice::HeapString<> file_path, ice::URI const& uri) noexcept
                : ice::Resource{ }
                , _file_path{ ice::move(file_path) }
                , _uri{ uri }
            { }

            ~DllResource() noexcept override = default;

            auto name() const noexcept -> ice::String override
            {
                return ice::path::filename(_file_path);
            }

            auto location() const noexcept -> ice::URI const& override
            {
                return _uri;
            }

            auto metadata() const noexcept -> ice::Metadata const& override
            {
                return _metadata_view;
            }

            auto data() noexcept -> ice::Data override
            {
                return { };
            }

        private:
            ice::HeapString<> _file_path;
            ice::URI _uri;

            ice::Metadata _metadata_view;
        };

    } // namespace detail

    class WindowsDllIndex final : public ResourceIndex
    {
    public:
        WindowsDllIndex(ice::Allocator& alloc, ice::String base_path) noexcept;
        ~WindowsDllIndex() noexcept;

        auto find_relative(
            ice::Resource const& root_resource,
            ice::String path
        ) noexcept -> ice::Resource* override;

        bool query_changes(ice::ResourceQuery& query) noexcept override;
        bool mount(ice::URI const& uri) noexcept override;
        auto request(ice::URI const& uri) noexcept -> ice::Resource* override;
        bool release(ice::URI const& uri) noexcept override;

    private:
    private:
        ice::Allocator& _allocator;
        ice::HeapString<> _base_path;

        ice::pod::Array<ice::Resource*> _resources;

        ice::pod::Array<ice::ResourceEvent> _events;
        ice::pod::Array<ice::Resource*> _event_objects;
    };

    WindowsDllIndex::WindowsDllIndex(ice::Allocator& alloc, ice::String base_path) noexcept
        : ice::ResourceIndex{ }
        , _allocator{ alloc }
        , _base_path{ _allocator, base_path }
        , _resources{ _allocator }
        , _events{ _allocator }
        , _event_objects{ _allocator }
    {
    }

    WindowsDllIndex::~WindowsDllIndex() noexcept
    {
        for (ice::Resource* res : _resources)
        {
            _allocator.destroy(res);
        }
    }

    auto WindowsDllIndex::find_relative(ice::Resource const& root_resource, ice::String path) noexcept -> ice::Resource*
    {
        return nullptr;
    }

    bool WindowsDllIndex::query_changes(ice::ResourceQuery& query) noexcept
    {
        if (ice::pod::array::any(_events))
        {
            query.events = ice::move(_events);
            query.objects = ice::move(_event_objects);
            return true;
        }
        return false;
    }

    bool WindowsDllIndex::mount(ice::URI const& uri) noexcept
    {
        if (ice::stringid_hash(uri.scheme) != ice::stringid_hash(ice::scheme_dynlib))
        {
            return false;
        }

        ice::HeapString<> directory_path{ _allocator, _base_path };
        ice::path::join(directory_path, uri.path);

        std::filesystem::path mount_path = std::filesystem::weakly_canonical(ice::String{ directory_path });
        if (std::filesystem::is_directory(mount_path) == false)
        {
            // #todo warning
            return false;
        }

        std::filesystem::recursive_directory_iterator directory_iterator{ mount_path };
        for (auto const& native_entry : directory_iterator)
        {
            if (std::filesystem::is_regular_file(native_entry))
            {
                ice::HeapString<> file_path{ _allocator, native_entry.path().generic_string() };
                ice::String file_name = ice::path::filename(file_path);
                ice::String file_extension = ice::path::extension(file_name);

                if (file_extension == ".dll")
                {
                    ice::Resource* resource = _allocator.make<detail::DllResource>(
                        ice::move(file_path),
                        ice::URI{ ice::scheme_dynlib, file_path }
                    );

                    ice::pod::array::push_back(_resources, resource);
                    ice::pod::array::push_back(_events, ice::ResourceEvent::Added);
                    ice::pod::array::push_back(_event_objects, resource);
                }
            }
        }

        directory_path = mount_path.generic_string();
        return true;
    }

    auto WindowsDllIndex::request(ice::URI const& uri) noexcept -> ice::Resource*
    {
        // #todo assert uri.scheme == ice::scheme_dynlib
        // #todo assert uri.fragment == ice::stringid_invalid

        ice::Resource* result = nullptr;
        for (ice::Resource* res : _resources)
        {
            ice::URI const& possible_uri = res->location();
            if (possible_uri.path == uri.path)
            {
                result = res;
                break;
            }
        }
        return result;
    }

    bool WindowsDllIndex::release(ice::URI const& /*uri*/) noexcept
    {
        return false;
    }

    auto create_dynlib_index(ice::Allocator& alloc, ice::String app_relative_path) noexcept -> ice::UniquePtr<ice::ResourceIndex>
    {
        return ice::make_unique<ice::ResourceIndex, ice::WindowsDllIndex>(alloc, alloc, app_relative_path);
    }

} // namespace ice

#endif // ISP_WINDOWS
