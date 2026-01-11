/// Copyright 2024 - 2025, Dandielo <dandielo@iceshard.net>
/// SPDX-License-Identifier: MIT

#pragma once
#include <ice/render/render_pass.hxx>
#include <ice/array.hxx>
#include "webgpu_utils.hxx"
#include "webgpu_image.hxx"

namespace ice::render::webgpu
{

    struct WebGPURenderPass
    {
        ice::Array<ice::render::RenderAttachment> attachments;
        ice::Array<ice::render::AttachmentReference> references;
        ice::Array<ice::render::RenderSubPass> subpasses;

        WebGPURenderPass(
            ice::Allocator& alloc,
            ice::render::RenderpassInfo const& info
        ) noexcept
            : attachments{ alloc }
            , references{ alloc }
            , subpasses{ alloc }
        {
            ice::array::push_back(attachments, info.attachments);
            ice::array::reserve(subpasses, ice::count(info.subpasses));

            ice::ucount num_references = 0;
            for (ice::render::RenderSubPass const& subpass : info.subpasses)
            {
                num_references += ice::count(subpass.input_attachments);
                num_references += ice::count(subpass.color_attachments);
            }
            ice::array::reserve(references, num_references);

            for (ice::render::RenderSubPass const& subpass : info.subpasses)
            {
                ice::ucount const offset = ice::array::count(references);
                ice::array::push_back(references, subpass.input_attachments);
                ice::array::push_back(references, subpass.color_attachments);

                ice::array::push_back(subpasses,
                    RenderSubPass {
                        .input_attachments = ice::array::slice(references, offset, ice::count(subpass.input_attachments)),
                        .color_attachments = ice::array::slice(references, offset + ice::count(subpass.input_attachments), ice::count(subpass.color_attachments)),
                        .depth_stencil_attachment = subpass.depth_stencil_attachment
                    }
                );
            }
        }

        static auto handle(WebGPURenderPass* native) noexcept
        {
            return static_cast<ice::render::Renderpass>(reinterpret_cast<uintptr_t>(native));
        }

        static auto native(ice::render::Renderpass handle) noexcept
        {
            return reinterpret_cast<WebGPURenderPass*>(static_cast<uintptr_t>(handle));
        }
    };

} // namespace ice::render::webgpu
