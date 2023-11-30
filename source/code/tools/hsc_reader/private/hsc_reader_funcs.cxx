#include "hsc_reader_funcs.hxx"
#include "hsc_reader_app.hxx"
#include <ice/log.hxx>

namespace ishs = ice::hailstorm;

using ice::LogSeverity;
using ice::LogTag;

static constexpr ice::u32 Constant_HailstormHeaderVersion_0 = 'HSC0';

static constexpr ice::String Constant_YesNo[]{ "no", "yes" };

#define HSCR_INFO(tag, format, ...) ICE_LOG(LogSeverity::Retail, tag, format, __VA_ARGS__)

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

void hailstorm_print_headerinfo(
    ice::ParamList const& params,
    ice::hailstorm::v1::HailstormHeader const& header
) noexcept
{
    if (header.header_version == Constant_HailstormHeaderVersion_0)
    {
        HSCR_INFO(LogTag_InfoHeader, "Base-Header");
        HSCR_INFO(LogTag_InfoHeader, "- version: {:X}", Constant_HailstormHeaderVersion_0);
        HSCR_INFO(LogTag_InfoHeader, "- size: {:p}", header.header_size);
        HSCR_INFO(LogTag_InfoHeader, "Pack-Header");
        HSCR_INFO(LogTag_InfoHeader, "- version: {}.{}.{}", header.version[0], header.version[1], header.version[2]);
        HSCR_INFO(LogTag_InfoHeader, "- size-total: {:p}", header.offset_next);
        HSCR_INFO(LogTag_InfoHeader, "- flag-encrypted: {}", Constant_YesNo[header.is_encrypted]);
        HSCR_INFO(LogTag_InfoHeader, "- flag-expansion: {}", Constant_YesNo[header.is_expansion]);
        HSCR_INFO(LogTag_InfoHeader, "- flag-baked: {}", Constant_YesNo[header.is_baked]);
        HSCR_INFO(LogTag_InfoHeader, "- flag-patch: {}", Constant_YesNo[header.is_patch]);
        HSCR_INFO(LogTag_InfoHeader, "- chunks: {}", header.count_chunks);
        HSCR_INFO(LogTag_InfoHeader, "- resources: {}", header.count_resources);
        HSCR_INFO(LogTag_InfoHeader, "- paths: {:p}", (header.offset_data - header.header_size).to_usize());
        if (ice::params::has_any(params, Param_InfoCustomValues))
        {
            HSCR_INFO(LogTag_InfoHeader, "- app-custom-0: {:X}", header.app_custom_values[0]);
            HSCR_INFO(LogTag_InfoHeader, "- app-custom-1: {:X}", header.app_custom_values[1]);
            HSCR_INFO(LogTag_InfoHeader, "- app-custom-2: {:X}", header.app_custom_values[2]);
        }
    }
}

void hailstorm_print_chunkinfo(
    ice::ParamList const& params,
    ice::hailstorm::v1::HailstormData const& data,
    ParamRange range
) noexcept
{
    if (data.header.header_version == Constant_HailstormHeaderVersion_0)
    {
        static constexpr ice::String Constant_TypeName[]{ "Invalid", "MetaData", "FileData", "Mixed" };
        static constexpr ice::String Constant_Persistance[]{ "Temporary", "Regular", "KeepIfPossible", "LoadAlways" };

        ice::u32 idx = range.start;
        for (ice::hailstorm::v1::HailstormChunk const& chunk : ice::span::subspan(data.chunks, range.start, range.count))
        {
            HSCR_INFO(LogTag_InfoChunks, "Chunk ({})", idx++);
            HSCR_INFO(LogTag_InfoChunks, "- offset: {:i}", chunk.offset);
            HSCR_INFO(LogTag_InfoChunks, "- size: {:p}", chunk.size);
            HSCR_INFO(LogTag_InfoChunks, "- size-origin: {:p}", chunk.size_origin);
            HSCR_INFO(LogTag_InfoChunks, "- alignment: {}", ice::u32(chunk.align));
            HSCR_INFO(LogTag_InfoChunks, "- flag-encrypted: {}", Constant_YesNo[chunk.is_encrypted]);
            HSCR_INFO(LogTag_InfoChunks, "- flag-compressed: {}", Constant_YesNo[chunk.is_compressed]);
            HSCR_INFO(LogTag_InfoChunks, "- type: {} ({})", chunk.type, Constant_TypeName[chunk.type]);
            HSCR_INFO(LogTag_InfoChunks, "- persistance: {} ({})", chunk.persistance, Constant_Persistance[chunk.persistance]);
            HSCR_INFO(LogTag_InfoChunks, "- count-entries: {}", chunk.count_entries);
            if (ice::params::has_any(params, Param_InfoCustomValues))
            {
                HSCR_INFO(LogTag_InfoChunks, "- app-custom: {}", chunk.app_custom_value);
            }
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
            ice::String res_path{ ((char const*)data.paths_data.location) + res.path_offset, res.path_size };

            HSCR_INFO(LogTag_InfoResources, "Resource ({})", idx++);
            HSCR_INFO(LogTag_InfoPaths, "- path: {}", res_path);
            HSCR_INFO(LogTag_InfoResources, "- data-chunk: {}", res.chunk);
            HSCR_INFO(LogTag_InfoResources, "- data-offset: {}", res.offset);
            HSCR_INFO(LogTag_InfoResources, "- data-size: {:p}", ice::usize{ res.size });
            HSCR_INFO(LogTag_InfoResources, "- meta-chunk: {}", res.meta_chunk);
            HSCR_INFO(LogTag_InfoResources, "- meta-offset: {}", res.meta_offset);
            HSCR_INFO(LogTag_InfoResources, "- meta-size: {:p}", ice::usize{ res.meta_size });
        }
    }
}
