/// Copyright 2023 - 2023, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/resource_hailstorm.hxx>
#include <ice/resource_hailstorm_operations.hxx>
#include <ice/native_file.hxx>

using ice::operator""_MiB;

struct HSCPWriteParams
{
    //! \brief File path for the generated file.
    //! \note As of now it needs to be an absoulte path.
    ice::native_file::FilePath filename;

    //! \brief Scheduler onto which AIO tasks should be scheduled.
    ice::TaskScheduler& task_scheduler;

    //! \brief Size for each data chunk.
    ice::usize size_data_chunk = 16_MiB;

    //! \brief Size for each meta chunk.
    ice::usize size_meta_chunk = 2_MiB;

    //! \brief Hailstorm chunk selection logic.
    ice::hailstorm::v1::HailstormWriteParams::ChunkSelectFn* fn_chunk_selector;
    ice::hailstorm::v1::HailstormWriteParams::ChunkCreateFn* fn_chunk_create;

    //! \brief Userdata passed to selector function.
    void* ud_chunk_selector;
};

bool hscp_write_hailstorm_file(
    ice::Allocator& alloc,
    HSCPWriteParams const& params,
    ice::hailstorm::v1::HailstormWriteData const& data,
    ice::ResourceTracker& tracker,
    ice::Span<ice::ResourceHandle*> resources
) noexcept;
