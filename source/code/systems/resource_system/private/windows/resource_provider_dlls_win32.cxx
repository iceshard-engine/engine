/// Copyright 2022 - 2022, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/resource_provider.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string/heap_string.hxx>

#include "resource_dll_win32.hxx"
#include "resource_utils_win32.hxx"

#if ISP_WINDOWS

namespace ice
{

    class ResourceProvider_Win32Dlls final : public ice::ResourceProvider
    {
    public:
        ResourceProvider_Win32Dlls(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept
            : _allocator{ alloc }
            , _base_path{ _allocator }
            , _resources{ _allocator }
        {
            ice::win32::utf8_to_wide_append(path, _base_path);

            ice::u32 at = ice::string::find_first_of(_base_path, L'\\');
            while (at != ice::String_NPos)
            {
                _base_path[at] = L'/';
                at = ice::string::find_first_of(_base_path, L'\\');
            }
        }

        ~ResourceProvider_Win32Dlls() noexcept override
        {
            for (Resource_v2* res_entry : _resources)
            {
                _allocator.destroy(res_entry);
            }
        }

        auto schemeid() const noexcept -> ice::StringID override
        {
            return ice::Scheme_Dynlib;
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
                        // TODO: Decide how to work with string static arrays.
                        ice::WString const file_name = (ice::wchar const*)file_data.cFileName;

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

            ice::string::push_back(directory_path, L"*.dll");

            WIN32_FIND_DATA file_data;
            HANDLE const handle = FindFirstFileW(
                ice::string::begin(directory_path),
                &file_data
            );

            ice::string::resize(directory_path, directory_path_size);

            if (handle != INVALID_HANDLE_VALUE)
            {

                do
                {
                    // TODO: Decide how to work with string static arrays.
                    ice::WString const file_name = (ice::wchar const*)file_data.cFileName;

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
            ice::path::win32::join(dir_tracker, L"."); // Ensure we end with a '/' character

            // An allocator for temporary paths, 1024 bytes should suffice for 2x256 wchar_t long paths
            ice::StackAllocator_1024 temp_path_alloc;

            auto fncb = [&](ice::WString file) noexcept
            {
                temp_path_alloc.reset();

                ice::HeapString<ice::wchar> file_path{ temp_path_alloc, file };
                ice::Resource_v2* const resource = create_resource_from_dll_path(
                    _allocator,
                    file_path
                );

                if (resource != nullptr)
                {
                    ice::hashmap::set(
                        _resources,
                        ice::hash(resource->name()),
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

        auto load_resource(
            ice::Allocator& alloc,
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<ice::Memory> override
        {
            // Cannot load DLL's in this way.
            co_return ice::Memory{ };
        }

        auto release_resource(
            ice::Resource_v2 const* resource,
            ice::TaskScheduler_v2& scheduler
        ) noexcept -> ice::Task<>
        {
            // Cannot release DLL's in this way.
            co_return;
        }

    private:
        ice::Allocator& _allocator;
        ice::HeapString<ice::wchar> _base_path;

        ice::HashMap<ice::Resource_v2*> _resources;
    };

    auto create_resource_provider_dlls(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>
    {
        return ice::make_unique<ice::ResourceProvider_Win32Dlls>(alloc, alloc, path);
    }

} // namespace ice

#endif // #if ISP_WINDOWS
