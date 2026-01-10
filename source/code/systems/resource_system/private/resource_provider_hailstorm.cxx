/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include "resource_aio_request.hxx"
#include "resource_provider_hailstorm.hxx"

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
                queue.push_back(this);
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

    HailstormChunkLoader_Persistent::HailstormChunkLoader_Persistent(
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

    HailstormChunkLoader_Persistent::~HailstormChunkLoader_Persistent() noexcept
    {
        ICE_ASSERT_CORE(_awaitcount == 0);
        // If it's a metadata chunk we can't fully track all references unfortunately.
        ICE_ASSERT_CORE(_refcount == 0 || _chunk.type == 1);
        _allocator.deallocate(_memory);
    }

    void HailstormChunkLoader_Persistent::free_slice(
        ice::u32 offset,
        ice::u32 size
    ) noexcept
    {
        _refcount.fetch_sub(1, std::memory_order_relaxed);
    }

    auto HailstormChunkLoader_Persistent::request_slice(
        ice::u32 offset,
        ice::u32 size,
        ice::native_aio::AIOPort aioport
    ) noexcept -> ice::Task<ice::Data>
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
            ice::Memory const res_memory = _allocator.allocate({ { size_t(_chunk.size) }, (ice::ualign) _chunk.align });
            ice::detail::AsyncReadRequest request{ aioport, _file, ice::usize{static_cast<ice::usize::base_type>(_chunk.size)}, ice::usize{static_cast<ice::usize::base_type>(_chunk.offset)}, res_memory };
            ice::usize const bytes_read = co_await request;

            // Clear memory if failed to load the file chunk
            if (bytes_read == 0_B)
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
                for (auto const* awaiting : _awaiting_tasks.consume())
                {
                    _awaitcount.fetch_sub(1, std::memory_order_relaxed);
                    awaiting->_coro.resume();
                }
            }

            ICE_ASSERT_CORE(_awaitcount.load(std::memory_order_relaxed) == 0);
        }

        co_return { ice::ptr_add(_memory.location, ice::usize{ offset }), { size }, _memory.alignment };
    }

    void HailstormChunkLoader_Persistent::release_slice_refcount() noexcept
    {
        _refcount.fetch_sub(1, std::memory_order_relaxed);
    }

    HailstormChunkLoader_Regular::HailstormChunkLoader_Regular(
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
        _pointers.resize(estimated_pointer_count);
    }

    HailstormChunkLoader_Regular::~HailstormChunkLoader_Regular() noexcept
    {
        for (void* pointer : _pointers)
        {
            _allocator.deallocate(pointer);
        }
    }

    auto HailstormChunkLoader_Regular::free_space() noexcept -> ice::usize
    {
        return 0_B;
    }

    void HailstormChunkLoader_Regular::free_slice(
        ice::u32 offset,
        ice::u32 size
    ) noexcept
    {
        ice::u32 const ptr_idx = ice::hashmap::get_or_set(
            _offset_map, offset, ice::hashmap::count(_offset_map)
        );
        // Why would we free something that was never allocated?
        ICE_ASSERT_CORE(_pointers[ptr_idx] != nullptr);
        _allocator.deallocate(ice::exchange(_pointers[ptr_idx], nullptr));
    }

    auto HailstormChunkLoader_Regular::request_slice(
        ice::u32 offset,
        ice::u32 size,
        ice::native_aio::AIOPort aioport
    ) noexcept -> ice::Task<ice::Data>
    {
        ice::u32 const ptr_idx = ice::hashmap::get_or_set(
            _offset_map, offset, ice::hashmap::count(_offset_map)
        );
        if (_pointers[ptr_idx] == nullptr)
        {
            // TODO: See if we could use large page allocations if files size < 1kib
            ice::Memory const memory = _allocator.allocate({ { size }, (ice::ualign)_chunk.align });
            ice::detail::AsyncReadRequest request{ aioport, _file, ice::usize{static_cast<ice::usize::base_type>(_chunk.size)}, ice::usize{static_cast<ice::usize::base_type>(_chunk.offset)}, memory };
            ice::usize const bytes_read = co_await request;

            ICE_ASSERT_CORE(bytes_read == ice::usize{ size });
            _pointers[ptr_idx] = memory.location;
        }
        co_return { _pointers[ptr_idx], { size }, (ice::ualign)_chunk.align };
    }

    HailStormResourceProvider::HailStormResourceProvider(
        ice::Allocator& alloc,
        ice::String path,
        ice::native_aio::AIOPort aioport
    ) noexcept
        : _allocator{ alloc, "Hailstorm" }
        , _data_allocator{ alloc, "Data" }
        , _aioport{ aioport }
        , _hspack_path{ _allocator }
        , _packname{ _allocator, ice::path::filename(path) }
        , _header_memory{ }
        , _paths_memory{ }
        , _loaders{ _allocator }
        , _entries{ _allocator }
        , _entrymap{ _allocator }
        , _devui_widget{ create_hailstorm_provider_devui(_allocator, _packname, *this) }
    {
        ice::native_file::path_from_string(_hspack_path, path);
    }

    HailStormResourceProvider::~HailStormResourceProvider() noexcept
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

    auto HailStormResourceProvider::schemeid() const noexcept -> ice::StringID
    {
        return ice::Scheme_HailStorm;
    }

    auto HailStormResourceProvider::refresh(
        ice::Array<ice::Resource*>& out_changes
    ) noexcept -> ice::ResourceProviderResult
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
            prefix.push_back("/");

            ice::usize::base_type const size_extended_paths = hailstorm::v1::prefixed_resource_paths_size(
                _pack.paths, (ice::u32)_pack.resources.size(), ice::String{ prefix }
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
                { resptr, (ice::u32)_pack.resources.size() },
                { _paths_memory.location, _paths_memory.size.value, (size_t)_paths_memory.alignment },
                ice::String{ prefix }
            );
            ICE_ASSERT_CORE(prefixing_success);

            _pack.paths_data = { _paths_memory.location, _paths_memory.size.value, (size_t)_paths_memory.alignment };

            // Reopen as async
            _hspack_file.close();
            _hspack_file = ice::native_file::open_file(
                _aioport,
                _hspack_path,
                FileOpenFlags::Exclusive
            ).value();

            _loaders.resize(_pack.header.count_chunks);
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

            _entries.resize(_pack.header.count_resources);
            for (ice::u32 idx = 0; idx < _pack.header.count_resources; ++idx)
            {
                v1::HailstormResource const& res = _pack.resources[idx];
                ice::String const respath = ice::String{ (char const*)ice::ptr_add(_pack.paths_data.location, { res.path_offset }), res.path_size };

                ice::URI const res_uri{ Scheme_HailStorm, respath };

                if (res.chunk == res.meta_chunk)
                {
                    v1::HailstormChunk const& chunk = _pack.chunks[res.chunk];
                    ICE_ASSERT_CORE(chunk.type == 3);

                    _entries[idx] = ice::create_resource_object<ice::HailstormResourceMixed>(
                        _allocator,
                        *this,
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

                    _entries[idx] = ice::create_resource_object<ice::HailstormResourceSplit>(
                        _allocator,
                        *this,
                        res_uri,
                        res, *_loaders[res.meta_chunk], *_loaders[res.chunk]
                    );
                }

                ice::multi_hashmap::insert(_entrymap, ice::hash(res_uri.path()), idx);
                out_changes.push_back(_entries[idx]);
            }

            return ResourceProviderResult::Success;
        }
        return ResourceProviderResult::Skipped;
    }

    auto HailStormResourceProvider::find_resource(
        ice::URI const& uri
    ) const noexcept -> ice::Resource*
    {
        u32 idx = ice::u32_max;
        auto it = ice::multi_hashmap::find_first(_entrymap, ice::hash(uri.path()));
        while (it != nullptr && idx == ice::u32_max)
        {
            if (_entries[it.value()]->name() == uri.path())
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

    auto HailStormResourceProvider::access_loose_resource(
        ice::Resource const* resource
    ) const noexcept -> ice::LooseResource const*
    {
        ICE_ASSERT_CORE(false);
        return nullptr;
    }

    void HailStormResourceProvider::unload_resource(
        ice::Resource const* resource
    ) noexcept
    {
        hailstorm::HailstormResource const& hsres = static_cast<ice::HailstormResource const*>(resource)->_handle;
        _loaders[hsres.chunk]->free_slice(hsres.offset, hsres.size);
        _loaders[hsres.meta_chunk]->free_slice(hsres.meta_offset, hsres.meta_size);
    }

    auto HailStormResourceProvider::load_resource(
        ice::Resource const* resource,
        ice::String fragment
    ) noexcept -> ice::TaskExpected<ice::Data>
    {
        hailstorm::HailstormResource const& hsres = static_cast<ice::HailstormResource const*>(resource)->_handle;
        if (fragment.not_empty() && fragment == "meta")
        {
            co_return co_await _loaders[hsres.meta_chunk]->request_slice(hsres.meta_offset, hsres.meta_size, _aioport);
        }
        else
        {
            co_return co_await _loaders[hsres.chunk]->request_slice(hsres.offset, hsres.size, _aioport);
        }
    }

    auto HailStormResourceProvider::resolve_relative_resource(
        ice::URI const& relative_uri,
        ice::Resource const* root_resource
    ) const noexcept -> ice::Resource const*
    {
        return nullptr;
    }

} // namespace ice
