/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_filelist.hxx"
#include <ice/string_utils.hxx>

namespace ice
{

    FileListResourceProvider::FileListResourceProvider(
        ice::Allocator& alloc,
        ice::Span<ice::ResourceFileEntry const> entries,
        ice::native_aio::AIOPort aioport,
        ice::String virtual_hostname
    ) noexcept
        : _named_allocator{ alloc, "FileSystem" }
        , _data_allocator{ alloc }
        , _file_paths{ _named_allocator }
        , _virtual_hostname{ virtual_hostname }
        , _aioport{ aioport }
        , _resources{ _data_allocator }
        , _resources_data{ _named_allocator }
        , _devui_widget{ create_filesystem_provider_devui(_named_allocator, _resources) }
    {
        ice::native_file::HeapFilePath file_path{ _named_allocator };
        for (ice::ResourceFileEntry const& entry : entries)
        {
            using enum native_file::PathFlags;
            if (ice::path::is_absolute(entry.path) == false)
            {
                file_path = ice::native_file::path_from_strings<Normalized>(
                    _named_allocator, entry.basepath, entry.path
                );
            }
            else
            {
                // Create temporary basepath from string
                ice::native_file::HeapFilePath base_path = ice::native_file::path_from_strings<Normalized>(
                    _named_allocator, entry.basepath
                );

                file_path = ice::native_file::path_from_strings<Normalized>(
                    _named_allocator, entry.path
                );

                ICE_ASSERT_CORE(ice::string::starts_with((ice::native_file::FilePath)file_path, (ice::native_file::FilePath)base_path));
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
            _named_allocator.destroy(res_entry);
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

        // Handle full .isr files
        ice::FileSystemResource* resource = nullptr;
        if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isr"))
        {
            ice::HeapString<> uri_base{ _named_allocator };
            ice::string::push_format(uri_base, "file://{}/", _virtual_hostname);

            resource = create_resource_from_baked_file(_named_allocator, *this, uri_base, file_path);
        }
        else
        {

            ice::StackAllocator_1024 temp_alloc;
            ice::native_file::FilePath const uribase = ice::path::directory(base_path);
            ice::native_file::FilePath const datafile = file_path;
            ice::native_file::HeapFilePath metafile{ temp_alloc };
            ice::string::reserve(metafile, 512);
            ice::string::push_back(metafile, file_path);
            ice::string::push_back(metafile, ISP_PATH_LITERAL(".isrm"));

            resource = create_resources_from_loose_files(
                _named_allocator,
                *this,
                base_path,
                uribase,
                metafile,
                datafile
            );
        }

        if (resource != nullptr)
        {
            resource->data_index = ice::array::count(_resources_data);
            ice::array::push_back(_resources_data, ice::Memory{});

            ice::u64 const hash = ice::hash(resource->uri().path());
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
        ice::Array<ice::Resource*>& out_changes
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
        ice::Array<ice::Resource*>& out_changes
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

        ice::HeapString<> predicted_path{ (ice::Allocator&)_named_allocator };
        for (ice::FileListEntry const& file_entry : _file_paths)
        {
            ice::native_file::path_to_string(file_entry.path, predicted_path);
            ice::path::normalize(predicted_path);

            ice::u64 const resource_hash = ice::hash(uri.path());
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
        ice::Resource const* resource
    ) noexcept
    {
        ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
        _data_allocator.deallocate(std::exchange(_resources_data[filesys_res->data_index], {}));
    }

    auto FileListResourceProvider::load_resource(
        ice::Resource const* resource,
        ice::String fragment
    ) noexcept -> ice::TaskExpected<ice::Data>
    {
        ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
        co_return co_await filesys_res->load_data(
            _data_allocator, _resources_data[filesys_res->data_index], fragment, _aioport
        );
    }

    auto FileListResourceProvider::resolve_relative_resource(
        ice::URI const& relative_uri,
        ice::Resource const* root_resource
    ) const noexcept -> ice::Resource const*
    {
        ice::u32 const origin_size = ice::string::size(root_resource->origin());

        ice::HeapString<> predicted_path{ (ice::Allocator&) _data_allocator };
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
    ice::native_aio::AIOPort aioport,
    ice::String virtual_hostname
) noexcept -> ice::UniquePtr<ice::ResourceProvider>
{
    return ice::make_unique<ice::FileListResourceProvider>(alloc, alloc, entries, aioport, virtual_hostname);
}
