#include "image_loader.hxx"
#include <ice/render/render_image.hxx>
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice
{

    //auto IceshardImageLoader::load(
    //    ice::AssetType type,
    //    ice::Data data,
    //    ice::Allocator& alloc,
    //    ice::Memory& out_data
    //) const noexcept -> ice::AssetStatus
    //{
    //    using ice::render::ImageInfo;

    //    out_data.size = sizeof(ImageInfo);
    //    out_data.alignment = alignof(ImageInfo);
    //    out_data.location = alloc.allocate(out_data.size, out_data.alignment);

    //    ImageInfo const& image_data = *reinterpret_cast<ImageInfo const*>(data.location);
    //    ImageInfo* image = reinterpret_cast<ImageInfo*>(out_data.location);
    //    image->type = image_data.type;
    //    image->usage = image_data.usage;
    //    image->format = image_data.format;
    //    image->width = image_data.width;
    //    image->height = image_data.height;
    //    image->data = ice::memory::ptr_add(
    //        data.location,
    //        static_cast<ice::u32>(reinterpret_cast<ice::uptr>(image_data.data))
    //    );

    //    return ice::AssetStatus::Loaded;
    //}

} // namespace ice
