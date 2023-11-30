#include <ice/resource_hailstorm_operations.hxx>
#include <ice/container/array.hxx>
#include <ice/sync_manual_events.hxx>
#include <ice/task_utils.hxx>
#include <ice/task.hxx>

namespace ice::hailstorm::v1
{

    namespace detail
    {

        struct ErrorHandler
        {
            bool const is_valid;
            inline bool await_ready() const noexcept { return is_valid; }
            // Since we know the coroutine is not used outside we can destroy it when an error happens and
            //   it will clean up all resources on destruction.
            inline void await_suspend(ice::coroutine_handle<> coro) const noexcept { coro.destroy(); }
            inline void await_resume() const noexcept { }
        };

        struct TrackedStream final
        {
            TrackedStream(
                ice::hailstorm::v1::HailstormAsyncWriteParams const& params,
                ice::usize size
            ) noexcept
                : _params{ params }
                , _open{ params.fn_async_open(size, params.async_userdata) }
            {
            }

            auto write_header(ice::Data data, ice::usize offset) noexcept
            {
                return ErrorHandler{ _params.fn_async_write_header(data, offset, _params.async_userdata) };
            }

            auto write_resource(
                ice::hailstorm::v1::HailstormWriteData const& data, ice::u32 idx, ice::usize offset
            ) noexcept
            {
                return ErrorHandler{ _params.fn_async_write_resource(data, idx, offset, _params.async_userdata) };
            }

            auto write_metadata(
                ice::hailstorm::v1::HailstormWriteData const& data, ice::u32 idx, ice::usize offset
            ) noexcept
            {
                return ErrorHandler{ _params.fn_async_write_metadata(data, idx, offset, _params.async_userdata) };
            }

            void close() noexcept
            {
                if (_open)
                {
                    _params.fn_async_close(_params.async_userdata);
                    _open = false;
                }
            }

            ~TrackedStream() noexcept
            {
                ICE_ASSERT_CORE(_open == false);
            }

            ice::hailstorm::v1::HailstormAsyncWriteParams const& _params;
            bool _open;
        };

        struct TrackedMemory final
        {
            inline explicit TrackedMemory(ice::Allocator& alloc, ice::AllocRequest req) noexcept
                : _allocator{ alloc }
                , _memory{ (ice::Memory)_allocator.allocate(req) }
                , location{ _memory.location }
                , size{ _memory.size }
                , alignment{ _memory.alignment }
            {
            }

            ~TrackedMemory() noexcept
            {
                _allocator.deallocate(_memory);
            }

            inline explicit operator ice::Data() const noexcept
            {
                return ice::data_view(_memory);
            }

            ice::Allocator& _allocator;
            ice::Memory const _memory;

            void* const& location;
            ice::usize const& size;
            ice::ualign const& alignment;
        };

    } // namespace detail

    auto read_header(
        ice::Data data,
        ice::hailstorm::v1::HailstormData& out_hailstorm
    ) noexcept -> ice::Result
    {
        void const* start = data.location;
        ice::hailstorm::v1::HailstormHeader const* header;
        ice::hailstorm::v1::HailstormPaths const* paths;
        data = ice::data::read_raw(data, header);
        data = ice::data::read_raw(data, paths);

        if (header->magic != Constant_HailstormMagic
            || header->header_version != Constant_HailstormHeaderVersion
            || header->header_size >= 1024_MiB)
        {
            return Res::E_InvalidArgument;
        }

        out_hailstorm.header = *header;
        out_hailstorm.paths = *paths;
        data = ice::data::read_span(data, header->count_chunks, out_hailstorm.chunks);
        data = ice::data::read_span(data, header->count_resources, out_hailstorm.resources);

        if (paths->size <= data.size)
        {
            out_hailstorm.paths_data = ice::Data{
                ice::ptr_add(start, paths->offset),
                paths->size,
                ice::ualign::b_1
            };
        }
        else
        {
            out_hailstorm.paths_data = ice::Data{};
        }
        return Res::Success;
    }

    struct Offsets
    {
        ice::usize chunks;
        ice::usize ids;
        ice::usize resources;
        ice::usize data;
        ice::usize paths_info;
        ice::usize paths_data;
    };

    auto cluster_meminfo(
        ice::ucount resource_count,
        ice::Span<HailstormChunk const> chunks,
        HailstormPaths paths,
        Offsets& out_offsets
    ) noexcept -> ice::meminfo
    {
        ice::meminfo hsmem = ice::meminfo_of<HailstormHeader>;
        out_offsets.paths_info = hsmem += ice::meminfo_of<HailstormPaths>;
        out_offsets.chunks = hsmem += ice::meminfo_of<HailstormChunk> * ice::span::count(chunks);
        out_offsets.resources = hsmem += ice::meminfo_of<HailstormResource> * resource_count;

        ICE_ASSERT_CORE(paths.size % ualign::b_8 == 0_B);
        out_offsets.paths_data = hsmem += { paths.size, ualign::b_8 };
        out_offsets.data = hsmem.size;

        for (HailstormChunk const& chunk : chunks)
        {
            hsmem.size += chunk.size;
        }
        return hsmem;
    }

    auto write_cluster_internal(
        ice::hailstorm::v1::HailstormWriteParams const& params,
        ice::hailstorm::v1::HailstormWriteData const& write_data
    ) noexcept -> ice::Memory
    {
        ice::ucount const res_count = ice::count(write_data.paths);

        ice::Array<HailstormChunk> chunks{ params.temp_alloc };
        ice::array::reserve(chunks, params.estimated_chunk_count);
        ice::array::push_back(chunks, params.initial_chunks);

        if (ice::count(chunks) == 0)
        {
            HailstormChunk new_chunk{};
            new_chunk = params.fn_create_chunk({}, {.alignment = ualign::b_8}, new_chunk, params.userdata);
            ice::array::push_back(chunks, new_chunk);
        }

        // Keep an array for all final chunk references.
        ice::Array<HailstormWriteChunkRef> refs{ params.temp_alloc };
        ice::array::resize(refs, res_count);

        // Sizes start with 0_B fill.
        ice::Array<ice::usize> sizes{ params.temp_alloc };
        ice::array::resize(sizes, ice::count(chunks));
        ice::array::memset(sizes, 0);

        // Metadata saved tracker.
        ice::Array<ice::u32> metatracker{ params.temp_alloc };
        ice::array::resize(metatracker, ice::count(write_data.metadata_mapping));
        ice::array::memset(metatracker, ice::u8_max);

        bool requires_data_writer_callback = false;

        // We go over all resources and create the list of final chunks.
        HailstormPaths paths_info{ .size = 8_B }; // If empty always contains eight '0' values.
        for (ice::ucount idx = 0; idx < res_count;)
        {
            ice::u32 metadata_idx = idx;

            // If the metadata is shared, check for the already assigned chunk
            if (ice::array::any(metatracker))
            {
                metadata_idx = write_data.metadata_mapping[idx];
            }

            ice::Metadata const& meta = write_data.metadata[metadata_idx];
            ice::Data data = write_data.data[idx];

            // Check if even one data object is not provided.
            requires_data_writer_callback |= data.location == nullptr;

            HailstormWriteChunkRef ref = params.fn_select_chunk(meta, data, chunks, params.userdata);
            ICE_ASSERT_CORE(ref.data_chunk < ice::count(chunks));
            ICE_ASSERT_CORE(ref.meta_chunk < ice::count(chunks));

            // If the metadata is shared, check for the already assigned chunk
            bool shared_meta = false;
            if (ice::array::any(metatracker))
            {
                if (metatracker[metadata_idx] != u32_max)
                {
                    shared_meta = true;
                    ref.meta_chunk = refs[metadata_idx].meta_chunk;
                }
            }

            ice::meminfo const metamem = ice::meta_meminfo(meta);
            ice::isize const data_remaining = (chunks[ref.data_chunk].size - sizes[ref.data_chunk])
                - isize{ static_cast<ice::isize::base_type>(data.alignment) };
            ice::isize const meta_remaining = chunks[ref.meta_chunk].size - sizes[ref.meta_chunk] - 8_B;

            // Check if we need to create a new chunk due to size restrictions.
            if (ref.data_chunk == ref.meta_chunk)
            {
                ref.data_create |= (data_remaining - metamem.size) < data.size;
                ref.meta_create = false; // We only want to create one chunk if both data and meta are the same.
            }
            else
            {
                ref.data_create |= data_remaining < data.size;
                ref.meta_create |= meta_remaining < metamem.size;
            }

            if (ref.data_create)
            {
                HailstormChunk const new_chunk = params.fn_create_chunk(
                    meta, data, chunks[ref.data_chunk], params.userdata
                );

                // Either mixed or data only chunks
                ICE_ASSERT_CORE(
                    (ref.data_chunk == ref.meta_chunk && new_chunk.type == 3)
                    || (new_chunk.type == 2)
                );

                // Push the new chunk
                ice::array::push_back(chunks, new_chunk);
            }

            if (ref.meta_create)
            {
                ICE_ASSERT_CORE(shared_meta == false); // This should not happen?
                HailstormChunk const new_chunk = params.fn_create_chunk(
                    meta, data, chunks[ref.meta_chunk], params.userdata
                );

                // Meta only chunks
                ICE_ASSERT_CORE(new_chunk.type == 1);

                // Push the new chunk
                ice::array::push_back(chunks, new_chunk);
            }

            // If chunks where created, re-do the selection
            if (ref.data_create || ref.meta_create)
            {
                // We don't want to increase the index yet
                continue;
            }

            // Only update the tracker once we are sure we have a final chunk selected
            if (ice::array::any(metatracker))
            {
                if (metatracker[metadata_idx] == u32_max)
                {
                    metatracker[metadata_idx] = idx;
                }
            }

            refs[idx] = ref;

            ICE_ASSERT_CORE(chunks[ref.meta_chunk].type & 0x1);
            ICE_ASSERT_CORE(chunks[ref.data_chunk].type & 0x2);

            chunks[ref.data_chunk].count_entries += 1;

            // Udpate the sizes array, however only update meta if it's not shared (not assigned to a chunk yet)
            if (shared_meta == false)
            {
                // Add an entry to a meta chunk only if it's not duplicated and if it's not mixed
                if (ref.data_chunk != ref.meta_chunk)
                {
                    chunks[ref.meta_chunk].count_entries += 1;
                }

                sizes[ref.meta_chunk] = ice::align_to(sizes[ref.meta_chunk], ice::ualign::b_8).value + metamem.size;
            }
            sizes[ref.data_chunk] = ice::align_to(sizes[ref.data_chunk], data.alignment).value + data.size;

            // Calculate total size needed for all paths to be stored
            ice::ucount const path_size = ice::string::size(write_data.paths[idx]);
            ICE_ASSERT_CORE(path_size > 0);
            paths_info.size += ice::usize{ path_size + 1 }; //ice::u8(path_size > 0)

            // Increase index at the end
            idx += 1;
        }

        // Either we don't need the callback or we need to have it provided!
        ICE_ASSERT_CORE(requires_data_writer_callback == false || params.fn_resource_write != nullptr);

        // Paths needs to be aligned to boundary of 8
        paths_info.size = ice::align_to(paths_info.size, ualign::b_8).value;

        // Reduce chunk sizes but align them to their alignment boundary.
        ice::u32 chunk_idx = 0;
        for (HailstormChunk& chunk : chunks)
        {
            chunk.size = ice::align_to(sizes[chunk_idx], chunk.align).value;
            chunk_idx += 1;
        }

        // Calculate an estimated size for the whole cluster.
        // TODO: This size is currently exact, but once we start compressing / encrypting this will no longer be the case.
        Offsets offsets;
        ice::meminfo const final_meminfo = cluster_meminfo(res_count, chunks, paths_info, offsets);
        ice::Memory const final_memory = params.cluster_alloc.allocate(final_meminfo);

        // Fill-in header data
        HailstormHeader header{
            .offset_next = final_meminfo.size,
            .offset_data = offsets.data,
            .version = { },
            .is_encrypted = false,
            .is_expansion = false,
            .is_patch = false,
            .is_baked = false,
            .count_chunks = ice::u16(ice::count(chunks)),
            .count_resources = ice::u16(res_count),
        };
        header.magic = Constant_HailstormMagic;
        header.header_version = Constant_HailstormHeaderVersion;
        header.header_size = offsets.paths_data;
        paths_info.offset = offsets.paths_data;

        // Copy custom values into the final header.
        static_assert(sizeof(write_data.custom_values) == sizeof(header.app_custom_values));
        ice::memcpy(header.app_custom_values, write_data.custom_values, sizeof(header.app_custom_values));

        // Place chunk offsets at their proper location.
        ice::usize chunk_offset = offsets.data;
        for (HailstormChunk& chunk : chunks)
        {
            chunk.size_origin = chunk.size;
            chunk.offset = ice::exchange(
                chunk_offset,
                ice::align_to(chunk_offset + chunk.size, ualign::b_8).value
            );
        }

        // Copy over all chunk data
        ice::memcpy(final_memory, ice::data_view(header));
        ice::memcpy(ice::ptr_add(final_memory, offsets.paths_info), ice::data_view(paths_info));
        ice::memcpy(ice::ptr_add(final_memory, offsets.chunks), ice::array::data_view(chunks));

        HailstormResource* const pack_resources = reinterpret_cast<HailstormResource*>(
            ice::ptr_add(final_memory, offsets.resources).location
        );

        ice::u32 paths_offset = 0;
        char* const paths_data = reinterpret_cast<char*>(
            ice::ptr_add(final_memory, offsets.paths_data).location
        );

        // Clear sizes
        ice::array::memset(sizes, 0);
        ice::array::memset(metatracker, ice::u8_max);

        // We now go over the list again, this time already filling data in.
        for (ice::ucount idx = 0; idx < res_count; ++idx)
        {
            HailstormResource& res = pack_resources[idx];
            res.chunk = refs[idx].data_chunk;
            res.meta_chunk = refs[idx].meta_chunk;

            HailstormChunk const& data_chunk = chunks[res.chunk];
            HailstormChunk const& meta_chunk = chunks[res.meta_chunk];

            // Get the index of the resource that stored the meta originally, or 'u32_max' if this is the first occurence.
            ice::u32 meta_idx = idx;
            ice::u32 meta_map_idx = ice::u32_max;
            if (ice::array::any(metatracker))
            {
                meta_idx = write_data.metadata_mapping[idx];
                meta_map_idx = ice::exchange(metatracker[meta_idx], idx);
            }

            // Calc, Store and ralign the used space so the next value can be already copied onto the proper location.
            if (meta_map_idx == ice::u32_max)
            {
                ice::usize& meta_chunk_used = sizes[res.meta_chunk];
                ice::Memory const meta_mem = ice::ptr_add(final_memory, meta_chunk.offset + meta_chunk_used);
                ice::usize const meta_size = ice::meta_store(write_data.metadata[meta_idx], meta_mem);

                // Store meta location
                res.meta_size = ice::u32(meta_size.value);
                res.meta_offset = ice::u32(meta_chunk_used.value);

                // Need to update the 'used' variable after we wrote the metadata
                meta_chunk_used = ice::align_to(meta_chunk_used + meta_size, meta_chunk.align).value;
            }
            else
            {
                res.meta_size = pack_resources[meta_map_idx].meta_size;
                res.meta_offset = pack_resources[meta_map_idx].meta_offset;
            }

            {
                ice::usize& data_chunk_used = sizes[res.chunk];
                ice::Data const data = write_data.data[idx];

                // Store data location
                res.size = ice::u32(data.size.value);
                res.offset = ice::u32(data_chunk_used.value);

                // Ensure the data view has an alignment smaller or equal to the chunk alignment.
                ICE_ASSERT_CORE(data.alignment <= data_chunk.align);
                ice::Memory const data_mem = ice::ptr_add(final_memory, data_chunk.offset + data_chunk_used);

                // If data has a nullptr locations, call the write callback to access resource data.
                // This allows us to "stream" input data to the final buffer.
                // TODO: It would be also good to provide a way to stream final data to a file also.
                if (data.location == nullptr)
                {
                    params.fn_resource_write(idx, write_data, data_mem, params.userdata);
                }
                else // We got the data so just copy it over
                {
                    ice::memcpy(data_mem, data);
                }

                data_chunk_used = ice::align_to(data_chunk_used + data.size, data_chunk.align).value;
            }

            {
                res.path_size = ice::string::size(write_data.paths[idx]);
                res.path_offset = paths_offset;

                // Copy and increment the offset with an '\0' character added.
                ice::usize const path_size = ice::usize{ res.path_size };
                ice::memcpy({ paths_data + paths_offset, path_size, ice::ualign::b_1 }, ice::string::data_view(write_data.paths[idx]));
                paths_offset += res.path_size + 1;
                paths_data[paths_offset - 1] = '\0';
            }
        }

        // Clear the final bytes required to be zeroed in the paths block
        ice::memset(ice::ptr_add(paths_data, { paths_offset }), 0, paths_info.size.value - paths_offset);

        return final_memory;
    }

    auto write_cluster_internal(
        ice::hailstorm::v1::HailstormWriteParams const& params,
        ice::hailstorm::v1::HailstormAsyncWriteParams const& stream_params,
        ice::hailstorm::v1::HailstormWriteData const& write_data
    ) noexcept -> ice::Task<>
    {
        ice::ucount const res_count = ice::count(write_data.paths);

        ice::Array<HailstormChunk> chunks{ params.temp_alloc };
        ice::array::reserve(chunks, params.estimated_chunk_count);
        ice::array::push_back(chunks, params.initial_chunks);

        if (ice::count(chunks) == 0)
        {
            HailstormChunk new_chunk{};
            new_chunk = params.fn_create_chunk({}, { .alignment = ualign::b_8 }, new_chunk, params.userdata);
            ice::array::push_back(chunks, new_chunk);
        }

        // Keep an array for all final chunk references.
        ice::Array<HailstormWriteChunkRef> refs{ params.temp_alloc };
        ice::array::resize(refs, res_count);

        // Sizes start with 0_B fill.
        ice::Array<ice::usize> sizes{ params.temp_alloc };
        ice::array::resize(sizes, ice::count(chunks));
        ice::array::memset(sizes, 0);

        // Metadata saved tracker.
        ice::Array<ice::u32> metatracker{ params.temp_alloc };
        ice::array::resize(metatracker, ice::count(write_data.metadata_mapping));
        ice::array::memset(metatracker, ice::u8_max);

        bool requires_data_writer_callback = false;

        // We go over all resources and create the list of final chunks.
        HailstormPaths paths_info{ .size = 8_B }; // If empty always contains eight '0' values.
        for (ice::ucount idx = 0; idx < res_count;)
        {
            ice::u32 metadata_idx = idx;

            // If the metadata is shared, check for the already assigned chunk
            if (ice::array::any(metatracker))
            {
                metadata_idx = write_data.metadata_mapping[idx];
            }

            ice::Metadata const& meta = write_data.metadata[metadata_idx];
            ice::Data data = write_data.data[idx];

            // Check if even one data object is not provided.
            requires_data_writer_callback |= data.location == nullptr;

            HailstormWriteChunkRef ref = params.fn_select_chunk(meta, data, chunks, params.userdata);
            ICE_ASSERT_CORE(ref.data_chunk < ice::count(chunks));
            ICE_ASSERT_CORE(ref.meta_chunk < ice::count(chunks));

            // If the metadata is shared, check for the already assigned chunk
            bool shared_meta = false;
            if (ice::array::any(metatracker))
            {
                if (metatracker[metadata_idx] != u32_max)
                {
                    shared_meta = true;
                    ref.meta_chunk = refs[metadata_idx].meta_chunk;
                }
            }

            ice::meminfo const metamem = ice::meta_meminfo(meta);
            ice::isize const data_remaining = (chunks[ref.data_chunk].size - sizes[ref.data_chunk])
                - isize{ static_cast<ice::isize::base_type>(data.alignment) };
            ice::isize const meta_remaining = chunks[ref.meta_chunk].size - sizes[ref.meta_chunk] - 8_B;

            // Check if we need to create a new chunk due to size restrictions.
            if (ref.data_chunk == ref.meta_chunk)
            {
                ref.data_create |= (data_remaining - metamem.size) < data.size;
                ref.meta_create = false; // We only want to create one chunk if both data and meta are the same.
            }
            else
            {
                ref.data_create |= data_remaining < data.size;
                ref.meta_create |= meta_remaining < metamem.size;
            }

            if (ref.data_create)
            {
                HailstormChunk const new_chunk = params.fn_create_chunk(
                    meta, data, chunks[ref.data_chunk], params.userdata
                );

                // Either mixed or data only chunks
                ICE_ASSERT_CORE(
                    (ref.data_chunk == ref.meta_chunk && new_chunk.type == 3)
                    || (new_chunk.type == 2)
                );

                // Push the new chunk
                ice::array::push_back(chunks, new_chunk);
            }

            if (ref.meta_create)
            {
                ICE_ASSERT_CORE(shared_meta == false); // This should not happen?
                HailstormChunk const new_chunk = params.fn_create_chunk(
                    meta, data, chunks[ref.meta_chunk], params.userdata
                );

                // Meta only chunks
                ICE_ASSERT_CORE(new_chunk.type == 1);

                // Push the new chunk
                ice::array::push_back(chunks, new_chunk);
            }

            // If chunks where created, re-do the selection
            if (ref.data_create || ref.meta_create)
            {
                // We don't want to increase the index yet
                continue;
            }

            // Only update the tracker once we are sure we have a final chunk selected
            if (ice::array::any(metatracker))
            {
                if (metatracker[metadata_idx] == u32_max)
                {
                    metatracker[metadata_idx] = idx;
                }
            }

            refs[idx] = ref;

            ICE_ASSERT_CORE(chunks[ref.meta_chunk].type & 0x1);
            ICE_ASSERT_CORE(chunks[ref.data_chunk].type & 0x2);

            chunks[ref.data_chunk].count_entries += 1;

            // Udpate the sizes array, however only update meta if it's not shared (not assigned to a chunk yet)
            if (shared_meta == false)
            {
                // Add an entry to a meta chunk only if it's not duplicated and if it's not mixed
                if (ref.data_chunk != ref.meta_chunk)
                {
                    chunks[ref.meta_chunk].count_entries += 1;
                }

                sizes[ref.meta_chunk] = ice::align_to(sizes[ref.meta_chunk], ice::ualign::b_8).value + metamem.size;
            }
            sizes[ref.data_chunk] = ice::align_to(sizes[ref.data_chunk], data.alignment).value + data.size;

            // Calculate total size needed for all paths to be stored
            ice::ucount const path_size = ice::string::size(write_data.paths[idx]);
            ICE_ASSERT_CORE(path_size > 0);
            paths_info.size += ice::usize{ path_size + 1 }; //ice::u8(path_size > 0)

            // Increase index at the end
            idx += 1;
        }

        // Either we don't need the callback or we need to have it provided!
        ICE_ASSERT_CORE(requires_data_writer_callback == false || params.fn_resource_write != nullptr);

        // Paths needs to be aligned to boundary of 8
        paths_info.size = ice::align_to(paths_info.size, ualign::b_8).value;

        // Reduce chunk sizes but align them to their alignment boundary.
        ice::u32 chunk_idx = 0;
        for (HailstormChunk& chunk : chunks)
        {
            chunk.size = ice::align_to(sizes[chunk_idx], chunk.align).value;
            chunk_idx += 1;
        }

        // Calculate an estimated size for the whole cluster.
        // TODO: This size is currently exact, but once we start compressing / encrypting this will no longer be the case.
        Offsets offsets;
        ice::meminfo const final_meminfo = cluster_meminfo(res_count, chunks, paths_info, offsets);
        detail::TrackedStream stream{ stream_params, final_meminfo.size };

        // Fill-in header data
        HailstormHeader header{
            .offset_next = final_meminfo.size,
            .offset_data = offsets.data,
            .version = { },
            .is_encrypted = false,
            .is_expansion = false,
            .is_patch = false,
            .is_baked = false,
            .count_chunks = ice::u16(ice::count(chunks)),
            .count_resources = ice::u16(res_count),
        };
        header.magic = Constant_HailstormMagic;
        header.header_version = Constant_HailstormHeaderVersion;
        header.header_size = offsets.paths_data;
        paths_info.offset = offsets.paths_data;

        // Copy custom values into the final header.
        static_assert(sizeof(write_data.custom_values) == sizeof(header.app_custom_values));
        ice::memcpy(header.app_custom_values, write_data.custom_values, sizeof(header.app_custom_values));

        // Place chunk offsets at their proper location.
        ice::usize chunk_offset = offsets.data;
        for (HailstormChunk& chunk : chunks)
        {
            chunk.size_origin = chunk.size;
            chunk.offset = ice::exchange(
                chunk_offset,
                ice::align_to(chunk_offset + chunk.size, ualign::b_8).value
            );
        }

        // Copy over all chunk data
        co_await stream.write_header(ice::data_view(header), 0_B);
        co_await stream.write_header(ice::data_view(paths_info), offsets.paths_info);
        co_await stream.write_header(ice::array::data_view(chunks), offsets.chunks);

        // Prepare temporary data for resources and paths
        detail::TrackedMemory temp_resource_mem{ params.temp_alloc, ice::size_of<HailstormResource> * res_count };
        detail::TrackedMemory temp_paths_mem{ params.temp_alloc, paths_info.size };

        HailstormResource* const pack_resources = reinterpret_cast<HailstormResource*>(
            temp_resource_mem.location
        );

        ice::u32 paths_offset = 0;
        char* const paths_data = reinterpret_cast<char*>(
            temp_paths_mem.location
        );

        // Clear sizes
        ice::array::memset(sizes, 0);
        ice::array::memset(metatracker, ice::u8_max);

        // We now go over the list again, this time already filling data in.
        for (ice::ucount idx = 0; idx < res_count; ++idx)
        {
            HailstormResource& res = pack_resources[idx];
            res.chunk = refs[idx].data_chunk;
            res.meta_chunk = refs[idx].meta_chunk;

            HailstormChunk const& data_chunk = chunks[res.chunk];
            HailstormChunk const& meta_chunk = chunks[res.meta_chunk];

            // Get the index of the resource that stored the meta originally, or 'u32_max' if this is the first occurence.
            ice::u32 meta_idx = idx;
            ice::u32 meta_map_idx = ice::u32_max;
            if (ice::array::any(metatracker))
            {
                meta_idx = write_data.metadata_mapping[idx];
                meta_map_idx = ice::exchange(metatracker[meta_idx], idx);
            }

            // Calc, Store and ralign the used space so the next value can be already copied onto the proper location.
            if (meta_map_idx == ice::u32_max)
            {
                ice::usize& meta_chunk_used = sizes[res.meta_chunk];

                co_await stream.write_metadata(
                    write_data, meta_idx, meta_chunk.offset + meta_chunk_used
                );

                // Store meta location
                ice::meminfo const meta_meminfo = ice::meta_meminfo(write_data.metadata[meta_idx]);
                res.meta_size = ice::u32(meta_meminfo.size.value);
                res.meta_offset = ice::u32(meta_chunk_used.value);

                // Need to update the 'used' variable after we wrote the metadata
                meta_chunk_used = ice::align_to(meta_chunk_used + meta_meminfo.size, meta_chunk.align).value;
            }
            else
            {
                res.meta_size = pack_resources[meta_map_idx].meta_size;
                res.meta_offset = pack_resources[meta_map_idx].meta_offset;
            }

            {
                ice::usize& data_chunk_used = sizes[res.chunk];
                ice::Data const data = write_data.data[idx];

                // Store data location
                res.size = ice::u32(data.size.value);
                res.offset = ice::u32(data_chunk_used.value);

                co_await stream.write_resource(
                    write_data, idx, data_chunk.offset + data_chunk_used
                );

                // Ensure the data view has an alignment smaller or equal to the chunk alignment.
                ICE_ASSERT_CORE(data.alignment <= data_chunk.align);

                data_chunk_used = ice::align_to(data_chunk_used + data.size, data_chunk.align).value;
            }

            {
                res.path_size = ice::string::size(write_data.paths[idx]);
                res.path_offset = paths_offset;

                // Copy and increment the offset with an '\0' character added.
                ice::usize const path_size = ice::usize{ res.path_size };
                ice::memcpy({ paths_data + paths_offset, path_size, ice::ualign::b_1 }, ice::string::data_view(write_data.paths[idx]));
                paths_offset += res.path_size + 1;
                paths_data[paths_offset - 1] = '\0';
            }
        }

        // Clear the final bytes required to be zeroed in the paths block
        ice::memset(ice::ptr_add(paths_data, { paths_offset }), 0, paths_info.size.value - paths_offset);

        // Write final memory information
        co_await stream.write_header(ice::data_view(temp_paths_mem), offsets.paths_data);
        co_await stream.write_header(ice::data_view(temp_resource_mem), offsets.resources);
        stream.close();
        co_return;
    }

    auto write_cluster(
        ice::hailstorm::v1::HailstormWriteParams const& params,
        ice::hailstorm::v1::HailstormWriteData const& data
    ) noexcept -> ice::Memory
    {
        ice::ucount const count_ids = ice::count(data.paths);
        ICE_ASSERT_CORE(count_ids == ice::count(data.data));
        ICE_ASSERT_CORE(count_ids == ice::count(data.metadata) || count_ids <= ice::count(data.metadata_mapping));

        return write_cluster_internal(
            params,
            data
        );
    }

    bool write_cluster_async(
        ice::hailstorm::v1::HailstormAsyncWriteParams const& params,
        ice::hailstorm::v1::HailstormWriteData const& data
    ) noexcept
    {
        ice::ucount const count_ids = ice::count(data.paths);
        ICE_ASSERT_CORE(count_ids == ice::count(data.data));
        ICE_ASSERT_CORE(count_ids == ice::count(data.metadata) || count_ids <= ice::count(data.metadata_mapping));

        ice::ManualResetEvent finished{};
        ice::manual_wait_for(
            write_cluster_internal(
                params.base_params,
                params,
                data
            ),
            finished
        );

        return finished.is_set();
    }

    auto prefixed_resource_paths_size(
        ice::hailstorm::v1::HailstormPaths const& paths_info,
        ice::ucount count_resources,
        ice::String prefix
    ) noexcept -> ice::usize
    {
        ice::usize const extended_size = { count_resources * ice::string::size(prefix) };
        return paths_info.size + extended_size;
    }

    bool prefix_resource_paths(
        ice::hailstorm::v1::HailstormPaths const& paths_info,
        ice::Span<ice::hailstorm::v1::HailstormResource> resources,
        ice::Memory paths_data,
        ice::String prefix
    ) noexcept
    {
        ice::ucount const count_resources = ice::count(resources);
        ice::usize const size_extended_paths = prefixed_resource_paths_size(
            paths_info, count_resources, prefix
        );
        if (size_extended_paths > paths_data.size)
        {
            return false;
        }

        ice::ucount const size_prefix = ice::string::size(prefix);
        ice::ucount const size_extending = size_prefix * count_resources;

        // We find the first non '0' character. The .hsc ensures the last few bytes in the pats chunk are zeros.
        char* const paths_start = reinterpret_cast<char*>(paths_data.location);
        char* paths_end = paths_start + paths_info.size.value;
        char* ex_paths_end = paths_end + size_extending;
        while (paths_end[-1] == '\0')
        {
            paths_end -= 1;
            ex_paths_end -= 1;
        }

        // Iterate over each resource, update it's path (from last to first)
        ice::u32 resource_idx;
        for (resource_idx = count_resources; ex_paths_end > paths_start && resource_idx > 0; --resource_idx)
        {
            v1::HailstormResource& res = resources[resource_idx - 1];
            *ex_paths_end = '\0';

            ex_paths_end -= res.path_size;
            ice::memcpy(ex_paths_end, paths_start + res.path_offset, res.path_size);

            ex_paths_end -= size_prefix;
            ice::memcpy(ex_paths_end, ice::string::begin(prefix), size_prefix);

            // Update resource path information
            res.path_offset = ice::u32(ice::ptr_distance(paths_start, ex_paths_end).value);
            res.path_size += size_prefix;
            ex_paths_end -= 1;
        }

        // Ensure we finished at the proper location
        ICE_ASSERT_CORE((ex_paths_end + 1) == paths_start);
        return resource_idx == 0 && (ex_paths_end + 1) == paths_start;
}

} // namespace ice::hailstorm::v1
