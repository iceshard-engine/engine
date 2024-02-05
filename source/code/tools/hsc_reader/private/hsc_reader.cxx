/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/tool_app.hxx>
#include <ice/log_module.hxx>
#include <ice/log.hxx>

#include <hailstorm/hailstorm_operations.hxx>

#include "hsc_reader_app.hxx"
#include "hsc_reader_funcs.hxx"

struct HSReaderLog : ice::Module<ice::LogModule>
{
    IS_WORKAROUND_MODULE_INITIALIZATION(HSReaderLog);
};

class HailStormReaderApp final : public ice::tool::ToolApp<HailStormReaderApp>
{
public:
    HailStormReaderApp() noexcept
        : ToolApp<HailStormReaderApp>{}
        , _file_path{ _allocator }
        , _file{}
        , _data{}
    { }

    void setup(ice::ParamList& params) noexcept override
    {
        hscr_initialize_logging(params);
    }

    auto run(ice::ParamList const& params) noexcept -> ice::i32 override
    {
        ice::String packfile;
        if (ice::params::find_first(params, Param_File, packfile) == false)
        {
            ICE_LOG(ice::LogSeverity::Retail, LogTag_Main, "Use '-f' to provide an input file.");
            return 1;
        }

        // Open the pack file
        ice::native_file::path_from_string(packfile, _file_path);
        if (packfile_validate(params) == false)
        {
            ICE_LOG(ice::LogSeverity::Retail, LogTag_Main, "Provide input file is not a valid Hailstorm pack.");
            return 1;
        }

        bool const print_chunks = ice::params::has_any(params, Param_InfoChunks);
        bool const print_resources = ice::params::has_any(params, Param_InfoResources);
        if (print_chunks || print_resources)
        {
            packfile_print_info(params);
        }
        return 0;
    }

private:
    bool packfile_validate(ice::ParamList const& params) noexcept
    {
        using namespace hailstorm;

        _file_path = ice::tool::path_make_absolute(_file_path);
        if (_file = ice::native_file::open_file(_file_path); _file)
        {
            ice::usize const bytes_read = ice::native_file::read_file(
                _file,
                ice::size_of<HailstormHeader>,
                { &_data.header, ice::size_of<HailstormHeader>, ice::align_of<HailstormHeader> }
            );

            if (hailstorm_validate_header(_data.header) == 0)
            {
                hailstorm_print_headerinfo(params, _data.header);
                return true;
            }
        }
        return false;
    }

    void packfile_print_info(ice::ParamList const& params) noexcept
    {
        using ice::operator""_B;

        ice::Memory header_mem = _allocator.allocate(ice::usize{ _data.header.offset_data });
        ice::usize const bytes_read = ice::native_file::read_file(
            _file, 0_B, header_mem.size, header_mem
        );

        if (bytes_read < header_mem.size)
        {
            return;
        }

        if (hailstorm::v1::read_header({ header_mem.location, header_mem.size.value, (size_t)header_mem.alignment }, _data) == hailstorm::Result::Success)
        {
            ParamRange range;
            if (ice::params::find_first(params, Param_InfoChunks, range))
            {
                hailstorm_print_chunkinfo(params, _data, range);
            }

            if (ice::params::find_first(params, Param_InfoResources, range))
            {
                hailstorm_print_resourceinfo(_data, range);
            }
        }

        _allocator.deallocate(header_mem);
    }

private:
    ice::native_file::HeapFilePath _file_path;
    ice::native_file::File _file;
    hailstorm::HailstormData _data;
};
