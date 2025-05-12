/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/task_types.hxx>
#include <ice/resource_types.hxx>
#include <ice/native_file.hxx>
#include <hailstorm/hailstorm.hxx>
#include <hailstorm/hailstorm_operations.hxx>

using ice::operator""_MiB;

struct HSCPWriteParams
{
    //! \brief File path for the generated file.
    //! \note As of now it needs to be an absoulte path.
    ice::native_file::FilePath filename;

    //! \brief The Async IO port to be used for async write request handling.
    ice::native_aio::AIOPort aioport;

    //! \brief Scheduler onto which read AIO tasks should be re-scheduled.
    ice::TaskScheduler& task_scheduler;

    //! \brief Size for each data chunk.
    ice::usize size_data_chunk = 16_MiB;

    //! \brief Size for each meta chunk.
    ice::usize size_meta_chunk = 2_MiB;

    //! \brief Hailstorm chunk selection logic.
    hailstorm::v1::HailstormWriteParams::ChunkSelectFn* fn_chunk_selector;
    hailstorm::v1::HailstormWriteParams::ChunkCreateFn* fn_chunk_create;

    //! \brief Userdata passed to selector function.
    void* ud_chunk_selector;
};

bool hscp_write_hailstorm_file(
    ice::Allocator& alloc,
    HSCPWriteParams const& params,
    hailstorm::v1::HailstormWriteData const& data,
    ice::ResourceTracker& tracker,
    ice::Span<ice::ResourceHandle> resources
) noexcept;
