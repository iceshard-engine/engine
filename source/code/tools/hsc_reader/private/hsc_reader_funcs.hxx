#pragma once
#include <ice/resource_hailstorm.hxx>

auto hailstorm_validate_header(ice::hailstorm::HailstormHeaderBase const& header) noexcept -> ice::i32;
void hailstorm_print_headerinfo(ice::hailstorm::v1::HailstormHeader const& header) noexcept;
void hailstorm_print_chunkinfo(ice::hailstorm::v1::HailstormData const& header, ParamRange range) noexcept;
void hailstorm_print_resourceinfo(ice::hailstorm::v1::HailstormData const& header, ParamRange range) noexcept;
