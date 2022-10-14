#include <ice/resource_provider.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/os/windows.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>

#include "resource_loose_files_win32.hxx"
#include "resource_utils_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    class ResourceProvider_Win32Filesystem final : public ice::ResourceProvider
    {
    public:
        ResourceProvider_Win32Filesystem(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept
            : _allocator{ alloc }
            , _base_path{ _allocator }
            , _resources{ _allocator }
        {
            ice::win32::utf8_to_wide_append(path, _base_path);
        }

        ~ResourceProvider_Win32Filesystem() noexcept override
        {
            for (Resource_v2* res_entry : _resources)
            {
                _allocator.destroy(res_entry);
            }
        }

        auto schemeid() const noexcept -> ice::StringID override
        {
            return ice::Scheme_File;
        }

        template<typename Fn>
        void traverse_directory(ice::HeapString<wchar_t>& directory_path, Fn&& callback) noexcept
        {
            ice::u32 const directory_path_size = ice::string::size(directory_path);

            {
                ice::string::push_back(directory_path, L'*');

                WIN32_FIND_DATA file_data;
                HANDLE const handle = FindFirstFileW(
                    ice::string::begin(directory_path),
                    &file_data
                );

                ice::string::pop_back(directory_path); // remove the '*' from the string

                if (handle != INVALID_HANDLE_VALUE)
                {

                    do
                    {
                        ice::WString const file_name = file_data.cFileName;

                        if (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                        {
                            if (file_name == L"." || file_name == L"..")
                            {
                                continue;
                            }

                            ice::path::win32::join(directory_path, file_name);
                            ice::string::push_back(directory_path, L'/');
                            traverse_directory(directory_path, callback);
                            ice::string::resize(directory_path, directory_path_size);
                        }

                    } while (FindNextFileW(handle, &file_data) != FALSE);
                    FindClose(handle);
                }
            }

            ice::string::push_back(directory_path, L"*.isrm");

            WIN32_FIND_DATA file_data;
            HANDLE const handle = FindFirstFileW(
                ice::string::begin(directory_path),
                &file_data
            );

            ice::string::pop_back(directory_path, ice::count(L"*.isrm") - 1); // remove the '*' from the string

            if (handle != INVALID_HANDLE_VALUE)
            {

                do
                {
                    ice::WString const file_name = file_data.cFileName;

                    if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    {
                        ice::path::win32::join(directory_path, file_name);
                        callback(directory_path);
                        ice::string::resize(directory_path, directory_path_size);
                    }
                } while (FindNextFileW(handle, &file_data) != FALSE);
                FindClose(handle);
            }
        }

        // GitHub Issue: #108
        void ISATTR_NOINLINE initial_traverse() noexcept
        {
            ice::HeapString<ice::wchar> dir_tracker = _base_path;
            ice::string::reserve(dir_tracker, 512);
            ice::path::win32::join(dir_tracker, L"."); // Ensure we end with a '/' character

            ice::WString uri_base_path = ice::path::win32::directory(ice::path::win32::directory(dir_tracker));

            ice::Array<ice::Resource_Win32*> resources{ _allocator };
            ice::array::reserve(resources, 10);

            // An allocator for temporary paths, 1024 bytes should suffice for 2x256 wchar_t long paths
            ice::StackAllocator_1024 temp_path_alloc;
            auto fncb = [&](ice::WString file) noexcept
            {
                temp_path_alloc.reset();

                ice::HeapString<ice::wchar> meta_file{ temp_path_alloc, file };
                ice::HeapString<ice::wchar> data_file{ temp_path_alloc, ice::string::substr(file, 0, ice::string::find_last_of(file, L'.')) };

                ice::array::clear(resources);

                create_resources_from_loose_files(
                    _allocator,
                    _base_path,
                    uri_base_path,
                    meta_file,
                    data_file,
                    resources
                );

                for (ice::Resource_Win32* resource : resources)
                {
                    ice::u64 const hash = ice::hash(resource->origin());
                    ICE_ASSERT(
                        ice::hashmap::has(_resources, hash) == false,
                        "A resource cannot be a explicit resource AND part of another resource."
                    );

                    ice::hashmap::set(
                        _resources,
                        hash,
                        resource
                    );
                }
            };

            traverse_directory(dir_tracker, fncb);
        }

        auto query_resources(
            ice::Array<ice::Resource_v2 const*>& out_changes
        ) const noexcept -> ice::u32 override
        {
            for (Resource_v2 const* entry : _resources)
            {
                ice::array::push_back(out_changes, entry);
            }
            return 0;
        }

        auto refresh() noexcept -> ice::Task<ice::ResourceProviderResult> override
        {
            if (ice::hashmap::empty(_resources))
            {
                initial_traverse();
            }

            co_return ResourceProviderResult::Success;
        }

        auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource_v2 const* override
        {
            ICE_ASSERT(
                uri.scheme == ice::stringid_hash(schemeid()),
                "Trying to find resource for URI that is not handled by this provider."
            );

            ice::u32 const origin_size = ice::string::size(uri.path);

            ice::HeapString<> predicted_path{ _allocator, };
            ice::string::reserve(predicted_path, origin_size + ice::string::size(_base_path));

            ice::win32::wide_to_utf8(_base_path, predicted_path);
            ice::path::join(predicted_path, ".."); // Remove one directory (because it's the common value of the base path and the uri path)
            ice::path::join(predicted_path, uri.path);
            ice::path::normalize(predicted_path);

            ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });

            ice::Resource_Win32 const* found_resource = ice::hashmap::get(_resources, resource_hash, nullptr);
            if (found_resource != nullptr)
            {
                return found_resource;
            }
            else
            {
                return nullptr;
            }
        }

        auto load_resource(
            ice::Allocator& alloc,
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<ice::Memory> override
        {
            ice::Resource_Win32 const* const win_res = static_cast<ice::Resource_Win32 const*>(resource);
            co_return co_await win_res->load_data_for_flags(alloc, ResourceFlags::None, scheduler);
        }

        auto release_resource(
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<>
        {
            ice::Resource_Win32 const* const win_res = static_cast<ice::Resource_Win32 const*>(resource);
            ice::u64 const hash = ice::hash(win_res->name());

            ice::Resource_Win32* win_res_mut = ice::hashmap::get(_resources, hash, nullptr);
            ICE_ASSERT(win_res == win_res_mut, "Trying to delete resource with similar name but object pointers differ!");

            ice::hashmap::remove(
                _resources,
                hash
            );

            _allocator.destroy(win_res_mut);
            co_return;
        }

        auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource_v2 const* root_resource
        ) const noexcept -> ice::Resource_v2 const* override
        {
            ice::u32 const origin_size = ice::string::size(root_resource->origin());

            ice::HeapString<> predicted_path{ _allocator, };
            ice::string::reserve(predicted_path, origin_size + ice::string::size(relative_uri.path));

            predicted_path = ice::string::substr(
                root_resource->origin(),
                0,
                origin_size - ice::string::size(ice::path::filename(root_resource->name()))
            );

            ice::path::join(predicted_path, relative_uri.path);
            ice::path::normalize(predicted_path);

            ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });

            ice::Resource_Win32 const* found_resource = ice::hashmap::get(_resources, resource_hash, nullptr);
            if (found_resource != nullptr)
            {
                return found_resource;
            }
            else
            {
                return nullptr;
            }
        }

    private:
        ice::Allocator& _allocator;
        ice::HeapString<wchar_t> _base_path;

        ice::HashMap<ice::Resource_Win32*> _resources;
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>
    {
        return ice::make_unique<ice::ResourceProvider_Win32Filesystem>(alloc, alloc, path);
    }

} // namespace ice

#endif // #if ISP_WINDOWS
