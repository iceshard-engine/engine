/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
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
            ice::destroy_resource_object(_allocator, res_entry);
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
        if (file_path.extension() == ISP_PATH_LITERAL(".isrm"))
        {
            return;
        }

        ice::StackAllocator_1024 temp_alloc;
        ice::native_file::FilePath const uribase = base_path.directory();
        ice::native_file::FilePath const datafile = file_path;
        ice::native_file::HeapFilePath metafile{ temp_alloc };
        metafile.reserve(512);
        metafile.push_back(file_path);
        metafile.push_back(ISP_PATH_LITERAL(".isrm"));

        ice::FileSystemResource* const resource = create_resources_from_loose_files(
            _allocator,
            *this,
            base_path,
            uribase,
            metafile,
            datafile
        );

        if (resource != nullptr)
        {
            ICE_ASSERT(
                _resources.missing(resource->origin()),
                "A resource cannot be a explicit resource AND part of another resource."
            );

            _resources.set(
                resource->origin(),
                resource
            );
        }
    }

    auto CustomResourceProvider::collect(
        ice::Array<ice::Resource*>& out_changes
    ) noexcept -> ice::u32
    {
        IPT_ZONE_SCOPED;

        out_changes.reserve(out_changes.size() + _resources.size());
        for (auto* resource : _resources)
        {
            out_changes.push_back(resource);
        }
        return _resources.size().u32();
    }

    auto CustomResourceProvider::refresh(
        ice::Array<ice::Resource*>& out_changes
    ) noexcept -> ice::ResourceProviderResult
    {
        IPT_ZONE_SCOPED;
        if (_resources.is_empty())
        {
            collect(out_changes);
        }
        return ResourceProviderResult::Success;
    }

    auto CustomResourceProvider::find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource*
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
        ICE_ASSERT_CORE(false); // TODO: Check if this should be here.
        return static_cast<ice::LooseFilesResource const*>(resource);
    }

    void CustomResourceProvider::unload_resource(
        ice::Resource const* /*resource*/
    ) noexcept
    {
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
        ice::ncount const origin_size = root_resource->origin().size();

        ice::HeapPath predicted_path{ _allocator };
        predicted_path.reserve(origin_size + relative_uri.path().size());

        predicted_path = root_resource->origin().substr(
            0, origin_size - ice::Path{ root_resource->name() }.filename().size()
        );

        predicted_path.join(relative_uri.path());
        predicted_path.normalize();

        ice::u64 const resource_hash = ice::hash(ice::String{ predicted_path });

        ice::FileSystemResource const* found_resource = _resources.get(resource_hash, nullptr);
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
