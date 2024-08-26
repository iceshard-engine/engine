/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_filelist.hxx"

namespace ice
{

    FileListResourceProvider::FileListResourceProvider(
        ice::Allocator& alloc,
        ice::Span<ice::ResourceFileEntry const> entries,
        ice::TaskScheduler* scheduler,
        ice::String virtual_hostname
    ) noexcept
        : _named_allocator{ alloc, "FileSystem" }
        , _allocator{ _named_allocator }
        , _file_paths{ _allocator }
        , _virtual_hostname{ virtual_hostname }
        , _resources{ _allocator }
        , _devui_widget{ create_filesystem_provider_devui(_allocator, _resources) }
    {
        ice::native_file::HeapFilePath file_path{ _allocator };
        for (ice::ResourceFileEntry const& entry : entries)
        {
            if (ice::path::is_absolute(entry.path) == false)
            {
                ice::native_file::path_from_string(file_path, entry.basepath);
                ice::native_file::path_join_string(file_path, entry.path);
                ice::path::normalize(file_path);
            }
            else
            {
                ice::native_file::HeapFilePath base_path{ _allocator };
                ice::native_file::path_from_string(base_path, entry.basepath);
                ice::native_file::path_from_string(file_path, entry.path);
                ice::path::normalize(base_path);
                ice::path::normalize(file_path);
                ICE_ASSERT_CORE(ice::string::starts_with((ice::WString)file_path, (ice::WString)base_path));
            }

            ice::ucount const basepath_size = ice::string::empty(entry.basepath)
                ? ice::string::size(entry.path) - ice::string::size(ice::path::filename(entry.path))
                : ice::string::size(entry.basepath);

            ice::array::push_back(_file_paths, { .path = file_path, .basepath_size = basepath_size, });
        }
    }

    FileListResourceProvider::~FileListResourceProvider() noexcept
    {
        for (FileSystemResource* res_entry : _resources)
        {
            _allocator.destroy(res_entry);
        }
    }

    auto FileListResourceProvider::schemeid() const noexcept -> ice::StringID
    {
        return ice::Scheme_File;
    }

    void FileListResourceProvider::create_resource_from_file(
        ice::native_file::FilePath base_path,
        ice::native_file::FilePath file_path
    ) noexcept
    {
        // Early out for metadata files.
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isrm"))
        {
            return;
        }

        ice::StackAllocator_1024 temp_alloc;
        ice::native_file::FilePath const uribase = ice::path::directory(base_path);
        ice::native_file::FilePath const datafile = file_path;
        ice::native_file::HeapFilePath metafile{ temp_alloc };
        ice::string::reserve(metafile, 512);
        ice::string::push_back(metafile, file_path);
        ice::string::push_back(metafile, ISP_PATH_LITERAL(".isrm"));

        ice::FileSystemResource* const resource = create_resources_from_loose_files(
            _allocator,
            base_path,
            uribase,
            metafile,
            datafile
        );

        if (resource != nullptr)
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
    }

    void FileListResourceProvider::initial_traverse() noexcept
    {
        [[maybe_unused]]
        std::atomic_uint32_t remaining = 0;
        for (ice::FileListEntry const& entry : _file_paths)
        {
            create_resource_from_file(
                ice::string::substr(entry.path, 0, entry.basepath_size),
                entry.path
            );
        }
    }

    auto FileListResourceProvider::collect(
        ice::Array<ice::Resource const*>& out_changes
    ) noexcept -> ice::ucount
    {
        IPT_ZONE_SCOPED;

        ice::array::reserve(out_changes, ice::array::count(out_changes) +  ice::hashmap::count(_resources));
        for (auto* resource : _resources)
        {
            ice::array::push_back(out_changes, resource);
        }
        return ice::hashmap::count(_resources);
    }

    auto FileListResourceProvider::refresh(
        ice::Array<ice::Resource const*>& out_changes
    ) noexcept -> ice::ResourceProviderResult
    {
        IPT_ZONE_SCOPED;
        if (ice::hashmap::empty(_resources))
        {
            initial_traverse();
            collect(out_changes);
        }
        return ResourceProviderResult::Success;
    }

    auto FileListResourceProvider::find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource const*
    {
        ICE_ASSERT(
            uri.scheme() == ice::stringid_hash(schemeid()),
            "Trying to find resource for URI that is not handled by this provider."
        );

        ice::FileSystemResource const* found_resource = nullptr;

        ice::HeapString<> predicted_path{ _allocator };
        for (ice::FileListEntry const& file_entry : _file_paths)
        {
            ice::native_file::path_to_string(file_entry.path, predicted_path);
            ice::path::normalize(predicted_path);

            ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });
            found_resource = ice::hashmap::get(_resources, resource_hash, nullptr);
            if (found_resource != nullptr)
            {
                break;
            }
        }
        return found_resource;
    }

    auto FileListResourceProvider::access_loose_resource(
        ice::Resource const* resource
    ) const noexcept -> ice::LooseResource const*
    {
        return static_cast<ice::LooseFilesResource const*>(resource);
    }

    void FileListResourceProvider::unload_resource(
        ice::Allocator& alloc,
        ice::Resource const* /*resource*/,
        ice::Memory memory
    ) noexcept
    {
        alloc.deallocate(memory);
    }

    auto FileListResourceProvider::load_resource(
        ice::Allocator& alloc,
        ice::Resource const* resource,
        ice::TaskScheduler& scheduler,
        ice::NativeAIO* nativeio
    ) const noexcept -> ice::Task<ice::Memory>
    {
        ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
        co_return co_await filesys_res->load_data(alloc, scheduler, nativeio);
    }

    auto FileListResourceProvider::resolve_relative_resource(
        ice::URI const& relative_uri,
        ice::Resource const* root_resource
    ) const noexcept -> ice::Resource const*
    {
        ice::u32 const origin_size = ice::string::size(root_resource->origin());

        ice::HeapString<> predicted_path{ _allocator };
        ice::string::reserve(predicted_path, origin_size + ice::string::size(relative_uri.path()));

        predicted_path = ice::string::substr(
            root_resource->origin(),
            0,
            origin_size - ice::string::size(
                ice::path::filename(root_resource->name())
            )
        );

        ice::path::join(predicted_path, relative_uri.path());
        ice::path::normalize(predicted_path);

        ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });

        ice::FileSystemResource const* found_resource = ice::hashmap::get(_resources, resource_hash, nullptr);
        if (found_resource != nullptr)
        {
            return found_resource;
        }
        else
        {
            return nullptr;
        }
    }

} // namespace ice

auto ice::create_resource_provider_files(
    ice::Allocator& alloc,
    ice::Span<ice::ResourceFileEntry const> entries,
    ice::TaskScheduler* scheduler,
    ice::String virtual_hostname
) noexcept -> ice::UniquePtr<ice::ResourceProvider>
{
    return ice::make_unique<ice::FileListResourceProvider>(alloc, alloc, entries, scheduler, virtual_hostname);
}
