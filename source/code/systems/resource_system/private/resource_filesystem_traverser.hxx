/// Copyright 2025 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_types.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/native_file.hxx>
#include <ice/native_aio.hxx>
#include <ice/expected.hxx>
#include <ice/task.hxx>

#include "resource_filesystem.hxx"

namespace ice
{

    struct FileSystemTraverseRequest;

    struct FileSystemTraverserCallbacks
    {
        virtual auto allocator() noexcept -> ice::Allocator& = 0;

        virtual auto create_baked_resource(
            ice::native_file::FilePath filepath
        ) noexcept -> ice::FileSystemResource* = 0;

        virtual auto create_loose_resource(
            ice::native_file::FilePath base_path,
            ice::native_file::FilePath uri_base_path,
            ice::native_file::FilePath meta_filepath,
            ice::native_file::FilePath data_filepath
        ) noexcept -> ice::FileSystemResource* = 0;

        virtual auto register_resource(
            ice::FileSystemResource* resource
        ) noexcept -> ice::Result = 0;

        virtual void destroy_resource(
            ice::FileSystemResource* resource
        ) noexcept = 0;
    };

    class FileSystemTraverser
    {
    public:
        FileSystemTraverser(ice::FileSystemTraverserCallbacks& callbacks) noexcept;

        void create_resource_from_file(
            ice::native_file::FilePath base_path,
            ice::native_file::FilePath file_path
        ) noexcept;

        auto create_resource_from_file_async(
            ice::native_file::FilePath base_path,
            ice::native_file::HeapFilePath file_path,
            ice::FileSystemTraverseRequest& request
        ) noexcept -> ice::Task<>;

        auto traverse_async(
            ice::native_file::HeapFilePath dir_path,
            ice::FileSystemTraverseRequest& request
        ) noexcept -> ice::Task<>;

        static auto traverse_callback(
            ice::native_file::FilePath,
            ice::native_file::FilePath path,
            ice::native_file::EntityType type,
            void* userdata
        ) noexcept -> ice::native_file::TraverseAction;

        void initial_traverse(
            ice::Span<ice::native_file::HeapFilePath> base_paths
        ) noexcept;

        void initial_traverse_mt(
            ice::Span<ice::native_file::HeapFilePath> base_paths,
            ice::TaskScheduler& scheduler
        ) noexcept;

    private:
        ice::FileSystemTraverserCallbacks& _callbacks;
    };

} // namespace ice
