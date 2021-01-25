#include "image_loader.hxx"
#include <ice/render/render_image.hxx>
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice
{

    auto IceshardImageLoader::load(
        ice::AssetType type,
        ice::Data data,
        ice::Allocator& alloc,
        ice::Memory& out_data
    ) noexcept -> ice::AssetStatus
    {
        using ice::render::Image;

        out_data.size = sizeof(Image);
        out_data.alignment = alignof(Image);
        out_data.location = alloc.allocate(out_data.size, out_data.alignment);

        Image image_data = *reinterpret_cast<Image const*>(data.location);
        Image* image = reinterpret_cast<Image*>(out_data.location);
        image->width = image_data.width;
        image->height = image_data.height;
        image->data = ice::memory::ptr_add(
            data.location,
            static_cast<ice::u32>(reinterpret_cast<ice::uptr>(image_data.data))
        );

        return ice::AssetStatus::Loaded;
    }

} // namespace ice
