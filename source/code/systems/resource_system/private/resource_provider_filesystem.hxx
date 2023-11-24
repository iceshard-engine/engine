/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/uri.hxx>
#include <ice/task.hxx>
#include <ice/resource.hxx>
#include <ice/resource_provider.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/string_utils.hxx>
#include <ice/path_utils.hxx>
#include <ice/mem_allocator_stack.hxx>

#include "native/native_aio_tasks.hxx"
#include "resource_filesystem_loose.hxx"

namespace ice
{

    class FileSystemResourceProvider final : public ice::ResourceProvider
    {
    public:
        FileSystemResourceProvider(
            ice::Allocator& alloc,
            ice::Span<ice::String const> const& paths
        ) noexcept
            : _allocator{ alloc }
            , _base_paths{ _allocator }
            , _resources{ _allocator }
        {
            ice::native_fileio::HeapFilePath base_path{ _allocator };
            for (ice::String path : paths)
            {
                ice::native_fileio::path_from_string(path, base_path);
                ice::path::normalize(base_path);
                ice::array::push_back(_base_paths, base_path);
            }
        }

        ~FileSystemResourceProvider() noexcept override
        {
            for (FileSystemResource* res_entry : _resources)
            {
                _allocator.destroy(res_entry);
            }
        }

        auto schemeid() const noexcept -> ice::StringID override
        {
            return ice::Scheme_File;
        }

        void create_resource_from_file(
            ice::native_fileio::FilePath base_path,
            ice::native_fileio::FilePath file_path
        ) noexcept
        {
            // Early out for metadata files.
            if (ice::path::extension(file_path) == ISP_PATH_LITERAL(".isrm"))
            {
                return;
            }

            ice::StackAllocator_1024 temp_alloc;
            ice::native_fileio::FilePath const uribase = ice::path::directory(base_path);
            ice::native_fileio::FilePath const datafile = file_path;
            ice::native_fileio::HeapFilePath metafile{ temp_alloc };
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

        static auto traverse_callback(
            ice::native_fileio::FilePath base_path,
            ice::native_fileio::FilePath path,
            ice::native_fileio::EntityType type,
            void* userdata
        ) noexcept -> ice::native_fileio::TraverseAction
        {
            if (type == ice::native_fileio::EntityType::File)
            {
                FileSystemResourceProvider* self = reinterpret_cast<FileSystemResourceProvider*>(userdata);
                self->create_resource_from_file(base_path, path);
            }
            return ice::native_fileio::TraverseAction::Continue;
        }

        void initial_traverse() noexcept
        {
            for (ice::native_fileio::FilePath base_path : _base_paths)
            {
                ice::native_fileio::traverse_directories(base_path, traverse_callback, this);
            }
        }

        auto collect(
            ice::Array<ice::Resource const*>& out_changes
        ) noexcept -> ice::ucount
        {
            for (auto* resource : _resources)
            {
                ice::array::push_back(out_changes, resource);
            }
            return ice::hashmap::count(_resources);
        }

        auto refresh(
            ice::Array<ice::Resource const*>& out_changes
        ) noexcept -> ice::ResourceProviderResult override
        {
            if (ice::hashmap::empty(_resources))
            {
                initial_traverse();

                for (auto* resource : _resources)
                {
                    ice::array::push_back(out_changes, resource);
                }
            }
            return ResourceProviderResult::Success;
        }

        auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource const* override
        {
            ICE_ASSERT(
                uri.scheme == ice::stringid_hash(schemeid()),
                "Trying to find resource for URI that is not handled by this provider."
            );

            ice::FileSystemResource const* found_resource = nullptr;
            ice::u32 const origin_size = ice::string::size(uri.path);

            ice::HeapString<> predicted_path{ _allocator };
            for (ice::native_fileio::FilePath base_path : _base_paths)
            {
                ice::string::resize(predicted_path, 0);
                ice::string::reserve(predicted_path, origin_size + ice::string::size(base_path));
                ice::native_fileio::path_to_string(base_path, predicted_path);

                // Remove one directory if neccessary, because it's may be the common value of the base path and the uri path.
                // Note: This is because if a base path like 'dir/subdir' is provided the uri is created against 'dir/'
                //  While a base path like 'dir/subdir/' will create uris against 'dir/subdir/'
                if (ice::string::back(base_path) != '/')
                {
                    ice::path::join(predicted_path, "..");
                }
                ice::path::join(predicted_path, uri.path);
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

        auto access_loose_resource(
            ice::Resource const* resource
        ) const noexcept -> ice::LooseResource const* override
        {
            return static_cast<ice::FileSystemResource const*>(resource);
        }

        void unload_resource(
            ice::Allocator& alloc,
            ice::Resource const* /*resource*/,
            ice::Memory memory
        ) noexcept override
        {
            alloc.deallocate(memory);
        }

        auto load_resource(
            ice::Allocator& alloc,
            ice::Resource const* resource,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> override
        {
            ice::FileSystemResource const* const filesys_res = static_cast<ice::FileSystemResource const*>(resource);
            co_return co_await filesys_res->load_data(alloc, scheduler, nativeio);
        }

        auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource const* root_resource
        ) const noexcept -> ice::Resource const* override
        {
            ice::u32 const origin_size = ice::string::size(root_resource->origin());

            ice::HeapString<> predicted_path{ _allocator, };
            ice::string::reserve(predicted_path, origin_size + ice::string::size(relative_uri.path));

            predicted_path = ice::string::substr(
                root_resource->origin(),
                0,
                origin_size - ice::string::size(
                    ice::path::filename(root_resource->name())
                )
            );

            ice::path::join(predicted_path, relative_uri.path);
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

    protected:
        ice::Allocator& _allocator;
        ice::Array<ice::native_fileio::HeapFilePath, ice::ContainerLogic::Complex> _base_paths;

        ice::HashMap<ice::FileSystemResource*> _resources;
    };

    auto create_resource_provider(
        ice::Allocator& alloc,
        ice::Span<ice::String const> paths
    ) noexcept -> ice::UniquePtr<ice::ResourceProvider>
    {
        return ice::make_unique<ice::FileSystemResourceProvider>(alloc, alloc, paths);
    }

} // namespace ice
