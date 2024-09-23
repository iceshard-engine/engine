/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_provider_custom.hxx"

namespace ice
{

    CustomResourceProvider::CustomResourceProvider(ice::Allocator& alloc) noexcept
        : _named_allocator{ alloc, "Custom" }
        , _allocator{ _named_allocator }
        , _resources{ _allocator }
        , _devui_widget{ create_filesystem_provider_devui(_allocator, _resources) }
    {
    }

    CustomResourceProvider::~CustomResourceProvider() noexcept
    {
        for (FileSystemResource* res_entry : _resources)
        {
            _allocator.destroy(res_entry);
        }
    }

    auto CustomResourceProvider::schemeid() const noexcept -> ice::StringID
    {
        return ice::Scheme_File;
    }

    void CustomResourceProvider::create_resource_from_file(
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

    auto CustomResourceProvider::collect(
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

    auto CustomResourceProvider::refresh(
        ice::Array<ice::Resource const*>& out_changes
    ) noexcept -> ice::ResourceProviderResult
    {
        IPT_ZONE_SCOPED;
        if (ice::hashmap::empty(_resources))
        {
            collect(out_changes);
        }
        return ResourceProviderResult::Success;
    }

    auto CustomResourceProvider::find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource const*
    {
        ICE_ASSERT(
            uri.scheme() == ice::stringid_hash(schemeid()),
            "Trying to find resource for URI that is not handled by this provider."
        );
        return nullptr;
    }

    auto CustomResourceProvider::access_loose_resource(
        ice::Resource const* resource
    ) const noexcept -> ice::LooseResource const*
    {
        return static_cast<ice::LooseFilesResource const*>(resource);
    }

    void CustomResourceProvider::unload_resource(
        ice::Allocator& alloc,
        ice::Resource const* /*resource*/,
        ice::Memory memory
    ) noexcept
    {
        alloc.deallocate(memory);
    }

    auto CustomResourceProvider::load_resource(
        ice::Resource const* resource,
        ice::String fragment
    ) noexcept -> ice::TaskExpected<ice::Data>
    {
        co_return {};
    }

    auto CustomResourceProvider::resolve_relative_resource(
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

auto ice::create_resource_provider_custom(
    ice::Allocator& alloc
) noexcept -> ice::UniquePtr<ice::ResourceProvider>
{
    return ice::make_unique<ice::CustomResourceProvider>(alloc, alloc);
}
