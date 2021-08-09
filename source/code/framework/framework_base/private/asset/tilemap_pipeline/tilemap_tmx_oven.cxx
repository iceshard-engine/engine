#pragma once
#include "tilemap_tmx_oven.hxx"
#include "tilemap_pipeline.hxx"

#include <ice/game_tilemap.hxx>
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_system.hxx>

#include <ice/asset_system.hxx>

#include <ice/stack_string.hxx>
#include <ice/log.hxx>

#include <rapidxml/rapidxml.hpp>
#undef assert

#define TILED_LOG(severity, format, ...) \
    ICE_LOG(severity, LogTag_TiledOven, format, ##__VA_ARGS__)

namespace ice
{

    struct TileMapInfo
    {
        ice::vec2f map_size;
        ice::vec2f tile_size;

        ice::u32 layer_count;
        ice::u32 tile_count;

        ice::u32 tileset_count;
        ice::u16 tileset_gids[16];
        ice::u16 tileset_columns[16];
        ice::String tileset_image[16];

        ice::TileMap* tilemap;
        ice::TileSet* tilesets;
        ice::TileLayer* layers;
    };

    auto node_name(rapidxml::xml_node<> const& node) noexcept
    {
        return ice::String{ node.name(), node.name_size() };
    }

    template<typename T>
    bool parse_attrib_value(rapidxml::xml_attribute<> const* attrib, T& out) noexcept = delete;

    template<>
    bool parse_attrib_value<ice::String>(rapidxml::xml_attribute<> const* attrib, ice::String& out_value) noexcept
    {
        if (attrib != nullptr)
        {
            out_value = ice::String{ attrib->value(), attrib->value_size() };
            return true;
        }
        return false;
    }

    template<>
    bool parse_attrib_value<ice::u32>(rapidxml::xml_attribute<> const* attrib, ice::u32& out_value) noexcept
    {
        if (attrib != nullptr)
        {
            char const* const val_beg = attrib->value();
            char const* const val_end = val_beg + attrib->value_size();

            return std::from_chars(val_beg, val_end, out_value).ptr == val_end;
        }
        return false;
    }

    template<>
    bool parse_attrib_value<ice::f32>(rapidxml::xml_attribute<> const* attrib, ice::f32& out_value) noexcept
    {
        if (attrib != nullptr)
        {
            char const* const val_beg = attrib->value();
            char const* const val_end = val_beg + attrib->value_size();

            return std::from_chars(val_beg, val_end, out_value).ptr == val_end;
        }
        return false;
    }

    template<>
    bool parse_attrib_value<ice::f64>(rapidxml::xml_attribute<> const* attrib, ice::f64& out_value) noexcept
    {
        if (attrib != nullptr)
        {
            char const* const val_beg = attrib->value();
            char const* const val_end = val_beg + attrib->value_size();

            return std::from_chars(val_beg, val_end, out_value).ptr == val_end;
        }
        return false;
    }

    void parse_map(
        rapidxml::xml_node<> const& map_node,
        ice::TileMapInfo& out_tilemap,
        bool& out_infinite
    ) noexcept
    {
        rapidxml::xml_attribute<> const* attrib = map_node.first_attribute("version");
        if (attrib == nullptr)
        {
            return;
        }

        ice::String tiled_map_ver;
        parse_attrib_value(attrib, tiled_map_ver);

        TILED_LOG(LogSeverity::Info, "Loading Tiled Map (ver: {})", tiled_map_ver);
        // width = 20
        // height = 20
        // tilewidth = 16
        // tileheight = 16
        // infinite = 0

        attrib = attrib->next_attribute("width");
        parse_attrib_value(attrib, out_tilemap.map_size.x);
        attrib = attrib->next_attribute("height");
        parse_attrib_value(attrib, out_tilemap.map_size.y);

        attrib = attrib->next_attribute("tilewidth");
        parse_attrib_value(attrib, out_tilemap.tile_size.x);
        attrib = attrib->next_attribute("tileheight");
        parse_attrib_value(attrib, out_tilemap.tile_size.y);

        attrib = attrib->next_attribute("infinite");

        ice::u32 infinite_val;
        if (parse_attrib_value(attrib, infinite_val))
        {
            out_infinite = infinite_val == 1;
        }
    }

    void parse_tileset(
        rapidxml::xml_node<> const& tileset_node,
        ice::TileSet& tileset
    ) noexcept
    {
        rapidxml::xml_attribute<> const* attrib = tileset_node.first_attribute("name");

        ice::String name = "<unnamed>";
        parse_attrib_value(attrib, name);
        TILED_LOG(LogSeverity::Debug, "Parsing tileset: {}", name);

        attrib = attrib->next_attribute("tilewidth");
        parse_attrib_value(attrib, tileset.element_size.x);
        attrib = attrib->next_attribute("tileheight");
        parse_attrib_value(attrib, tileset.element_size.y);
    }

    void parse_layer(
        rapidxml::xml_node<> const& tileset_node,
        ice::TileMapInfo const& tilemap_info,
        ice::TileLayer& layer,
        ice::Tile*& tiles,
        ice::u32& tile_offset
    ) noexcept
    {
        layer.tile_offset = tile_offset;
        layer.tile_count = 0;


        rapidxml::xml_attribute<> const* attrib = tileset_node.first_attribute("id");

        ice::u32 layer_id = 0;
        parse_attrib_value(attrib, layer_id);
        layer.name = ice::StringID{ ice::StringID_Hash{ layer_id } };

        ice::String name = "<unnamed>";
        parse_attrib_value(attrib->next_attribute("name"), name);
        TILED_LOG(LogSeverity::Debug, "Parsing layer: {}", name);

        ice::u32 layer_width = 0;
        parse_attrib_value(attrib->next_attribute("width"), layer_width);
        ice::u32 layer_height = 0;
        parse_attrib_value(attrib->next_attribute("height"), layer_height);

        ice::u32 const layer_tile_count = layer_width * layer_height;

        rapidxml::xml_node<> const* data_node = tileset_node.first_node("data");

        if (data_node != nullptr)
        {
            ice::String encoding;
            parse_attrib_value(data_node->first_attribute("encoding"), encoding);

            if (encoding == "csv")
            {
                ice::String csv_data{ data_node->value(), data_node->value_size() };

                auto parse_tile_id = [&](ice::u32 beg, ice::u32 end) noexcept
                {
                    char const* val_beg = csv_data.data() + beg;
                    char const* val_end = csv_data.data() + end;

                    ice::u32 value = 0;
                    std::from_chars(val_beg, val_end, value);
                    return value;
                };


                ice::u32 total_tiles_read = 0;

                ice::u32 beg = 0;
                ice::u32 end = csv_data.find_first_of(',');
                while (end != ice::string_npos)
                {
                    ice::u32 const value = parse_tile_id(beg, end);
                    if (value != 0)
                    {
                        for (ice::u32 idx = tilemap_info.tileset_count - 1; idx != ~0; --idx)
                        {
                            ice::u32 const gid = tilemap_info.tileset_gids[idx];
                            if (tilemap_info.tileset_gids[idx] < value)
                            {
                                ice::u32 const tileset_columns = tilemap_info.tileset_columns[idx];

                                ice::u8 const value_flips = static_cast<ice::u8>((value & 0xe000'0000) >> 29);
                                ice::u32 const value_noflips = (value & 0x1fff'ffff) - gid;
                                ice::u32 const tile_index = /*layer_tile_count - */total_tiles_read;

                                ice::u32 const tile_x = (tile_index % layer_width);
                                ice::u32 const tile_y = layer_height - (tile_index / layer_width) - 1;

                                tiles->offset = (tile_y << 16) | tile_x;
                                tiles->tile_id = ice::make_tileid(
                                    static_cast<ice::u8>(idx),
                                    value_flips,
                                    value_noflips % tileset_columns,
                                    value_noflips / tileset_columns
                                );
                                tiles += 1;

                                layer.tile_count += 1;
                                break;
                            }
                        }

                    }

                    total_tiles_read += 1;

                    beg = end + 1;
                    end = csv_data.find_first_of(',', beg);
                }

                ice::u32 const value = parse_tile_id(beg, data_node->value_size());
                if (value != 0)
                {
                    for (ice::u32 idx = tilemap_info.tileset_count - 1; idx != ~0; --idx)
                    {
                        ice::u32 const gid = tilemap_info.tileset_gids[idx];
                        if (tilemap_info.tileset_gids[idx] < value)
                        {
                            ice::u32 const tileset_columns = tilemap_info.tileset_columns[idx];

                            ice::u8 const value_flips = static_cast<ice::u8>((value & 0xe000'0000) >> 29);
                            ice::u32 const value_noflips = (value & 0x1fff'ffff) - gid;
                            ice::u32 const tile_index = /*layer_tile_count - */total_tiles_read;

                            ice::u32 const tile_x = (tile_index % layer_width);
                            ice::u32 const tile_y = layer_height - (tile_index / layer_width) - 1;

                            tiles->offset = (tile_y << 16) | tile_x;
                            tiles->tile_id = ice::make_tileid(
                                static_cast<ice::u8>(idx),
                                value_flips,
                                value_noflips % tileset_columns,
                                value_noflips / tileset_columns
                            );
                            tiles += 1;

                            layer.tile_count += 1;
                            break;
                        }
                    }
                }

                total_tiles_read += 1;

                ICE_ASSERT(total_tiles_read == (layer_width * layer_height), "Invalid number or tiles read from the <Data> node.");
            }
        }

        tile_offset += layer.tile_count;

        //attrib = attrib->next_attribute("x");
        //parse_attrib_value(attrib, layer.offset.x);
        //attrib = attrib->next_attribute("y");
        //parse_attrib_value(attrib, layer.offset.y);
    }

    void parse_root(
        rapidxml::xml_node<> const& root_node,
        ice::TileMapInfo const& tilemap_info,
        ice::Tile* tiles
    ) noexcept
    {
        ice::String const root_name = node_name(root_node);
        if (root_name == "map")
        {
            bool is_infinite = false;
            //parse_map(root_node, out_tilemap, is_infinite);

            ice::u32 tileset_idx = 0;
            ice::u32 layer_idx = 0;
            ice::u32 tile_offset = 0;

            rapidxml::xml_node<> const* child_node = root_node.first_node();
            while (child_node != nullptr)
            {
                ice::String child_name = node_name(*child_node);
                if (child_name == "tileset")
                {
                    parse_tileset(*child_node, tilemap_info.tilesets[tileset_idx]);
                    tileset_idx += 1;
                }
                else if (child_name == "layer")
                {
                    parse_layer(*child_node, tilemap_info, tilemap_info.layers[layer_idx], tiles, tile_offset);
                    layer_idx += 1;
                }

                child_node = child_node->next_sibling();
            }
        }
    }

    bool gather_tilemap_info(rapidxml::xml_node<> const& root_node, ice::TileMapInfo& out_tilemap) noexcept
    {
        bool valid_tilemap = true;

        ice::String const root_name = node_name(root_node);
        if (root_name == "map")
        {
            bool is_infinite = false;
            parse_map(root_node, out_tilemap, is_infinite);

            rapidxml::xml_node<> const* child_node = root_node.first_node();
            while (child_node != nullptr)
            {
                ice::String child_name = node_name(*child_node);
                if (child_name == "tileset")
                {
                    ice::u32 gid = 0;
                    valid_tilemap &= parse_attrib_value(child_node->first_attribute("firstgid"), gid);
                    ICE_ASSERT(gid <= 0x0000'ffff, "");

                    out_tilemap.tileset_gids[out_tilemap.tileset_count] = static_cast<ice::u16>(gid);

                    ice::u32 tileset_columns = 0;
                    parse_attrib_value(child_node->first_attribute("columns"), tileset_columns);
                    out_tilemap.tileset_columns[out_tilemap.tileset_count] = tileset_columns;

                    rapidxml::xml_node<> const* image_node = child_node->first_node("image");
                    if (image_node != nullptr)
                    {
                        ice::String image_source;
                        parse_attrib_value(image_node->first_attribute("source"), image_source);
                        out_tilemap.tileset_image[out_tilemap.tileset_count] = image_source;
                    }

                    out_tilemap.tileset_count += 1;
                }
                else if (child_name == "layer")
                {
                    rapidxml::xml_node<> const* data_node = child_node->first_node("data");

                    if (data_node != nullptr)
                    {
                        ice::String encoding;
                        parse_attrib_value(data_node->first_attribute("encoding"), encoding);

                        if (encoding == "csv")
                        {
                            ice::String csv_data{ data_node->value(), data_node->value_size() };

                            ice::u32 beg = 0;
                            ice::u32 end = csv_data.find_first_of(',');
                            while (end != ice::string_npos)
                            {
                                char const* val_beg = csv_data.data() + beg;
                                char const* val_end = csv_data.data() + end;

                                ice::u32 value = 0;
                                if (std::from_chars(val_beg, val_end, value).ptr == val_end)
                                {
                                    out_tilemap.tile_count += (value != 0);
                                }

                                beg = end + 1;
                                end = csv_data.find_first_of(',', beg);
                            }

                            {
                                char const* val_beg = csv_data.data() + beg;
                                char const* val_end = csv_data.data() + data_node->value_size();

                                ice::u32 value = 0;
                                if (std::from_chars(val_beg, val_end, value).ptr == val_end)
                                {
                                    out_tilemap.tile_count += (value != 0);
                                }
                            }
                        }
                    }

                    //rapidxml::xml_attribute<> const* attrib = child_node->first_attribute("width");
                    //valid_tilemap &= parse_attrib_value(*attrib, tileset_size.x);
                    //attrib = attrib->next_attribute("height");
                    //valid_tilemap &= parse_attrib_value(*attrib, tileset_size.y);

                    //out_tilemap.tile_count += tileset_size.x * tileset_size.y;
                    out_tilemap.layer_count += 1;
                }

                child_node = child_node->next_sibling();
            }
        }

        return valid_tilemap;
    }

    auto IceTiledTmxAssetOven::bake(
        ice::Resource& resource,
        ice::ResourceSystem& resource_system,
        ice::AssetSystem& asset_system,
        ice::Allocator& asset_alloc,
        ice::Memory& asset_data
    ) const noexcept -> ice::BakeResult
    {
        Data resource_data = resource.data();
        Metadata const& resource_meta = resource.metadata();

        void* resource_copy = asset_alloc.allocate(resource_data.size + 1, resource_data.alignment);
        ice::memcpy(resource_copy, resource_data.location, resource_data.size);

        char* const raw_doc = reinterpret_cast<char*>(resource_copy);
        raw_doc[resource_data.size] = '\0';

        rapidxml::xml_document<>* doc = asset_alloc.make<rapidxml::xml_document<>>();
        doc->parse<0>(raw_doc);
        if (doc->first_node() != nullptr)
        {
            ice::TileMapInfo tilemap_info{ };

            if (gather_tilemap_info(*doc->first_node(), tilemap_info))
            {
                ice::u32 total_tilemap_bytes = alignof(ice::TileSet) + alignof(ice::TileLayer);
                total_tilemap_bytes += sizeof(ice::TileMap);
                total_tilemap_bytes += sizeof(ice::TileSet) * tilemap_info.tileset_count;
                total_tilemap_bytes += sizeof(ice::TileLayer) * tilemap_info.layer_count;
                total_tilemap_bytes += sizeof(ice::Tile) * tilemap_info.tile_count;

                void* tilemap_data = asset_alloc.allocate(total_tilemap_bytes);

                ice::TileMap* tilemap = reinterpret_cast<ice::TileMap*>(tilemap_data);
                ice::TileSet* tilesets = reinterpret_cast<ice::TileSet*>(
                    ice::memory::ptr_align_forward(tilemap + 1, alignof(ice::TileSet))
                    );
                ice::TileLayer* layers = reinterpret_cast<ice::TileLayer*>(
                    ice::memory::ptr_align_forward(tilesets + tilemap_info.tileset_count, alignof(ice::TileLayer))
                    );
                ice::Tile* tiles = reinterpret_cast<ice::Tile*>(layers + tilemap_info.layer_count);

                tilemap->tile_size = tilemap_info.tile_size;
                tilemap->tileset_count = tilemap_info.tileset_count;
                tilemap->layer_count = tilemap_info.layer_count;
                tilemap->tilesets = nullptr;
                tilemap->layers = nullptr;
                tilemap->tiles = nullptr;

                tilemap_info.tilemap = tilemap;
                tilemap_info.tilesets = tilesets;
                tilemap_info.layers = layers;

                for (ice::u32 idx = 0; idx < tilemap_info.tileset_count; ++idx)
                {
                    ice::Resource* res = resource_system.find_relative(resource, tilemap_info.tileset_image[idx]);
                    ice::Asset asset = asset_system.load(AssetType::Texture, res);

                    tilesets[idx].asset = static_cast<ice::Asset>(ice::stringid_hash(ice::asset_name(asset)));
                }

                ICE_ASSERT(
                    ice::memory::ptr_distance(tilemap_data, tiles + tilemap_info.tile_count) <= total_tilemap_bytes,
                    "Allocation was to small for the given tilemap data."
                );

                parse_root(*doc->first_node(), tilemap_info, tiles);

                // Change pointers into offset values
                tilemap->tiles = reinterpret_cast<ice::Tile*>(
                    static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, tiles))
                    );

                tilemap->tilesets = reinterpret_cast<ice::TileSet*>(
                    static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, tilesets))
                    );

                tilemap->layers = reinterpret_cast<ice::TileLayer*>(
                    static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, layers))
                    );

                asset_data.location = tilemap_data;
                asset_data.size = total_tilemap_bytes;
                asset_data.alignment = alignof(ice::TileMap);
            }
        }
        asset_alloc.destroy(doc);
        asset_alloc.deallocate(resource_copy);

        return BakeResult::Success;
    }

} // namespace ice
