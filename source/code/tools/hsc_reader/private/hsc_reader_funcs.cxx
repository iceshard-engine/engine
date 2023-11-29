#include "hsc_reader_funcs.hxx"
#include "hsc_reader_app.hxx"
#include <ice/log.hxx>

namespace ishs = ice::hailstorm;

using ice::LogSeverity;
using ice::LogTag;

static constexpr ice::u32 Constant_HailstormHeaderVersion_0 = 'HSC0';

auto hailstorm_validate_header(ice::hailstorm::HailstormHeaderBase const& header) noexcept -> ice::i32
{
    if (header.magic != ishs::Constant_HailstormMagic)
    {
        ICE_LOG(LogSeverity::Error, LogTag_Main, "Invalid HailStorm header, unexpected 'MAGIC' value.");
        return 1;
    }
    if (header.header_version != ishs::Constant_HailstormHeaderVersion)
    {
        ICE_LOG(LogSeverity::Error, LogTag_Main, "Incompatible HailStorm header, unexpected 'VERSION' value.");
        return 2;
    }
    return 0;
}

void hailstorm_print_headerinfo(ice::hailstorm::v1::HailstormHeader const& header) noexcept
{
    if (header.header_version == Constant_HailstormHeaderVersion_0)
    {
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "Base-Header");
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- version: {:X}", Constant_HailstormHeaderVersion_0);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- size: {}", header.header_size.value);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "Pack-Header");
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- version: {}.{}.{}", header.version[0], header.version[1], header.version[2]);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- size-total: {}", header.offset_next.value);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- flag-encrypted: {}", header.is_encrypted);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- flag-expansion: {}", header.is_expansion);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- flag-baked: {}", header.is_baked);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- flag-patch: {}", header.is_patch);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- chunks: {}", header.count_chunks);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- resources: {}", header.count_resources);
        ICE_LOG(LogSeverity::Retail, LogTag_InfoHeader, "- paths: {} (total bytes)", (header.offset_data - header.header_size).value);
    }
}

void hailstorm_print_chunkinfo(ice::hailstorm::v1::HailstormData const& data, ParamRange range) noexcept
{

    if (data.header.header_version == Constant_HailstormHeaderVersion_0)
    {
        static constexpr ice::String Constant_TypeName[]{ "Invalid", "MetaData", "FileData", "Mixed" };
        static constexpr ice::String Constant_Persistance[]{ "Temporary", "Regular", "KeepIfPossible", "LoadAlways" };

        ice::u32 idx = range.start;
        for (ice::hailstorm::v1::HailstormChunk const& chunk : ice::span::subspan(data.chunks, range.start, range.count))
        {
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "Chunk ({})", idx++);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- offset: {}", chunk.offset.value);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- size: {}", chunk.size.value);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- size-origin: {}", chunk.size_origin.value);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- alignment: {}", ice::u32(chunk.align));
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- flag-encrypted: {} ({})", chunk.is_encrypted, chunk.is_encrypted ? "yes" : "no");
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- flag-compressed: {} ({})", chunk.is_compressed, chunk.is_compressed ? "yes" : "no");
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- type: {} ({})", chunk.type, Constant_TypeName[chunk.type]);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- persistance: {} ({})", chunk.persistance, Constant_Persistance[chunk.persistance]);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoChunks, "- count-entries: {}", chunk.count_entries);
        }
    }
}

void hailstorm_print_resourceinfo(ice::hailstorm::v1::HailstormData const& data, ParamRange range) noexcept
{
    if (data.header.header_version == Constant_HailstormHeaderVersion_0)
    {
        ice::u32 idx = range.start;
        for (ice::hailstorm::v1::HailstormResource const& res : ice::span::subspan(data.resources, range.start, range.count))
        {
            ICE_LOG(LogSeverity::Retail, LogTag_InfoResources, "Resource ({})", idx++);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoPaths, "- path: {}",
                ice::String{ ((char const*)data.paths_data.location) + res.path_offset, res.path_size });
            ICE_LOG(LogSeverity::Retail, LogTag_InfoResources, "- data-chunk: {}", res.chunk);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoResources, "- data-offset: {}", res.offset);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoResources, "- data-size: {}", res.size);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoResources, "- meta-chunk: {}", res.meta_chunk);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoResources, "- meta-offset: {}", res.meta_offset);
            ICE_LOG(LogSeverity::Retail, LogTag_InfoResources, "- meta-size: {}", res.meta_size);
        }
    }
}
