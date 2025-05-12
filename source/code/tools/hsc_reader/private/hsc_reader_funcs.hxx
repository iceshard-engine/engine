/// Copyright 2023 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <hailstorm/hailstorm.hxx>

auto hailstorm_validate_header(
    hailstorm::HailstormHeaderBase const& header
) noexcept -> ice::i32;

void hailstorm_print_headerinfo(
    hailstorm::HailstormHeader const& header
) noexcept;

void hailstorm_print_chunkinfo(
    hailstorm::HailstormData const& header
) noexcept;

void hailstorm_print_resourceinfo(
    hailstorm::HailstormData const& header
) noexcept;
