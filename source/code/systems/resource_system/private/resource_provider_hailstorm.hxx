/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/concept/strong_type_value.hxx>
#include <ice/container/linked_queue.hxx>
#include <ice/container/hashmap.hxx>
#include <ice/container/array.hxx>
#include <ice/string/heap_string.hxx>
#include <ice/resource_provider.hxx>
#include <ice/resource_meta.hxx>
#include <ice/task_awaitable.hxx>
#include <ice/task_queue.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/mem_data.hxx>
#include <ice/native_file.hxx>
#include <ice/path_utils.hxx>
#include <ice/sort.hxx>
#include <ice/span.hxx>
#include <ice/uri.hxx>

#include "resource_hailstorm_entry.hxx"
#include "native/native_aio_tasks.hxx"

#include <hailstorm/hailstorm_operations.hxx>
#include <atomic>

namespace ice
{

    namespace detail
    {

        struct LoadAwaitable : ice::TaskAwaitableBase
        {
            ice::TaskQueue& queue;

            LoadAwaitable(ice::TaskQueue& queue) noexcept
                : TaskAwaitableBase{ ._params{.task_flags = TaskFlags{}}, .next = nullptr, .result = {} }
                , queue{ queue }
            {
            }

            constexpr auto await_ready() const noexcept
            {
                return false;
            }

            inline auto await_suspend(ice::coroutine_handle<> coro) noexcept
            {
                _coro = coro;
                ice::linked_queue::push(queue._awaitables, this);
            }

            constexpr void await_resume() const noexcept
            {
                //// Aquire changes done to the memory block we are waiting on.
                //std::atomic_thread_fence(std::memory_order_acquire);
                //ICE_ASSERT_CORE(result.ptr != nullptr);
                //return *reinterpret_cast<ice::Memory const*>(result.ptr);
            }
        };

    } // namespace detail

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
            ice::NativeAIO* nativeio
        ) noexcept -> ice::Task<ice::Memory> = 0;

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
        ) noexcept
            : HailstormChunkLoader{ alloc, chunk }
            , _file{ file }
            , _awaiting_tasks{ }
            , _awaitcount{ 0 }
            , _refcount{ 0 }
            , _memory{ }
        { }

        ~HailstormChunkLoader_Persistent() noexcept
        {
            ICE_ASSERT_CORE(_awaitcount == 0);
            // If it's a metadata chunk we can't fully track all references unfortunately.
            ICE_ASSERT_CORE(_refcount == 0 || _chunk.type == 1);
            _allocator.deallocate(_memory);
        }

        void free_slice(
            ice::u32 offset,
            ice::u32 size
        ) noexcept override
        {
            _refcount.fetch_sub(1, std::memory_order_relaxed);
        }

        auto request_slice(
            ice::u32 offset,
            ice::u32 size,
            ice::NativeAIO* nativeio
        ) noexcept -> ice::Task<ice::Memory> override
        {
            if (_refcount.fetch_add(1u, std::memory_order_relaxed) > 0)
            {
                // If the pointer is not set, wait for it to be loaded.
                if (_memory.location == nullptr)
                {
                    // Atomically keep number of awaiting resources.
                    _awaitcount.fetch_add(1, std::memory_order_relaxed);
                    co_await detail::LoadAwaitable{ _awaiting_tasks };
                }
            }

            // We might actually have the block still allocated even though all references where removed at some point.
            if (_memory.location == nullptr)
            {
                ice::Memory const res_memory = _allocator.allocate({ { size_t(_chunk.size_origin) }, (ice::ualign) _chunk.align });
                ice::AsyncReadFileRef request{ nativeio, _file, res_memory, { size_t(_chunk.size) }, { size_t(_chunk.offset) } };
                ice::AsyncReadFileRef::Result result = co_await request;

                // Clear memory if failed to load the file chunk
                if (result.bytes_read == 0_B)
                {
                    _allocator.deallocate(res_memory);
                }
                else
                {
                    // Store the memory pointer, it's already ready to be consumed.
                    _memory = res_memory;
                }

                // Resume all awaiting coroutines even if we failed
                while (_awaitcount.load(std::memory_order_relaxed) > 0)
                {
                    for (auto const* awaiting : ice::linked_queue::consume(_awaiting_tasks._awaitables))
                    {
                        _awaitcount.fetch_sub(1, std::memory_order_relaxed);
                        awaiting->_coro.resume();
                    }
                }

                ICE_ASSERT_CORE(_awaitcount.load(std::memory_order_relaxed) == 0);
            }

            co_return { ice::ptr_add(_memory.location, ice::usize{ offset }), { size }, _memory.alignment };
        }

        void release_slice_refcount() noexcept override
        {
            _refcount.fetch_sub(1, std::memory_order_relaxed);
        }

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
        ) noexcept
            : HailstormChunkLoader{ alloc, chunk }
            , _file{ file }
            , _offset_map{ _allocator }
            , _pointers{ _allocator }
        {
            // For mixed-regular chunks we need double the number of entries because
            //   both data and metadata pointers are allocated separately, which doubles the required hashmap size.
            ice::u32 const estimated_pointer_count = _chunk.count_entries * (_chunk.type == 3 ? 2 : 1);
            ice::hashmap::reserve(_offset_map, estimated_pointer_count);
            ice::array::resize(_pointers, estimated_pointer_count);
        }

        ~HailstormChunkLoader_Regular() noexcept
        {
            for (void* pointer : _pointers)
            {
                _allocator.deallocate(pointer);
            }
        }

        auto free_space() noexcept -> ice::usize override { return 0_B; }

        void free_slice(
            ice::u32 offset,
            ice::u32 size
        ) noexcept override
        {
            ice::u32 const ptr_idx = ice::hashmap::get_or_set(
                _offset_map, offset, ice::hashmap::count(_offset_map)
            );
            // Why would we free something that was never allocated?
            ICE_ASSERT_CORE(_pointers[ptr_idx] != nullptr);
            _allocator.deallocate(ice::exchange(_pointers[ptr_idx], nullptr));
        }

        auto request_slice(
            ice::u32 offset,
            ice::u32 size,
            ice::NativeAIO* /*nativeio*/
        ) noexcept -> ice::Task<ice::Memory> override
        {
            ice::u32 const ptr_idx = ice::hashmap::get_or_set(
                _offset_map, offset, ice::hashmap::count(_offset_map)
            );
            if (_pointers[ptr_idx] == nullptr)
            {
                // TODO: See if we could use large page allocations if files size < 1kib
                ice::Memory const memory = _allocator.allocate({ { size }, (ice::ualign)_chunk.align });
                ice::AsyncReadFileRef request{ nullptr, _file, memory, { size }, { offset } };
                ice::AsyncReadFileRef::Result result = co_await request;

                ICE_ASSERT_CORE(result.bytes_read == ice::usize{ size });
                _pointers[ptr_idx] = memory.location;
            }
            co_return{ _pointers[ptr_idx], { size }, (ice::ualign)_chunk.align };
        }

    private:
        ice::native_file::File const& _file;
        ice::HashMap<ice::u32> _offset_map;
        ice::Array<void*> _pointers;
    };

    class HailStormResourceProvider final : public ice::ResourceProvider
    {
    public:
        HailStormResourceProvider(ice::Allocator& alloc, ice::String path) noexcept
            : _allocator{ alloc }
            , _hspack_path{ _allocator }
            , _packname{ _allocator, ice::path::filename(path) }
            , _header_memory{ }
            , _paths_memory{ }
            , _loaders{ _allocator }
            , _entries{ _allocator }
            , _entrymap{ _allocator }
        {
            ice::native_file::path_from_string(path, _hspack_path);
        }

        ~HailStormResourceProvider() noexcept
        {
            for (ice::HailstormResource* resource : _entries)
            {
                _allocator.destroy(resource);
            }
            for (ice::HailstormChunkLoader* loader : _loaders)
            {
                _allocator.destroy(loader);
            }

            _allocator.deallocate(_paths_memory);
            _allocator.deallocate(_header_memory);
            _hspack_file.close();
        }

        auto schemeid() const noexcept -> ice::StringID override { return ice::Scheme_HailStorm; }

        auto refresh(
            ice::Array<ice::Resource const*>& out_changes
        ) noexcept -> ice::ResourceProviderResult override
        {
            if (_hspack_file == false)
            {
                using native_file::FileOpenFlags;
                _hspack_file = ice::native_file::open_file(
                    _hspack_path,
                    FileOpenFlags::Exclusive
                );

                using namespace hailstorm;
                HailstormHeaderBase base_header;
                ice::native_file::read_file(
                    _hspack_file,
                    0_B,
                    ice::size_of<HailstormHeaderBase>,
                    { &base_header, ice::size_of<HailstormHeaderBase>, ice::align_of<HailstormHeaderBase> }
                );

                _header_memory = _allocator.allocate({ { size_t(base_header.header_size) }, ice::align_of<v1::HailstormHeader> });
                ice::native_file::read_file(
                    _hspack_file,
                    0_B,
                    _header_memory.size,
                    _header_memory
                );

                if (v1::read_header({ _header_memory.location, _header_memory.size.value, (size_t) _header_memory.alignment }, _pack) != hailstorm::Result::Success)
                {
                    return ResourceProviderResult::Failure;
                }

                ice::HeapString<> prefix{ _allocator, _packname };
                ice::string::push_back(prefix, "/");

                ice::usize::base_type const size_extended_paths = hailstorm::v1::prefixed_resource_paths_size(
                    _pack.paths, (ice::ucount)_pack.resources.size(), ice::String{ prefix }
                );

                // We allocate enough memory to keep all original paths prefixed with the resource file name and a slash.
                _paths_memory = _allocator.allocate(ice::usize{ size_extended_paths });
                ice::native_file::read_file(
                    _hspack_file,
                    { size_t(_pack.header.header_size) }, // Here paths data start
                    { _pack.paths.size },
                    _paths_memory
                );

                // Need to update the resources path_offset values
                v1::HailstormResource* const resptr = reinterpret_cast<v1::HailstormResource*>(
                    ice::ptr_add(
                        _header_memory.location,
                        ice::ptr_distance(_header_memory.location, _pack.resources.data())
                    )
                );

                bool const prefixing_success = v1::prefix_resource_paths(
                    _pack.paths,
                    { resptr, (ice::ucount)_pack.resources.size() },
                    { _paths_memory.location, _paths_memory.size.value, (size_t)_paths_memory.alignment },
                    ice::String{ prefix }
                );
                ICE_ASSERT_CORE(prefixing_success);

                _pack.paths_data = { _paths_memory.location, _paths_memory.size.value, (size_t)_paths_memory.alignment };

                // Reopen as async
                _hspack_file.close();
                _hspack_file = ice::native_file::open_file(
                    _hspack_path,
                    FileOpenFlags::Exclusive | FileOpenFlags::Asynchronous
                );

                ice::array::resize(_loaders, _pack.header.count_chunks);
                for (ice::u32 idx = 0; idx < _pack.header.count_chunks; ++idx)
                {
                    if (_pack.chunks[idx].persistance >= 2)
                    {
                        _loaders[idx] = _allocator.create<ice::HailstormChunkLoader_Persistent>(
                            _allocator, _pack.chunks[idx], _hspack_file
                        );
                    }
                    else
                    {
                        _loaders[idx] = _allocator.create<ice::HailstormChunkLoader_Regular>(
                            _allocator, _pack.chunks[idx], _hspack_file
                        );
                    }
                }

                ice::array::resize(_entries, _pack.header.count_resources);
                for (ice::u32 idx = 0; idx < _pack.header.count_resources; ++idx)
                {
                    v1::HailstormResource const& res = _pack.resources[idx];
                    ice::String const respath = ice::String{ (char const*)ice::ptr_add(_pack.paths_data.location, { res.path_offset }), res.path_size };

                    ice::URI const res_uri{ Scheme_HailStorm, respath };

                    if (res.chunk == res.meta_chunk)
                    {
                        v1::HailstormChunk const& chunk = _pack.chunks[res.chunk];
                        ICE_ASSERT_CORE(chunk.type == 3);

                        _entries[idx] = _allocator.create<ice::HailstormResourceMixed>(
                            res_uri,
                            res, *_loaders[res.chunk]
                        );
                    }
                    else
                    {
                        v1::HailstormChunk const& data_chunk = _pack.chunks[res.chunk];
                        v1::HailstormChunk const& meta_chunk = _pack.chunks[res.meta_chunk];
                        ICE_ASSERT_CORE(data_chunk.type == 2);
                        ICE_ASSERT_CORE(meta_chunk.type == 1);

                        _entries[idx] = _allocator.create<ice::HailstormResourceSplit>(
                            res_uri,
                            res, *_loaders[res.meta_chunk], *_loaders[res.chunk]
                        );
                    }

                    ice::multi_hashmap::insert(_entrymap, ice::hash(res_uri.path), idx);
                    ice::array::push_back(out_changes, _entries[idx]);
                }

                return ResourceProviderResult::Success;
            }
            return ResourceProviderResult::Skipped;
        }

        auto find_resource(
            ice::URI const& uri
        ) const noexcept -> ice::Resource const* override
        {
            u32 idx = ice::u32_max;
            auto it = ice::multi_hashmap::find_first(_entrymap, ice::hash(uri.path));
            while (it != nullptr && idx == ice::u32_max)
            {
                if (_entries[it.value()]->name() == uri.path)
                {
                    idx = it.value();
                }
                it = ice::multi_hashmap::find_next(_entrymap, it);
            }

            if (idx != ice::u32_max)
            {
                return _entries[idx];
            }
            return nullptr;
        }

        auto access_loose_resource(
            ice::Resource const* resource
        ) const noexcept -> ice::LooseResource const* override
        {
            ICE_ASSERT_CORE(false);
            return nullptr;
        }

        void unload_resource(
            ice::Allocator& alloc,
            ice::Resource const* resource,
            ice::Memory /*memory*/
        ) noexcept override
        {
            hailstorm::HailstormResource const& hsres = static_cast<ice::HailstormResource const*>(resource)->_handle;
            _loaders[hsres.chunk]->free_slice(hsres.offset, hsres.size);
            _loaders[hsres.meta_chunk]->free_slice(hsres.meta_offset, hsres.meta_size);
        }

        auto load_resource(
            ice::Allocator& alloc,
            ice::Resource const* resource,
            ice::TaskScheduler& scheduler,
            ice::NativeAIO* nativeio
        ) const noexcept -> ice::Task<ice::Memory> override
        {
            hailstorm::HailstormResource const& hsres = static_cast<ice::HailstormResource const*>(resource)->_handle;
            co_return co_await _loaders[hsres.chunk]->request_slice(hsres.offset, hsres.size, nativeio);
        }

        auto resolve_relative_resource(
            ice::URI const& relative_uri,
            ice::Resource const* root_resource
        ) const noexcept -> ice::Resource const* override
        {
            return nullptr;
        }

    private:
        ice::Allocator& _allocator;
        ice::native_file::HeapFilePath _hspack_path;
        ice::native_file::File _hspack_file;
        ice::HeapString<> _packname;

        ice::Memory _header_memory;
        ice::Memory _paths_memory;

        hailstorm::v1::HailstormData _pack;
        ice::Array<ice::HailstormChunkLoader*> _loaders;
        ice::Array<ice::HailstormResource*> _entries;
        ice::HashMap<ice::u32> _entrymap;
    };

} // namespace ice
