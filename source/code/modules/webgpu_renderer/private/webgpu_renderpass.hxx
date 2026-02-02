/// Copyright 2024 - 2026, Dandielo <dandielo@iceshard.net>
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
            attachments.push_back(info.attachments);
            subpasses.reserve(info.subpasses.size());

            ice::ncount num_references = 0;
            for (ice::render::RenderSubPass const& subpass : info.subpasses)
            {
                num_references += subpass.input_attachments.size();
                num_references += subpass.color_attachments.size();
            }
            references.reserve(num_references);

            for (ice::render::RenderSubPass const& subpass : info.subpasses)
            {
                ice::ncount const offset = references.size();
                references.push_back(subpass.input_attachments);
                references.push_back(subpass.color_attachments);

                subpasses.push_back(
                    RenderSubPass {
                        .input_attachments = references.subspan(offset, subpass.input_attachments.size()),
                        .color_attachments = references.subspan(offset + subpass.input_attachments.size(), subpass.color_attachments.size()),
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
