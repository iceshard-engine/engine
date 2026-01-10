/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/resource_provider.hxx>
#include <ice/mem_allocator_stack.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/heap_string.hxx>
#include <ice/native_file.hxx>
#include <ice/uri.hxx>

#include "resource_dynlib.hxx"

namespace ice
{

    class ResourceProvider_DynLibs final : public ice::ResourceProvider
    {
    public:
        ResourceProvider_DynLibs(
            ice::Allocator& alloc,
            ice::String path
        ) noexcept
            : _allocator{ alloc }
            , _base_path{ _allocator }
            , _resources{ _allocator }
        {
            ice::native_file::path_from_string(_base_path, path);
            ice::path::normalize(_base_path);
        }

        ~ResourceProvider_DynLibs() noexcept override
        {
            for (Resource* res_entry : _resources)
            {
                ice::destroy_resource_object(_allocator, res_entry);
            }
        }

        auto schemeid() const noexcept -> ice::StringID override
        {
            return ice::Scheme_Dynlib;
        }

        auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource* override
        {
            ice::u64 const resource_hash = ice::hash(uri.path());
            return ice::hashmap::get(_resources, resource_hash, nullptr);
        }

        void on_library_file(
            ice::native_file::FilePath file_path
        ) noexcept
        {
            ice::Resource* const resource = create_dynlib_resource_from_path(
                _allocator,
                *this,
                file_path
            );

            ice::u64 const resource_hash = ice::hash(resource->uri().path());
            if (ice::hashmap::has(_resources, resource_hash))
            {
                ICE_LOG(
                    LogSeverity::Warning, LogTag::Core,
                    "Skipping duplicate dyn-lib resource: '{}'",
                    resource->origin()
                );

                ice::destroy_resource_object(_allocator, resource);
            }
            else
            {
                ice::hashmap::set(_resources, resource_hash, resource);
            }
        }

        static auto traverse_callback(
            ice::native_file::FilePath base_path,
            ice::native_file::FilePath file_path,
            ice::native_file::EntityType type,
            void* userdata
        ) noexcept -> ice::native_file::TraverseAction
        {
            // We only want to list dll's in the apps main directory.
            //if (type == ice::native_file::EntityType::Directory)
            //{
            //    return ice::native_file::TraverseAction::SkipSubDir;
            //}

            ResourceProvider_DynLibs* const provider = reinterpret_cast<ResourceProvider_DynLibs*>(userdata);
            if constexpr (ice::build::is_windows)
            {
                if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".dll"))
                {
                    provider->on_library_file(file_path);
                }
            }
            if constexpr (ice::build::is_unix)
            {
                if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".so"))
                {
                    provider->on_library_file(file_path);
                }
            }
            return ice::native_file::TraverseAction::Continue;
        }

        // GitHub Issue: #108
        void initial_traverse() noexcept
        {
            ice::native_file::traverse_directories(_base_path, traverse_callback, this);
        }

        auto refresh(
            ice::Array<ice::Resource*>& out_changes
        ) noexcept -> ice::ResourceProviderResult override
        {
            if (ice::hashmap::empty(_resources))
            {
                initial_traverse();

                for (auto* resource : _resources)
                {
                    out_changes.push_back(resource);
                }
            }
            return ResourceProviderResult::Success;
        }

        void unload_resource(
            ice::Resource const* resource
        ) noexcept override { }

        auto load_resource(
            ice::Resource const* resource,
            ice::String fragment
        ) noexcept -> ice::TaskExpected<ice::Data> override
        {
            co_return ice::Data{ };
        }

    private:
        ice::Allocator& _allocator;
        ice::native_file::HeapFilePath _base_path;

        ice::HashMap<ice::Resource*> _resources;
    };

    auto create_resource_provider_dlls(
        ice::Allocator& alloc,
        ice::String path
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>
    {
        return ice::make_unique<ice::ResourceProvider_DynLibs>(alloc, alloc, path);
    }

} // namespace ice
