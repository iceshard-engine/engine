#pragma once
#include <ice/span.hxx>
#include <ice/stringid.hxx>
#include <ice/mem_allocator.hxx>
#include <ice/resource_types.hxx>
#include <ice/result_codes.hxx>

namespace ice::hailstorm
{

    //! \brief A word value used to identify the Hailstorm format.
    static constexpr ice::u64 Constant_HailstormMagic = 'ISHS';

    //! \brief A word value used to identify the Hailstorm format specification version.
    static constexpr ice::u32 Constant_HailstormHeaderVersion = 'HSC0';

    //! \brief A base header, always present in any Hailstorm version. Allows to properly select the API version
    //!   and the total size of header data.
    //! \note As long as the whole header size is loaded into memory, all header values, regardless of the version,
    //!   are accessible.
    struct HailstormHeaderBase
    {
        //! \brief Magic value, selected once and never changes for this format. Use for validation / format detection.
        ice::u32 magic;

        //! \brief The current version of the data format. This may change but will rarely happen.
        //! \note Differences between header versions might be breaking so each version should be handled separately.
        ice::u32 header_version;

        //! \brief The total size of header data. Allows to load only most important parts of a hailstorm file.
        //! \note The data size to be loaded allows to access all information about available resources.
        //! \note This does not include 'paths' data of resource.
        ice::usize header_size;
    };

    namespace v1
    {

        //! \brief Hailstorm header for version HSC0-X.Y.Z
        struct HailstormHeader : HailstormHeaderBase
        {
            //! \brief The next hailstorm header chunk. Only usable if a file was patched later and contains additional data.
            //! \note This header needs to be handled simillary to the original header. Which means reading the base header, and then the header size.
            ice::usize offset_next;

            //! \brief The offset at which actuall file data and metadata values are stored.
            //! \note Using this value for 'header_size' instead allows to also load all 'paths' for resources.
            ice::usize offset_data;

            //! \brief The engine version on which this data was created. May introduces minor change to the header format, but is not allowed to change the main headers ABI.
            //! \note All other HS header types (chunk, resource, resource_id, paths) may introduce ABI breaking changes.
            ice::u8 version[3];

            //! \brief ALL chunk data is encrypted separately. Load the whole file and decrypt it before reading.
            ice::u8 is_encrypted : 1;

            //! \brief This is an expansion data pack and does only contain patched or additional game/program data.
            ice::u8 is_expansion : 1;

            //! \brief This is an patch pack and does only contain updated versions of existing resources.
            ice::u8 is_patch : 1;

            //! \brief The data stored in this pack is pre-baked and can be consumed directly by most engine systems.
            ice::u8 is_baked : 1;

            //! \brief Reserved for future use.
            //! \version HSC0-0.0.1
            ice::u8 _unused05b : 4;

            //! \brief Number of data chunks in this pack.
            ice::u16 count_chunks;

            //! \brief Number of resources in this pack.
            ice::u16 count_resources;
        };

        static_assert(sizeof(HailstormHeaderBase) == 16);
        static_assert(sizeof(HailstormHeader) - sizeof(HailstormHeaderBase) == 24);
        static_assert(sizeof(HailstormHeader) == 40);

        //! \brief Hailstorm path information. Optional, might not contain actuall data.
        //! \version HSC0-0.0.1
        struct HailstormPaths
        {
            ice::usize offset;
            ice::usize size;
        };

        static_assert(sizeof(HailstormPaths) == 16);

        //! \brief Hailstorm chunk information used to optimize loading and keeping resources in memory.
        //! \version HSC0-0.0.1
        struct HailstormChunk
        {
            //! \brief Offset in file where chunk data is stored.
            ice::usize offset;

            //! \brief Total size of chunk data.
            ice::usize size;

            //! \brief The size of the stored data when uncompressed and/or decrypted. Use this value
            //!   to allocate the final runtime memory block.
            //! \note Value is equal to 'size' if 'is_compressed == 0' and 'is_encrypted == 0'
            ice::usize size_origin;

            //! \brief Alignment requirements of the data stored in the chunk.
            //! \note This requirements is applied to each resource, however loading the whole chunk with the given alignment
            //!   ensures all resources data is stored at the proper alignment.
            ice::ualign align;

            //! \brief The type of data stored in this chunk. One of: Metadata = 1, FileData = 2, Mixed = 3
            ice::u8 type : 2;

            //! \brief The preffered loading strategy. One of: Temporary = 0, Regular = 1, LoadIfPossible = 2, LoadAlways = 3
            //! \note Persistance details:
            //!   * 'temporary' - used for one-use files that can be released soon after. (value: 0)
            //!   * 'regular' - used for on-demand loading, but can be unloaded if necessary and unused. (value: 1)
            //!   * 'load if possible' - used for common resources files that allow to reduce loading times in various locations. (value: 2)
            //!   * 'force-load' - used for resources that are accessed all the time and/or should never be reloaded. (value: 3)
            ice::u8 persistance : 4;

            //! \brief The chunk data is encrypted separately. Load the whole chunk and decrypt it before reading.
            //! \note If data was also compressed, this step is done BEFORE decompressing the data.
            ice::u8 is_encrypted : 1;

            //! \brief The chunk data needs to be decompressed before reading.
            //! \note If data was also encrypted, this step is done AFTER decrypting the data.
            ice::u8 is_compressed : 1;

            //! \brief Reserved for future use.
            ice::u8 _reserved0[1];

            //! \brief Number of entries stored in this chunk. Can be used to optimize runtime allocations.
            ice::u16 count_entries;
        };

        static_assert(sizeof(HailstormChunk) == 32);

        //! \brief Hailstorm resource information, used to access resource related data.
        //! \version HSC0-0.0.1
        struct HailstormResource
        {
            //! \brief The chunk index at which resource data is stored.
            ice::u16 chunk;

            //! \brief The chunk index at which resource metadata is stored.
            ice::u16 meta_chunk;

            //! \brief The offset at which data is stored. This value is relative to the chunk it is stored in.
            ice::u32 offset;

            //! \brief The size of the stored data.
            ice::u32 size;

            //! \brief The offset at which metadata is stored. This value is relative to the meta_chunk it is stored in.
            ice::u32 meta_offset;

            //! \brief The size of the stored metadata.
            ice::u32 meta_size;

            //! \brief The offset at which path information is stored. This value is relative to the HailsormPaths offset member.
            ice::u32 path_offset;

            //! \brief The size of the path.
            ice::u32 path_size;
        };

        static_assert(sizeof(HailstormResource) == 28);

        //! \brief Struct providing access to Hailstorm header data wrapped in a more accessible way.
        //! \note This struct can be filled used some of the hailstorm functions.
        struct HailstormData
        {
            ice::hailstorm::v1::HailstormHeader header;
            ice::Span<ice::hailstorm::v1::HailstormChunk const> chunks;
            ice::Span<ice::hailstorm::v1::HailstormResource const> resources;
            ice::hailstorm::v1::HailstormPaths paths;
            ice::Data paths_data;
        };

        struct HailstormReadParams;
        struct HailstormWriteParams;
        struct HailstormWriteData;

    } // namespace v1


    using HailstormHeader = v1::HailstormHeader;
    using HailstormPaths = v1::HailstormPaths;
    using HailstormChunk = v1::HailstormChunk;
    using HailstormResource = v1::HailstormResource;
    using HailstormData = v1::HailstormData;

} // namespace ice
