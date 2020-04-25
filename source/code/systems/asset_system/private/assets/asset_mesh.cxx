#include <asset_system/assets/asset_mesh.hxx>
#include <core/pod/array.hxx>
#include <core/memory.hxx>

#include <rapidjson/document.h>
#include <unordered_map>

namespace asset::detail
{

    class AssetMeshResolver : public asset::AssetResolver
    {
    public:
        auto resolve_asset_type(core::StringView extension, resource::ResourceMetaView const& meta) noexcept -> AssetType override
        {
            AssetType result = AssetType::Unresolved;
            if (core::string::equals(extension, ".ismesh"))
            {
                result = AssetType::Mesh;
            }
            else
            {
                int32_t meta_type = 0;
                if (resource::get_meta_int32(meta, "resource.meta.type"_sid, meta_type) && meta_type == 0)
                {
                    // The only supported format
                    core::StringView mesh_format;
                    if (resource::get_meta_string(meta, "mesh.format"_sid, mesh_format); mesh_format == "json")
                    {
                        result = AssetType::Mesh;
                    }
                }
            }
            return result;
        }
    };

    class AssetMeshLoader : public asset::AssetLoader
    {
        struct MeshVertice
        {
            float pos[4];
            float color[4];
        };

    public:
        AssetMeshLoader(core::allocator& alloc) noexcept
            : _allocator{ alloc }
        {

        }

        auto supported_asset_types() const noexcept -> core::pod::Array<asset::AssetType> const& override
        {
            core::pod::Array<asset::AssetType> empty_view{ core::memory::globals::null_allocator() };
            return empty_view;
        }

        auto request_asset(asset::Asset) noexcept -> asset::AssetStatus override
        {
            return asset::AssetStatus::Loaded; // Initially everything is loaded
        }

        auto load_asset(
            asset::Asset asset,
            resource::ResourceMetaView meta,
            core::data_view resource_data,
            asset::AssetData& result_data
        ) noexcept -> asset::AssetStatus override
        {
            // We can assume every mesh is currently in 'json' format
            rapidjson::Document doc;
            doc.Parse<rapidjson::ParseFlag::kParseCommentsFlag>(reinterpret_cast<char const*>(resource_data._data), resource_data._size);

            auto const& doc_obj = doc.GetObject();

            IS_ASSERT(doc_obj.HasMember("vertice_count"), "Invalid mesh format!");
            IS_ASSERT(doc_obj.HasMember("vertices"), "Invalid mesh format!");

            IS_ASSERT(doc_obj["vertice_count"].IsInt(), "Invalid mesh format!");
            IS_ASSERT(doc_obj["vertices"].IsArray(), "Invalid mesh format!");

            std::vector<MeshVertice> loaded_vertices;

            //int32_t vertice_count = doc["vertice_count"].GetInt();
            for (auto const& vertice_data : doc_obj["vertices"].GetArray())
            {
                IS_ASSERT(vertice_data.IsObject(), "Invalid mesh format!");
                IS_ASSERT(vertice_data.HasMember("pos"), "Invalid mesh format!");
                IS_ASSERT(vertice_data.HasMember("color"), "Invalid mesh format!");
                IS_ASSERT(vertice_data["pos"].IsArray(), "Invalid mesh format!");
                IS_ASSERT(vertice_data["color"].IsArray(), "Invalid mesh format!");

                auto const& pos_data = vertice_data["pos"].GetArray();
                auto const& color_data = vertice_data["color"].GetArray();

                loaded_vertices.emplace_back(MeshVertice{
                    .pos = { pos_data[0].GetFloat(), pos_data[1].GetFloat(), pos_data[2].GetFloat(), 1.0f },
                    .color = { color_data[0].GetFloat(), color_data[1].GetFloat(), color_data[2].GetFloat(), 1.0f }
                });
            }

            _loaded_meshes.emplace(asset.name.hash_value, std::move(loaded_vertices));
            result_data.content = { _loaded_meshes[asset.name.hash_value].data(), (uint32_t) _loaded_meshes[asset.name.hash_value].size() * sizeof(MeshVertice) };
            result_data.metadata = meta;

            return asset::AssetStatus::Loaded;
        }

        void release_asset(asset::Asset) noexcept override
        {
        }

    private:
        core::allocator& _allocator;

        std::unordered_map<core::cexpr::stringid_hash_type, std::vector<MeshVertice>> _loaded_meshes;
    };

}

auto asset::default_resolver_mesh(core::allocator& alloc) noexcept -> core::memory::unique_pointer<asset::AssetResolver>
{
    return core::memory::make_unique<asset::AssetResolver, asset::detail::AssetMeshResolver>(alloc);
}

auto asset::default_loader_mesh(core::allocator& alloc) noexcept -> core::memory::unique_pointer<asset::AssetLoader>
{
    return core::memory::make_unique<asset::AssetLoader, asset::detail::AssetMeshLoader>(alloc, alloc);
}
