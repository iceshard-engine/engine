#include <ice/resource_provider.hxx>
#include <ice/resource_action.hxx>
#include <ice/task_scheduler.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/os/windows.hxx>
#include <ice/heap_string.hxx>
#include <ice/pod/hash.hxx>

#include "resource_loose_files_win32.hxx"
#include "resource_utils_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    class ResourceProvider_Win32Filesystem final : public ice::ResourceProvider_v2
    {
    public:
        ResourceProvider_Win32Filesystem(
            ice::Allocator& alloc,
            ice::Utf8String path
        ) noexcept
            : _allocator{ alloc }
            , _base_path{ _allocator }
            , _resources{ _allocator }
        {
            ice::utf8_to_wide_append(path, _base_path);
        }

        ~ResourceProvider_Win32Filesystem() noexcept override
        {
            for (auto const& res_entry : _resources)
            {
                _allocator.destroy(res_entry.value);
            }
        }

        template<typename Fn>
        void traverse_directory(ice::HeapString<wchar_t>& directory_path, Fn&& callback) noexcept
        {
            ice::u32 const directory_path_size = ice::string::size(directory_path);

            {
                ice::string::push_back(directory_path, L'*');

                WIN32_FIND_DATA file_data;
                HANDLE const handle = FindFirstFileW(
                    ice::string::data(directory_path),
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

                            ice::path::join(directory_path, file_name);
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
                ice::string::data(directory_path),
                &file_data
            );

            ice::string::pop_back(directory_path, ice::size(L"*.isrm") - 1); // remove the '*' from the string

            if (handle != INVALID_HANDLE_VALUE)
            {

                do
                {
                    ice::WString const file_name = file_data.cFileName;

                    if ((file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                    {
                        ice::path::join(directory_path, file_name);
                        callback(directory_path);
                        ice::string::resize(directory_path, directory_path_size);
                    }
                } while (FindNextFileW(handle, &file_data) != FALSE);
                FindClose(handle);
            }
        }

        void initial_traverse() noexcept
        {
            ice::HeapString<wchar_t> dir_tracker = _base_path;
            ice::path::join(dir_tracker, L"."); // Ensure we end with a '/' character

            // An allocator for temporary paths, 1024 bytes should suffice for 2x256 wchar_t long paths
            ice::memory::StackAllocator_1024 temp_path_alloc;
            auto fncb = [&](ice::WString file) noexcept
            {
                temp_path_alloc.clear();

                ice::HeapString<wchar_t> meta_file{ temp_path_alloc, file };
                ice::HeapString<wchar_t> data_file{ temp_path_alloc, ice::string::substr(file, 0, ice::string::find_last_of(file, L'.')) };

                ice::Resource_Win32* const resource = create_resource_from_loose_files(
                    _allocator,
                    _base_path,
                    meta_file,
                    data_file
                );

                if (resource != nullptr)
                {
                    ice::pod::hash::set(
                        _resources,
                        ice::hash(resource->name()),
                        resource
                    );
                }
            };

            traverse_directory(dir_tracker, fncb);
        }

        auto query_resources(
            ice::pod::Array<ice::Resource_v2 const*>& out_changes
        ) const noexcept -> ice::u32 override
        {
            for (auto const& entry : _resources)
            {
                ice::pod::array::push_back(out_changes, entry.value);
            }
            return 0;
        }

        auto refresh() noexcept -> ice::Task<ice::ResourceProviderResult>
        {
            if (ice::pod::hash::empty(_resources))
            {
                initial_traverse();
            }

            co_return ResourceProviderResult::Success;
        }

        auto load_resource(
            ice::Allocator& alloc,
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<ice::Memory> override
        {
            ice::Resource_Win32 const* const win_res = static_cast<ice::Resource_Win32 const*>(resource);
            co_return co_await win_res->load_data_for_flags(alloc, 0, scheduler);
        }

        auto release_resource(
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<>
        {
            ice::Resource_Win32 const* const win_res = static_cast<ice::Resource_Win32 const*>(resource);
            ice::u64 const hash = ice::hash(win_res->name());

            ice::Resource_Win32* win_res_mut = ice::pod::hash::get(_resources, hash, nullptr);
            ICE_ASSERT(win_res == win_res_mut, "Trying to delete resource with similar name but object pointers differ!");

            ice::pod::hash::remove(
                _resources,
                hash
            );

            _allocator.destroy(win_res_mut);
            co_return;
        }

    private:
        ice::Allocator& _allocator;
        ice::HeapString<wchar_t> _base_path;

        ice::pod::Hash<ice::Resource_Win32*> _resources;
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Utf8String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider_v2>
    {
        return ice::make_unique<ice::ResourceProvider_v2, ice::ResourceProvider_Win32Filesystem>(alloc, alloc, path);
    }

} // namespace ice

#endif // #if ISP_WINDOWS
