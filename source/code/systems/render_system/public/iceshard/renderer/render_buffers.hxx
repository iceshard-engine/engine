#pragma once
#include <core/pod/collections.hxx>
#include <iceshard/renderer/render_api.hxx>

namespace iceshard::renderer
{

    auto create_buffer(
        api::BufferType type,
        uint32_t size
    ) noexcept -> api::Buffer;

    void map_buffer(
        api::Buffer buffer,
        api::DataView& view
    ) noexcept;

    void unmap_buffer(
        api::Buffer buffer
    ) noexcept;

    void map_buffers(
        core::pod::Array<api::Buffer>& buffers,
        core::pod::Array<api::DataView>& views
    ) noexcept;

    void unmap_buffers(
        core::pod::Array<api::Buffer>& buffers
    ) noexcept;

} // namespace iceshard::renderer
