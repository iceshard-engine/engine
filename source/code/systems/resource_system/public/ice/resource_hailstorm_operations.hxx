#pragma once
#include <ice/resource_hailstorm.hxx>
#include <ice/resource_meta.hxx>

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
            ice::Span<ice::Metadata const> metadata;

            //! \brief A list of indices referencing one of the Metadata objects in the 'metadata' list.
            //! \note If provided, this list is required to be the size of 'ids'.
            ice::Span<ice::u32 const> metadata_mapping;

            //! \brief Application custom values.
            ice::u64 custom_values[3];
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
                ice::Metadata const& resource_meta,
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
                ice::Metadata const& resource_meta,
                ice::Data resource_data,
                ice::hailstorm::v1::HailstormChunk base_chunk,
                void* userdata
            ) noexcept -> ice::hailstorm::v1::HailstormChunk;

            //! \brief
            using ResouceWriteFn = auto(
                ice::String path,
                ice::Metadata const& resource_meta,
                ice::Memory memory,
                void* userdata
            ) noexcept -> bool;

            //! \brief Allocator object used to handle various temporary allocations.
            ice::Allocator& temp_alloc;

            //! \brief Allocator object used to allocate the final memory for writing.
            //! \todo Allow for streamed writing, might need to make this a pointer since this will probably require
            //!   a callback approach.
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

        //! \brief Default heuristic for creating chunks.
        //! \note This function is suboptimal, it always returns Mixed chunk types with Regular persitance strategy.
        //!   Each chunk is at most 32_MiB big and files bigger than that will be stored in exclusive chunks.
        inline auto default_chunk_create_logic(
            ice::Metadata const& resource_meta,
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
            ice::meminfo chunk_meminfo = ice::meta_meminfo(resource_meta);
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
