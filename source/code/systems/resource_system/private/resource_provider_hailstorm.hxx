/// Copyright 2023 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/container/linked_queue.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/array.hxx>
#include <ice/heap_string.hxx>
#include <ice/resource_provider.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/task_queue.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/mem_data.hxx>
#include <ice/native_file.hxx>
#include <ice/path_utils.hxx>
#include <ice/sort.hxx>
#include <ice/span.hxx>
#include <ice/uri.hxx>
#include <ice/devui_widget.hxx>

#include "resource_hailstorm_entry.hxx"

#include <hailstorm/hailstorm_operations.hxx>
#include <atomic>

namespace ice
{

    class HailstormChunkLoader
    {
    public:
        HailstormChunkLoader(
            ice::Allocator& alloc,
            hailstorm::v1::HailstormChunk const& chunk
        ) noexcept
            : _allocator{ alloc }
            , _chunk{ chunk }
        { }

        virtual ~HailstormChunkLoader() noexcept = default;

        virtual auto free_space() noexcept -> ice::usize { return 0_B; }

        virtual void free_slice(
            ice::u32 offset,
            ice::u32 size
        ) noexcept = 0;

        virtual auto request_slice(
            ice::u32 offset,
            ice::u32 size,
            ice::native_aio::AIOPort aioport
        ) noexcept -> ice::Task<ice::Data> = 0;

        virtual void release_slice_refcount() noexcept { }

    protected:
        ice::Allocator& _allocator;
        hailstorm::v1::HailstormChunk const& _chunk;
    };

    // class HailstormChunkLoader_AlwaysLoaded final : public HailstormChunkLoader
    // {
    // public:
    //     HailstormChunkLoader_AlwaysLoaded(
    //         ice::Allocator& alloc,
    //         hailstorm::v1::HailstormChunk const& chunk,
    //         ice::native_file::File const& file
    //     ) noexcept
    //         : HailstormChunkLoader{ alloc, chunk }
    //         , _file{ file }
    //         , _memory{ }
    //     {
    //         // TODO: Move outside of the ctor, makes sure we can handle errors using a factory.
    //         _memory = _allocator.allocate({ _chunk.size, _chunk.align });
    //         ice::usize const bytes_read = ice::native_file::read_file(
    //             _file, _chunk.offset, _chunk.size, _memory
    //         );
    //         ICE_ASSERT_CORE(bytes_read == _chunk.size);
    //     }

    //     ~HailstormChunkLoader_AlwaysLoaded() override
    //     {
    //         _allocator.deallocate(_memory);
    //     }

    // private:
    //     ice::native_file::File const& _file;
    //     ice::Memory _memory;
    // };

    class HailstormChunkLoader_Persistent final : public HailstormChunkLoader
    {
    public:
        HailstormChunkLoader_Persistent(
            ice::Allocator& alloc,
            hailstorm::v1::HailstormChunk const& chunk,
            ice::native_file::File const& file
        ) noexcept;

        ~HailstormChunkLoader_Persistent() noexcept override;

        void free_slice(
            ice::u32 offset,
            ice::u32 size
        ) noexcept override;

        auto request_slice(
            ice::u32 offset,
            ice::u32 size,
            ice::native_aio::AIOPort aioport
        ) noexcept -> ice::Task<ice::Data> override;

        void release_slice_refcount() noexcept override;

    private:
        ice::native_file::File const& _file;
        ice::TaskQueue _awaiting_tasks;
        std::atomic_int32_t _awaitcount;
        std::atomic_int32_t _refcount;
        ice::Memory _memory;
    };

    class HailstormChunkLoader_Regular : public HailstormChunkLoader
    {
    public:
        HailstormChunkLoader_Regular(
            ice::Allocator& alloc,
            hailstorm::v1::HailstormChunk const& chunk,
            ice::native_file::File const& file
        ) noexcept;

        ~HailstormChunkLoader_Regular() noexcept override;

        auto free_space() noexcept -> ice::usize override;

        void free_slice(
            ice::u32 offset,
            ice::u32 size
        ) noexcept override;

        auto request_slice(
            ice::u32 offset,
            ice::u32 size,
            ice::native_aio::AIOPort aioport
        ) noexcept -> ice::Task<ice::Data> override;

    private:
        ice::native_file::File const& _file;
        ice::HashMap<ice::u32> _offset_map;
        ice::Array<void*> _pointers;
    };

    class HailStormResourceProvider final : public ice::ResourceProvider
    {
    public:
        HailStormResourceProvider(
            ice::Allocator& alloc,
            ice::String path,
            ice::native_aio::AIOPort aioport
        ) noexcept;
        ~HailStormResourceProvider() noexcept override;

        auto schemeid() const noexcept -> ice::StringID override;

        auto refresh(
            ice::Array<ice::Resource*>& out_changes
        ) noexcept -> ice::ResourceProviderResult override;

        auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource* override;

        auto access_loose_resource(
            ice::Resource const* resource
        ) const noexcept -> ice::LooseResource const* override;

        void unload_resource(
            ice::Resource const* resource
        ) noexcept override;

        auto load_resource(
            ice::Resource const* resource,
            ice::String fragment
        ) noexcept -> ice::TaskExpected<ice::Data> override;

        auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource const* root_resource
        ) const noexcept -> ice::Resource const* override;

        class DevUI;

    private:
        ice::ProxyAllocator _allocator;
        ice::ProxyAllocator _data_allocator;
        ice::native_aio::AIOPort _aioport;
        ice::native_file::HeapFilePath _hspack_path;
        ice::native_file::File _hspack_file;
        ice::HeapString<> _packname;

        ice::Memory _header_memory;
        ice::Memory _paths_memory;

        hailstorm::v1::HailstormData _pack;
        ice::Array<ice::HailstormChunkLoader*> _loaders;
        ice::Array<ice::HailstormResource*> _entries;
        ice::HashMap<ice::u32> _entrymap;

        ice::UniquePtr<DevUIWidget> _devui_widget;
    };

    auto create_hailstorm_provider_devui(
        ice::Allocator& alloc,
        ice::String name,
        ice::HailStormResourceProvider const& provider
    ) noexcept -> ice::UniquePtr<ice::DevUIWidget>;

} // namespace ice
