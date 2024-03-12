/// Copyright 2023 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/param_list.hxx>
#include <hailstorm/hailstorm.hxx>

auto hailstorm_validate_header(
    hailstorm::HailstormHeaderBase const& header
) noexcept -> ice::i32;

void hailstorm_print_headerinfo(
    ice::ParamList const& params,
    hailstorm::HailstormHeader const& header
) noexcept;

void hailstorm_print_chunkinfo(
    ice::ParamList const& params,
    hailstorm::HailstormData const& header,
    ParamRange range
) noexcept;

void hailstorm_print_resourceinfo(
    hailstorm::HailstormData const& header,
    ParamRange range
) noexcept;
