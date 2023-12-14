/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/string_types.hxx>
#include <ice/resource_hailstorm.hxx>

namespace ice::hailstorm
{

    namespace v1
    {

        //! \brief Reads hailstorm cluser form the given input containing at least the whole header data.
        //! \note If successful the passed HailstormData struct will have all field updated.
        //!
        //! \param [in] header_data Containing the whole hailstorm header description.
        //! \param [out] out_hailstorm Hailstorm object that will have fields updated to access header data easily.
        //! \return 'Res::Success' if the cluster was valid and version was supported, otherwise an error with the load error.
        auto read_header(
            ice::Data header_data,
            ice::hailstorm::v1::HailstormData& out_hailstorm
        ) noexcept -> ice::Result;

        //! \brief Creates a new Hailstorm cluster based on the write params and provided resource information.
        //!
        //! \note Because HS format is quite complex when it comes to writing the creation is handled internally,
        //!   however chunk selection and data transforms are defined using the HailstormWriteParams struct.
        //!   This allows for the main routine to stay stable and handle all boilter plate regarding resource
        //!   iterations, but flexible enough to have full control on how resources are stored in the final cluster.
        //!
        //! \pre All three lists describing resource information are of the same size.
        //!
        //! \param [in] params Write params containing logic and detailed information on how to create a final HS cluster.
        //! \param [in] data A struct containg the data describing all resources to be stored in this cluster.
        //!
        //! \return Allocated memory ready to be written to a file. Returns an empty block if the operation fails.
        auto write_cluster(
            ice::hailstorm::v1::HailstormWriteParams const& params,
            ice::hailstorm::v1::HailstormWriteData const& data
        ) noexcept -> ice::Memory;

        //! \brief Creates a new Hailstorm cluster based on the write params and provided resource information.
        //!
        //! \note This function requires the user to set all async functions to properly handle writing data.
        //!   Additionally there are no guarantees that write requests are in order. Always use the offset to write
        //!   data into it's expected location.
        //! \note Because HS format is quite complex when it comes to writing the creation is handled internally,
        //!   however chunk selection and data writing are defined using the HailstormAsyncWriteParams struct.
        //!   This allows for the main routine to stay stable and handle all boilter plate regarding resource
        //!   iterations, but flexible enough to have full control on how data is written.
        //!
        //! \pre All three lists describing resource information are of the same size.
        //!
        //! \param [in] params Write params containing logic and detailed information on how to create a final HS cluster.
        //! \param [in] data A struct containg the data describing all resources to be stored in this cluster.
        //!
        //! \return Allocated memory ready to be written to a file. Returns an empty block if the operation fails.
        bool write_cluster_async(
            ice::hailstorm::v1::HailstormAsyncWriteParams const& params,
            ice::hailstorm::v1::HailstormWriteData const& data
        ) noexcept;

        //! \brief Returns the total size necessary to store all path data with an prefix appended to each entry.
        //! \param [in] paths_info Path information coming from a hailstorm header.
        //! \param [in] resource_count Number of resources this prefix will be appended to.
        //! \param [in] prefix The prefix to be appended to each path.
        //! \return Size in bytes required for the path buffer to store all entries with the given prefix. \see prefix_resource_paths.
        auto prefixed_resource_paths_size(
            ice::hailstorm::v1::HailstormPaths const& paths_info,
            ice::ucount resource_count,
            ice::String prefix
        ) noexcept -> ice::usize;

        //! \brief Updates resource paths stored in memory with enough with a given prefix and updates the given resource list.
        //! \note The memory needs to be big enoguh to store all paths with the appended prefix, \see prefixed_resource_paths_size.
        //!
        //! \warning The passed buffer is required to have paths data at the start.
        //! \warning The operation will update the buffer contents and resource information.
        //! \warning It's is REQUIRED that this function works on the entire resource list.
        //!
        //! \param [in] paths_info Path information coming from a hailstorm header.
        //! \param [in] resources List of ALL resources that are part of the paths buffer.
        //! \param [in] resources The memory block containing path data and additional space to contain all prefixed entries.
        //! \param [in] prefix The prefix to be appended to each path.
        //! \return 'true' If the update was successful and all data could be updated.
        bool prefix_resource_paths(
            ice::hailstorm::v1::HailstormPaths const& paths_info,
            ice::Span<ice::hailstorm::v1::HailstormResource> resources,
            ice::Memory paths_data,
            ice::String prefix
        ) noexcept;

        //! \brief The data to be provided when writing a hailstorm cluster.
        struct HailstormWriteData
        {
            //! \brief A list of 'paths' for each entry in the `data` array.
            //! \note A path can be any string identifier for resources. However it's recommended to follow the URI format.
            ice::Span<ice::String const> paths;

            //! \brief A list of data blocks to be written to the hailstorm cluster.
            //! \note This list is required to be the size of 'ids'.
            ice::Span<ice::Data const> data;

            //! \brief A list of metadata entries to be writted to the hailstorm cluster.
            //! \note If 'metadata_mapping' is empty this list is required to be the size of 'ids'.
            //!
            //! \details This list may be smaller than the number of resources, if and only if, the 'metadata_mapping' is
            //!   provided and is equal to the number of written resources. This allows to store a single metadata entry
            //!   for multiple resources to reduce the total size of the file and runtime footprint.
            ice::Span<ice::Data const> metadata;

            //! \brief A list of indices referencing one of the Metadata objects in the 'metadata' list.
            //! \note If provided, this list is required to be the size of 'ids'.
            ice::Span<ice::u32 const> metadata_mapping;

            //! \brief Application custom values.
            ice::u64 custom_values[2];
        };

        //! \brief Used to select chunks for resource metadata and data destinations.
        //! \note If metadata mapping is used, the value of 'meta_chunk' may be ignored in favor for the shared metadata object.
        struct HailstormWriteChunkRef
        {
            //! \brief Chunk index where data should be stored.
            ice::u16 data_chunk;

            //! \brief Chunk index where data should be stored.
            ice::u16 meta_chunk;

            //! \brief If 'true' a new chunk will be created and the 'data_chunk' value will be used as a base.
            bool data_create = false;

            //! \brief If 'true' a new chunk will be created and the 'meta_chunk' value will be used as a base.
            bool meta_create = false;
        };

        //! \brief A description of the 'write' operation for a Hailstorm cluster. Allows to partially control how
        //!   the resulting hailstorm cluster looks.
        //! \attention Please make sure you properly fill 'required' members or use default values.
        struct HailstormWriteParams
        {
            //! \brief Function signature for a chunk selection heuristic. The returned structure will be used
            //!   to select or create a chunk for the given resource information.
            //!
            //! \attention If creation of new chunks was requested, the resource will be checked again.
            //!   In addition, the returned chunk indices will be used to serve as 'base_chunk' for the create heuristic.
            //!
            //! \param [in] resource_meta Metadata associated with the given resource.
            //! \param [in] resource_data Object data associated with the given resource.
            //! \param [in] chunks List of already existing chunks.
            //! \param [in] userdata Value passed by the user using the 'HailstormWriteParams' struct.
            //! \return Indices for the data and metadata pair given.
            using ChunkSelectFn = auto(
                ice::Data resource_meta,
                ice::Data resource_data,
                ice::Span<ice::hailstorm::v1::HailstormChunk const> chunks,
                void* userdata
            ) noexcept -> ice::hailstorm::v1::HailstormWriteChunkRef;

            //! \brief Function signature for chunk creation heuristic. The returned chunk definition will be used
            //!   to create a new chunk.
            //!
            //! \attention If the writing started without a list of pre-defined chunks, this function is called once
            //!   to define the first chunk. The 'base_chunk' variable in such a case is entierly empty.
            //!
            //! \param [in] resource_meta Metadata associated with the given resource.
            //! \param [in] resource_data Object data associated with the given resource.
            //! \param [in] base_chunk The base definition for the new chunk, based on the index returned from the 'select' function.
            //! \param [in] userdata Value passed by the user using the 'HailstormWriteParams' struct.
            //! \return A chunk definition that will be used to start a new chunk in the cluster.
            using ChunkCreateFn = auto(
                ice::Data resource_meta,
                ice::Data resource_data,
                ice::hailstorm::v1::HailstormChunk base_chunk,
                void* userdata
            ) noexcept -> ice::hailstorm::v1::HailstormChunk;

            //! \brief
            using ResouceWriteFn = auto(
                ice::u32 resource_index,
                ice::hailstorm::v1::HailstormWriteData const& write_data,
                ice::Memory memory,
                void* userdata
            ) noexcept -> bool;

            //! \brief Allocator object used to handle various temporary allocations.
            ice::Allocator& temp_alloc;

            //! \brief Allocator object used to allocate the final memory for writing.
            //! \note Unused during if using asynchronous write.
            ice::Allocator& cluster_alloc;

            //! \brief List of initial chunks to be part of the cluster.
            //! \attention This list is not curated and empty chunks may end up in the cluster.
            ice::Span<ice::hailstorm::v1::HailstormChunk const> initial_chunks;

            //! \brief Estimated number of chunks in the final cluster, allows to minimize temporary allocations.
            ice::u32 estimated_chunk_count = 0;

            //! \brief Please see documentation of ChunkSelectFn.
            ChunkSelectFn* fn_select_chunk;

            //! \brief Please see documentation of ChunkCreateFn.
            ChunkCreateFn* fn_create_chunk;

            //! \brief Please see documentation of ResouceWriteFn.
            ResouceWriteFn* fn_resource_write;

            //! \brief User provided value, can be anything, passed to function routines.
            void* userdata;
        };

        //! \brief A description of a async write operation for a Hailstorm cluster. Allows to partially control how
        //!   the resulting hailstorm cluster looks.
        //! \note This description is an extension of the regular write params description.
        //! \note All async function calls need to be provided by the user.
        struct HailstormAsyncWriteParams
        {
            HailstormWriteParams base_params;

            using AsyncOpenFn = auto(
                ice::usize final_cluster_size,
                void* userdata
            ) noexcept -> bool;

            using AsyncWriteHeaderFn = auto(
                ice::Data header_data,
                ice::usize write_offset,
                void* userdata
            ) noexcept -> bool;

            using AsyncWriteDataFn = auto(
                ice::hailstorm::v1::HailstormWriteData const& write_data,
                ice::u32 resource_index,
                ice::usize write_offset,
                void* userdata
            ) noexcept -> bool;

            using AsyncCloseFn = auto(
                void* userdata
            ) noexcept -> bool;

            AsyncOpenFn* fn_async_open;
            AsyncWriteHeaderFn* fn_async_write_header;
            AsyncWriteDataFn* fn_async_write_metadata;
            AsyncWriteDataFn* fn_async_write_resource;
            AsyncCloseFn* fn_async_close;

            //! \brief User provided value, can be anything, passed to function routines.
            void* async_userdata;
        };

        //! \brief Default heuristic for creating chunks.
        //! \note This function is suboptimal, it always returns Mixed chunk types with Regular persitance strategy.
        //!   Each chunk is at most 32_MiB big and files bigger than that will be stored in exclusive chunks.
        inline auto default_chunk_create_logic(
            ice::Data resource_meta,
            ice::Data resource_data,
            ice::hailstorm::v1::HailstormChunk base_chunk_info,
            void* /*userdata*/
        ) noexcept -> ice::hailstorm::v1::HailstormChunk
        {
            // If empty, set the first chunk so later it can be used as the base chunk
            if (base_chunk_info.size == 0_B)
            {
                base_chunk_info.align = ice::ualign::b_8;
                base_chunk_info.is_compressed = false;
                base_chunk_info.is_encrypted = false;
                base_chunk_info.persistance = 1; // Persistance is regular by default
                base_chunk_info.type = 3; // Type is mixed by default
                base_chunk_info.size = 32_MiB; // By default chunks should be at max 32MiB
            }

            // Calculate chunk size (meta + data)
            ice::meminfo chunk_meminfo = { resource_meta.size, resource_meta.alignment };
            chunk_meminfo += { resource_data.size, resource_data.alignment };

            // Base chunk should always be 32_MiB unless the resources requires more
            if (chunk_meminfo.size > 32_MiB || resource_data.alignment != ualign::b_8)
            {
                base_chunk_info.size = chunk_meminfo.size;
                base_chunk_info.align = resource_data.alignment;
            }
            return base_chunk_info;
        }

        //! \note This function is suboptimal, it assumes all chunks are mixed and always assigns both
        //!   data and metadata to the last chunk. New chunks will be created if the selected chunk is to small.
        inline auto default_chunk_select_logic(
            ice::Metadata const& /*resource_meta*/,
            ice::Data /*resource_data*/,
            ice::Span<ice::hailstorm::v1::HailstormChunk const> chunks,
            void* /*userdata*/
        ) noexcept -> ice::hailstorm::v1::HailstormWriteChunkRef
        {
            // Always pick last chunk, if it's too small a 'create' chunk will be called and the select logic will be repeated.
            ice::u16 const last_chunk = ice::u16(ice::count(chunks) - 1);

            // Default selection only supports mixed chunks.
            ICE_ASSERT_CORE(chunks[last_chunk].type == 3);
            return {
                .data_chunk = last_chunk,
                .meta_chunk = last_chunk
            };
        }

    } // namespace v1

} // namespace ice
