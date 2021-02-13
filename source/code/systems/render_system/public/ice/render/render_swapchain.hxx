#pragma once
#include <ice/math.hxx>
#include <ice/render/render_image.hxx>

namespace ice::render
{

    class RenderSwapchain
    {
    protected:
        virtual ~RenderSwapchain() noexcept = default;

    public:
        virtual auto extent() const noexcept -> ice::vec2u = 0;

        virtual auto image_format() const noexcept -> ice::render::ImageFormat = 0;

        virtual auto image_count() const noexcept -> ice::u32 = 0;

        virtual auto image(ice::u32 index) const noexcept -> ice::render::Image = 0;

        virtual auto aquire_image() noexcept -> ice::u32 = 0;
    };

} // namespace ice::render
