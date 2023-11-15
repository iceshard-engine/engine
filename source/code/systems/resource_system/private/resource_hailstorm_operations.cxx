#include <ice/resource_hailstorm_operations.hxx>
#include <ice/container/array.hxx>

namespace ice::hailstorm::v1
{

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

    auto write_cluster(
        ice::hailstorm::v1::HailstormWriteParams const& params,
        ice::Span<ice::String const> res_paths,
        ice::Span<ice::Data const> res_data,
        ice::Span<ice::Metadata const> res_meta,
        ice::Span<ice::u32 const> res_meta_map
    ) noexcept -> ice::Memory
    {
        ice::ucount const res_count = ice::count(res_paths);

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
        ice::array::resize(metatracker, ice::count(res_meta_map));
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
                metadata_idx = res_meta_map[idx];
            }

            ice::Metadata const& meta = res_meta[idx];
            ice::Data data = res_data[metadata_idx];

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
            ice::ucount const path_size = ice::string::size(res_paths[idx]);
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
        // TODO2: Might want to introduce a callback for streamed writing.
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
                meta_idx = res_meta_map[idx];
                meta_map_idx = ice::exchange(metatracker[meta_idx], idx);
            }

            // Calc, Store and ralign the used space so the next value can be already copied onto the proper location.
            if (meta_map_idx == ice::u32_max)
            {
                ice::usize& meta_chunk_used = sizes[res.meta_chunk];
                ice::Memory const meta_mem = ice::ptr_add(final_memory, meta_chunk.offset + meta_chunk_used);
                ice::usize const meta_size = ice::meta_store(res_meta[meta_idx], meta_mem);

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
                ice::Data const data = res_data[idx];

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
                    params.fn_resource_write(res_paths[idx], res_meta[idx], data_mem, params.userdata);
                }
                else // We got the data so just copy it over
                {
                    ice::memcpy(data_mem, data);
                }

                data_chunk_used = ice::align_to(data_chunk_used + data.size, data_chunk.align).value;
            }

            {
                res.path_size = ice::string::size(res_paths[idx]);
                res.path_offset = paths_offset;

                // Copy and increment the offset with an '\0' character added.
                ice::usize const path_size = ice::usize{ res.path_size };
                ice::memcpy({ paths_data + paths_offset, path_size, ice::ualign::b_1 }, ice::string::data_view(res_paths[idx]));
                paths_offset += res.path_size + 1;
                paths_data[paths_offset - 1] = '\0';
            }
        }

        // Clear the final bytes required to be zeroed in the paths block
        ice::memset(ice::ptr_add(paths_data, { paths_offset }), 0, paths_info.size.value - paths_offset);

        return final_memory;
    }

    auto write_cluster(
        ice::hailstorm::v1::HailstormWriteParams const& params,
        ice::hailstorm::v1::HailstormWriteData const& data
    ) noexcept -> ice::Memory
    {
        ice::ucount const count_ids = ice::count(data.paths);
        ICE_ASSERT_CORE(count_ids == ice::count(data.data));
        ICE_ASSERT_CORE(count_ids == ice::count(data.metadata) || count_ids <= ice::count(data.metadata_mapping));

        return write_cluster(params, data.paths, data.data, data.metadata, data.metadata_mapping);
    }

} // namespace ice::hailstorm::v1
