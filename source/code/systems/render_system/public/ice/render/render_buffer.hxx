/// Copyright 2022 - 2026, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/base.hxx>
#include <ice/mem_relative_ptr.hxx>
#include <ice/render/render_assets.hxx>

namespace ice::render
{

    enum class Buffer : ice::uptr
    {
        Invalid = 0x0
    };

    enum class BufferType : ice::u32
    {
        Uniform,
        Vertex,
        Index,
        Transfer,
    };

    struct BufferUpdateInfo
    {
        ice::render::Buffer buffer;
        ice::Data data;
        ice::usize offset;
    };

    struct Mesh
    {
        ice::ncount vertex_count; // holds count and stride
        ice::ncount index_count; // holds count and stride
        ice::render::Buffer vertex_buffer;
        ice::render::Buffer index_buffer;
    };

    struct MeshInfo
    {
        ice::ncount vertex_count; // holds count and stride
        ice::ncount index_count; // holds count and stride
        ice::relative_ptr<void const> data;
    };

} // namespace ice::render
