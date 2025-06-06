/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/span.hxx>
#include <ice/expected.hxx>
#include <ice/stringid.hxx>
#include <ice/mem_data.hxx>
#include <ice/mem_unique_ptr.hxx>
#include <ice/resource_types.hxx>
#include <ice/task.hxx>
#include <ice/task_expected.hxx>
#include <ice/task_flags.hxx>

namespace ice
{

    class AssetRequest;
    class AssetShelve;
    class AssetStorage;
    class AssetCategoryArchive;

    struct AssetHandle;
    struct AssetCategoryDefinition;
    struct AssetStateTrackers;
    struct AssetStateTransaction;

    enum class AssetRequestResult : ice::u8;
    enum class AssetState : ice::u8;

} // namespace ice
