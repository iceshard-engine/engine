#include "mesh_loader.hxx"
#include <ice/gfx/gfx_model.hxx>
#include <ice/memory/pointer_arithmetic.hxx>

namespace ice
{

    auto IceshardMeshLoader::load(
        ice::AssetType type,
        ice::Data data,
        ice::Allocator& alloc,
        ice::Memory& out_data
    ) noexcept -> ice::AssetStatus
    {
        using ice::gfx::Model;
        using ice::gfx::Mesh;
        using ice::gfx::Vertice;

        out_data.size = sizeof(Model);
        out_data.alignment = alignof(Model);
        out_data.location = alloc.allocate(out_data.size, out_data.alignment);

        Model model_data = *reinterpret_cast<Model const*>(data.location);
        void const* mesh_ptr = ice::memory::ptr_add(
            data.location,
            static_cast<ice::u32>(reinterpret_cast<ice::uptr>(model_data.mesh_list))
        );
        void const* vertice_ptr = ice::memory::ptr_add(
            data.location,
            static_cast<ice::u32>(reinterpret_cast<ice::uptr>(model_data.vertice_data))
        );
        void const* indice_ptr = ice::memory::ptr_add(
            data.location,
            static_cast<ice::u32>(reinterpret_cast<ice::uptr>(model_data.indice_data))
        );

        Model* model = reinterpret_cast<Model*>(out_data.location);
        model->mesh_count = model_data.mesh_count;
        model->vertice_data_size = model_data.vertice_data_size;
        model->indice_data_size = model_data.indice_data_size;
        model->mesh_list = reinterpret_cast<Mesh const*>(mesh_ptr);
        model->vertice_data = reinterpret_cast<Vertice const*>(vertice_ptr);
        model->indice_data = reinterpret_cast<ice::u16 const*>(indice_ptr);

        return AssetStatus::Loaded;
    }

} // namespace ice
