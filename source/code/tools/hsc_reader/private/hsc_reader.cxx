/// Copyright 2022 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT


#include <ice/container/array.hxx>
#include <ice/log.hxx>
#include <ice/log_module.hxx>
#include <ice/mem.hxx>
#include <ice/mem_allocator_host.hxx>
#include <ice/module.hxx>
#include <ice/module_register.hxx>
#include <ice/os/windows.hxx>
#include <ice/param_list.hxx>
#include <ice/resource_hailstorm.hxx>
#include <ice/resource_hailstorm_operations.hxx>

#include "hsc_reader_app.hxx"
#include "hsc_reader_funcs.hxx"

int main(int argc, char** argv)
{
    ice::HostAllocator alloc;
    ice::ParamList params{ alloc, { argv, ice::u32(argc) } };
    HSCReaderApp app{ alloc, params };

    ice::String packfile;
    if (ice::params::find_first(params, Param_File, packfile) == false)
    {
        return 1;
    }

    bool print_chunks = ice::params::has_any(params, Param_InfoChunks);
    bool print_resources = ice::params::has_any(params, Param_InfoResources);

    HANDLE const file = CreateFileA(
        packfile._data,
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    int result = 0;
    if (file != INVALID_HANDLE_VALUE)
    {
        ice::hailstorm::HailstormData data;

        BOOL r = ReadFile(file, &data.header, sizeof(data.header), NULL, NULL);
        ICE_ASSERT_CORE(r != FALSE);

        if (result = hailstorm_validate_header(data.header); result == 0)
        {
            hailstorm_print_headerinfo(data.header);

            if (print_chunks || print_resources)
            {
                ice::Memory header_mem = alloc.allocate(data.header.offset_data);

                OVERLAPPED ov{};
                r = ReadFile(file, header_mem.location, ice::u32(header_mem.size.value), NULL, &ov);
                ICE_ASSERT_CORE(r != FALSE);

                if (ice::hailstorm::v1::read_header(ice::data_view(header_mem), data) == ice::Res::Success)
                {
                    ParamRange range;
                    if (ice::params::find_first(params, Param_InfoChunks, range))
                    {
                        hailstorm_print_chunkinfo(data, range);
                    }

                    if (ice::params::find_first(params, Param_InfoResources, range))
                    {
                        hailstorm_print_resourceinfo(data, range);
                    }
                }
                else
                {
                    result = 3;
                }

                alloc.deallocate(header_mem);
            }
        }

        CloseHandle(file);
    }

    return result;
}
