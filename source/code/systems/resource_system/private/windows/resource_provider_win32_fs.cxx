#include <ice/resource_provider.hxx>
#include <ice/resource_provider_action.hxx>
#include <ice/resource_action.hxx>
#include <ice/memory/stack_allocator.hxx>
#include <ice/os/windows.hxx>
#include <ice/heap_string.hxx>

#include "resource_loose_files_win32.hxx"
#include "../path_utils.hxx"

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
        {
            ice::utf8_to_wide_append(path, _base_path);
        }

        bool query_changes(ice::pod::Array<ice::Resource_v2 const*>& out_changes) const noexcept
        {
            return false;
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

        auto refresh() noexcept -> ice::Task<ice::ResourceProviderResult>
        {
            ice::HeapString<wchar_t> dir_tracker = _base_path;
            ice::path::join(dir_tracker, L"."); // Ensure we end with a '/' character

            ice::pod::Array<ice::Resource_v2*> new_resources{ _allocator };

            // An allocator for temporary paths, 1024 bytes should suffice for 2x256 wchar_t long paths
            ice::memory::StackAllocator_1024 temp_path_alloc;
            auto fncb = [&](ice::WString file) noexcept
            {
                temp_path_alloc.clear();

                ice::HeapString<wchar_t> meta_file{ temp_path_alloc, file };
                ice::HeapString<wchar_t> data_file{ temp_path_alloc, ice::string::substr(file, 0, ice::string::find_last_of(file, L'.')) };

                ice::pod::array::push_back(
                    new_resources,
                    create_resource_from_loose_files(_allocator, _base_path, meta_file, data_file)
                );
            };

            traverse_directory(dir_tracker, fncb);

            for (ice::Resource_v2* res : new_resources)
            {
                _allocator.destroy(res);
            }

            co_return ResourceProviderResult::Success;
        }

        auto reset() noexcept -> ice::Task<ice::ResourceProviderResult>
        {
            co_return ResourceProviderResult::Skipped;
        }

    private:
        ice::Allocator& _allocator;
        ice::HeapString<wchar_t> _base_path;
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
