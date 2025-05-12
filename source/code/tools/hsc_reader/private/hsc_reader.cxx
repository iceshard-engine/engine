/// Copyright 2022 - 2025, Dandielo <dandielo@iceshard.net>
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
        , _param_packfile{ }
        , _file_path{ _allocator }
        , _file{}
        , _data{}
    { }

    bool setup(ice::Params& params) noexcept override
    {
        ice::params_register_globals(params);
        ice::params_define(
            params, {
                .name = "--pack,packfile,-f,--file",
                .description = "Pack file to be read",
                .flags = ice::ParamFlags::IsRequired | ice::ParamFlags::ValidateFile,
            },
            _param_packfile
        );
        return true;
    }

    auto run() noexcept -> ice::i32 override
    {
        hscr_initialize_logging();

        // Open the pack file
        if (packfile_validate() == false)
        {
            ICE_LOG(ice::LogSeverity::Retail, LogTag_Main, "Provide input file is not a valid Hailstorm pack.");
            return 1;
        }

        if (Param_ShowChunks.value.set || Param_ShowResources.value.set)
        {
            packfile_print_info();
        }
        return 0;
    }

private:
    bool packfile_validate() noexcept
    {
        using namespace hailstorm;

        ice::native_file::path_from_string(_file_path, _param_packfile);
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
                hailstorm_print_headerinfo(_data.header);
                return true;
            }
        }
        return false;
    }

    void packfile_print_info() noexcept
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
            if (Param_ShowChunks.value.set)
            {
                hailstorm_print_chunkinfo(_data);
            }

            if (Param_ShowResources.value.set)
            {
                hailstorm_print_resourceinfo(_data);
            }
        }

        _allocator.deallocate(header_mem);
    }

public: // Tool information
    auto name() const noexcept -> ice::String override { return "hsc_reader"; }
    auto version() const noexcept -> ice::String override { return "0.1.0"; }
    auto description() const noexcept -> ice::String override
    {
        return "Prints hailstorm pack information to the standard output. The printend information can be controlled using various options.";
    }

private:
    ice::String _param_packfile;
    ice::native_file::HeapFilePath _file_path;
    ice::native_file::File _file;
    hailstorm::HailstormData _data;
};
