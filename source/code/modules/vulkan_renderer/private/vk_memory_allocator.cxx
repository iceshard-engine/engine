/// Copyright 2022 - 2024, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#include <ice/base.hxx>

#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1

#if ISP_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable: 4189) //: local variable is initialized but not referenced
#pragma warning(disable: 4324) // {}: structure was padded due to alignment specifier
#elif ISP_COMPILER_CLANG || ISP_COMPILER_GCC
#endif

#include <vk_mem_alloc.h> // Vulkan Memory Allocator
#undef assert

#if ISP_COMPILER_MSVC
#pragma warning(pop)
#elif ISP_COMPILER_CLANG || ISP_COMPILER_GCC
#endif
