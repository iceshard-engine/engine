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

    namespace detail
    {

        struct TileSetInfo
        {
            ice::u16 gid;
            ice::u16 columns;
            ice::vec2f element_size;
            ice::String image;

            ice::u32 terrain_count;
            ice::u32 terrain_offset;
        };

        struct TileMapInfo
        {
            ice::vec2f map_size;
            ice::vec2f tile_size;

            ice::u32 tileset_count;
            ice::detail::TileSetInfo tileset_info[16];

            ice::u32 layer_count;
            ice::u32 terrain_count;
            ice::u32 tile_count;

            ice::u32* fixture_count;

            ice::TileSet* tilesets;
            ice::TileLayer* layers;
            ice::TileTerrain* terrain;
            ice::Tile* tiles;
        };

        constexpr auto x = sizeof(TileMapInfo);

        bool get_child(
            rapidxml::xml_node<> const* node,
            rapidxml::xml_node<> const*& out_node,
            char const* name = nullptr
        ) noexcept;

        bool get_attrib(
            rapidxml::xml_node<> const* node,
            rapidxml::xml_attribute<> const*& out_attrib,
            char const* name = nullptr
        ) noexcept;

        bool next_sibling(
            rapidxml::xml_node<> const* node,
            rapidxml::xml_node<> const*& out_node,
            char const* name = nullptr
        ) noexcept;

        bool next_attrib(
            rapidxml::xml_attribute<> const* attrib,
            rapidxml::xml_attribute<> const*& out_attrib,
            char const* name = nullptr
        ) noexcept;

        auto node_name(rapidxml::xml_node<> const* node) noexcept -> ice::String;

        template<typename T>
        bool attrib_value(rapidxml::xml_attribute<> const* attrib, T& out) noexcept = delete;

        template<>
        bool attrib_value<ice::String>(rapidxml::xml_attribute<> const* attrib, ice::String& out_value) noexcept;

        template<>
        bool attrib_value<ice::u32>(rapidxml::xml_attribute<> const* attrib, ice::u32& out_value) noexcept;

        template<>
        bool attrib_value<ice::f32>(rapidxml::xml_attribute<> const* attrib, ice::f32& out_value) noexcept;

    } // namespace detail

    using ice::detail::TileMapInfo;

    bool parse_node_map(
        rapidxml::xml_node<> const* node_map,
        ice::TileMapInfo& tilemap_info
    ) noexcept
    {
        rapidxml::xml_attribute<> const* attrib;
        if (detail::get_attrib(node_map, attrib, "version") == false)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, cannot read 'version' attribute from root 'map' node.");
            return false;
        }

        ice::String tiled_map_ver;
        if (detail::attrib_value(attrib, tiled_map_ver))
        {
            TILED_LOG(LogSeverity::Info, "Loading Tiled Map (ver: {})", tiled_map_ver);
        }

        detail::next_attrib(attrib, attrib, "width");
        detail::attrib_value(attrib, tilemap_info.map_size.x);
        detail::next_attrib(attrib, attrib, "height");
        detail::attrib_value(attrib, tilemap_info.map_size.y);
        detail::next_attrib(attrib, attrib, "tilewidth");
        detail::attrib_value(attrib, tilemap_info.tile_size.x);
        detail::next_attrib(attrib, attrib, "tileheight");
        detail::attrib_value(attrib, tilemap_info.tile_size.y);

        if (attrib == nullptr)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, failed to read map and/or tile dimensions.");
        }
        return attrib != nullptr;
    }

    bool parse_node_tileset(
        rapidxml::xml_node<> const* node_tileset,
        ice::TileMapInfo& tilemap_info
    ) noexcept
    {
        ice::u32 const tileset_idx = tilemap_info.tileset_count;

        rapidxml::xml_node<> const* node_image;
        if (detail::get_child(node_tileset, node_image, "image") == false)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, cannot access child 'image' node from 'tileset' node.");
            return false;
        }

        rapidxml::xml_attribute<> const* attrib;
        if (detail::get_attrib(node_image, attrib, "source") == false)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, cannot read 'source' attribute from 'image' node.");
            return false;
        }

        ice::String image_source;
        detail::attrib_value(attrib, image_source);
        tilemap_info.tileset_info[tileset_idx].image = image_source;

        if (detail::get_attrib(node_tileset, attrib, "firstgid") == false)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, cannot read 'firstgid' attribute from 'tileset' node.");
            return false;
        }

        ice::u32 gid = 0;
        detail::attrib_value(attrib, gid);
        if (gid >= 0x0000'ffff)
        {
            // #TODO: double check this, because we are still using 32bits just separated into x and y components (16 bits each)
            TILED_LOG(LogSeverity::Error, "Unsupported TMX resource, 'gid' values over {} are not supported.", 0x0000'ffff);
            return false;
        }
        tilemap_info.tileset_info[tileset_idx].gid = static_cast<ice::u16>(gid);

        ice::String name = "<unknown>";
        detail::next_attrib(attrib, attrib, "name");
        detail::attrib_value(attrib, name);
        TILED_LOG(LogSeverity::Info, "Parsing tileset: {}", name);

        detail::next_attrib(attrib, attrib, "tilewidth");
        detail::attrib_value(attrib, tilemap_info.tileset_info[tileset_idx].element_size.x);
        detail::next_attrib(attrib, attrib, "tileheight");
        detail::attrib_value(attrib, tilemap_info.tileset_info[tileset_idx].element_size.y);

        ice::u32 tileset_columns = 0;
        detail::next_attrib(attrib, attrib, "columns");
        detail::attrib_value(attrib, tileset_columns);
        tilemap_info.tileset_info[tileset_idx].columns = tileset_columns;

        rapidxml::xml_node<> const* node_terrain;
        if (detail::get_child(node_tileset, node_terrain, "terraintypes"))
        {
            detail::get_child(node_terrain, node_terrain, "terrain");
            while (node_terrain != nullptr)
            {
                tilemap_info.tileset_info[tileset_idx].terrain_count += 1;

                detail::next_sibling(node_terrain, node_terrain);
            }
        }

        tilemap_info.terrain_count += tilemap_info.tileset_info[tileset_idx].terrain_count;
        tilemap_info.tileset_count += 1;

        if (attrib == nullptr)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, failed to read one or multiple 'tileset' attributes.");
        }
        return attrib != nullptr;
    }

    template<typename Fn>
    void parse_node_data_csv(
        rapidxml::xml_node<> const* node_data,
        Fn&& tileid_callback
    ) noexcept
    {
        ice::String csv_data{ node_data->value(), node_data->value_size() };

        ice::u32 beg = 0;
        ice::u32 end = csv_data.find_first_of(',');
        while (end != ice::string_npos)
        {
            char const* val_beg = csv_data.data() + beg;
            char const* val_end = csv_data.data() + end;

            ice::u32 value = 0;
            if (std::from_chars(val_beg, val_end, value).ptr == val_end)
            {
                ice::forward<Fn>(tileid_callback)(value);
            }

            beg = end + 1;
            end = csv_data.find_first_of(',', beg);
        }

        {
            char const* val_beg = csv_data.data() + beg;
            char const* val_end = csv_data.data() + node_data->value_size();

            ice::u32 value = 0;
            if (std::from_chars(val_beg, val_end, value).ptr == val_end)
            {
                ice::forward<Fn>(tileid_callback)(value);
            }
        }
    }

    bool parse_node_layer_estimate_data(
        rapidxml::xml_node<> const* node_layer,
        ice::TileMapInfo& tilemap_info
    ) noexcept
    {
        rapidxml::xml_node<> const* node_data;
        if (detail::get_child(node_layer, node_data, "data") == false)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, cannot access child 'data' node from 'layer' node.");
            return false;
        }

        rapidxml::xml_attribute<> const* attrib;
        if (detail::get_attrib(node_data, attrib, "encoding") == false)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, cannot read 'encoding' attribute from 'data' node.");
            return false;
        }

        ice::String data_encoding;
        detail::attrib_value(attrib, data_encoding);
        if (data_encoding != "csv")
        {
            TILED_LOG(LogSeverity::Error, "Unsupported TMX resource, currently only tile data in a 'csv' encoding is supported.");
            return false;
        }

        parse_node_data_csv(node_data, [&](ice::u32 node_id) noexcept
            {
                tilemap_info.tile_count += (node_id != 0);
            }
        );

        tilemap_info.layer_count += 1;

        if (attrib == nullptr)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, failed to read one or multiple 'layer' attributes.");
        }
        return attrib != nullptr;
    }

    bool gather_tilemap_info(
        rapidxml::xml_node<> const* node_map,
        ice::TileMapInfo& tilemap_info
    ) noexcept
    {
        ice::String const name = detail::node_name(node_map);
        if (name != "map")
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, unexpected node '{}' was found.", name);
            return false;
        }

        if (parse_node_map(node_map, tilemap_info) == false)
        {
            return false;
        }

        rapidxml::xml_node<> const* node_child;
        detail::get_child(node_map, node_child);

        bool valid_map = true;
        while (node_child != nullptr && valid_map)
        {
            ice::String const child_name = detail::node_name(node_child);
            if (child_name == "tileset")
            {
                valid_map &= parse_node_tileset(node_child, tilemap_info);
            }
            else if (child_name == "layer")
            {
                valid_map &= parse_node_layer_estimate_data(node_child, tilemap_info);
            }
            else
            {
                TILED_LOG(LogSeverity::Info, "Skipping unsupported node {}...", child_name);
            }

            detail::next_sibling(node_child, node_child);
        }

        if (valid_map == false)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, failed to read one or multiple child nodes.");
        }

        if (tilemap_info.tileset_count == 0 || tilemap_info.layer_count == 0)
        {
            TILED_LOG(
                LogSeverity::Error,
                "Invalid TMX resource, at least one tileset and one layer are expected. [tilesets: {}, layers: {}]",
                tilemap_info.tileset_count,
                tilemap_info.layer_count
            );
            valid_map = false;
        }

        return valid_map;
    }

    void bake_tileset_terraintypes(
        rapidxml::xml_node<> const* node_tileset,
        ice::TileMapInfo const& tilemap_info,
        ice::u32 tileset_index,
        ice::TileTerrain* tileset_terrains
    ) noexcept
    {
        ice::u32 const tileset_columns = tilemap_info.tileset_info[tileset_index].columns;

        rapidxml::xml_node<> const* node_terrain;
        if (detail::get_child(node_tileset, node_terrain, "terraintypes"))
        {
            detail::get_child(node_terrain, node_terrain, "terrain");
            while (node_terrain != nullptr)
            {
                rapidxml::xml_attribute<> const* attrib;
                detail::get_attrib(node_terrain, attrib, "tile");

                ice::u32 local_tile_id;
                detail::attrib_value(attrib, local_tile_id);

                ice::u16 const tile_id = static_cast<ice::u16>(local_tile_id);

                tileset_terrains->tile_id = ice::make_tileid(
                    static_cast<ice::u8>(tileset_index),
                    0,
                    tile_id % tileset_columns,
                    tile_id / tileset_columns
                );

                tileset_terrains += 1;

                detail::next_sibling(node_terrain, node_terrain);
            }
        }
    }

    void bake_layer_tiles(
        rapidxml::xml_node<> const* node_layer,
        ice::TileMapInfo const& tilemap_info,
        ice::TileSet const* tilesets,
        ice::TileTerrain const* tileset_terrains,
        ice::TileLayer& layer,
        ice::Tile* layer_tiles
    ) noexcept
    {
        ice::u32 last_gid = 0xffff'ffff;
        ice::u32 last_tileset_size = 0;
        ice::u32 last_idx = 0xffff'ffff;

        auto const find_local_id = [&](ice::u32& tile_gid) noexcept
        {
            ice::u32 predicted_gid = tile_gid - last_gid;

            // If we access the same tileset in a row multiple tiles we can early return the last tileset index.
            if (predicted_gid < last_tileset_size)
            {
                tile_gid = predicted_gid;
                return last_idx;
            }

            ice::u32 prev_gid = 0xffff'ffff;
            ice::u32 idx = tilemap_info.tileset_count - 1;
            for (; idx != ~0; --idx)
            {
                last_gid = tilemap_info.tileset_info[idx].gid;
                if (tilemap_info.tileset_info[idx].gid < tile_gid)
                {
                    break;
                }
                else
                {
                    prev_gid = last_gid;
                }
            }

            if (idx == ~0)
            {
                last_gid = 0;
                prev_gid = 0;
            }

            last_tileset_size = prev_gid - last_gid;
            last_idx = idx;

            tile_gid -= last_gid;
            return last_idx;
        };


        rapidxml::xml_attribute<> const* attrib;
        detail::get_attrib(node_layer, attrib, "id");

        ice::u32 layer_id;
        detail::attrib_value(attrib, layer_id);
        layer.name = ice::StringID{ static_cast<ice::StringID_Hash>(layer_id) };

        rapidxml::xml_node<> const* node_data;
        detail::get_child(node_layer, node_data, "data");

        // detail::get_attrib(node_data, attrib, "encoding");
        // detail::attrib_value(attrib, encoding_type);

        detail::get_attrib(node_layer, attrib, "width");
        detail::attrib_value(attrib, layer.size.x);
        detail::next_attrib(attrib, attrib, "height");
        detail::attrib_value(attrib, layer.size.y);

        ice::u32 total_tile_count = 0;
        parse_node_data_csv(node_data, [&](ice::u32 tile_value) noexcept
            {
                if (tile_value != 0)
                {
                    ice::u8 const value_flips = static_cast<ice::u8>((tile_value & 0xe000'0000) >> 29);
                    ice::u32 tile_local_id = (tile_value & 0x1fff'ffff);

                    ice::u32 const tileset_idx = find_local_id(tile_local_id);
                    ICE_ASSERT(tileset_idx != ~0, "Node global ID {} couldn not be moved to tileset local ID.", tile_local_id);
                    ice::u16 const tileset_columns = tilemap_info.tileset_info[tileset_idx].columns;

                    ice::u16 const tile_id = static_cast<ice::u16>(tile_local_id);
                    ice::u32 const tile_index = total_tile_count;

                    ice::u32 const tile_x = (tile_index % layer.size.x);
                    ice::u32 const tile_y = layer.size.y - (tile_index / layer.size.x) - 1;

                    layer_tiles->offset = (tile_y << 16) | (0x0000'ffff & tile_x);
                    layer_tiles->tile_id = ice::make_tileid(
                        static_cast<ice::u8>(tileset_idx),
                        value_flips,
                        tile_id % tileset_columns,
                        tile_id / tileset_columns
                    );

                    detail::TileSetInfo const& tileset_info = tilemap_info.tileset_info[tileset_idx];
                    ice::TileTerrain const* terrain_list = tileset_terrains + tileset_info.terrain_offset;
                    for (ice::u32 idx = 0; idx < tileset_info.terrain_count; ++idx)
                    {
                        if (layer_tiles->tile_id == terrain_list[idx].tile_id)
                        {
                           *tilemap_info.fixture_count += 1;
                           break;
                        }
                    }


                    layer_tiles += 1;
                    layer.tile_count += 1;
                }

                total_tile_count += 1;
            }
        );

        if (total_tile_count != (layer.size.x * layer.size.y))
        {
            TILED_LOG(
                LogSeverity::Warning,
                "Invalid TMX resource, number of tile IDs read from 'data' node do not match total number of tiles in layer. [read: {}, expected: {}]",
                total_tile_count,
                (layer.size.x * layer.size.y)
            );
        }
    }

    void bake_tilemap_asset(
        rapidxml::xml_node<> const* node_map,
        ice::TileMapInfo const& tilemap_info
    ) noexcept
    {
        ice::u32 layer_idx = 0;
        ice::u32 layer_tiles_offset = 0;

        ice::u32 tileset_idx = 0;
        ice::u32 tileset_terrain_offset = 0;

        ice::Tile* layer_tiles = tilemap_info.tiles;
        ice::TileTerrain* terrain_tiles = tilemap_info.terrain;

        rapidxml::xml_node<> const* node_child;
        detail::get_child(node_map, node_child);
        while (node_child != nullptr)
        {
            ice::String const child_name = detail::node_name(node_child);
            if (child_name == "tileset")
            {
                bake_tileset_terraintypes(node_child, tilemap_info, tileset_idx, terrain_tiles);

                ice::u32 const terrain_count = tilemap_info.tileset_info[tileset_idx].terrain_count;
                terrain_tiles += terrain_count;
                tileset_terrain_offset += terrain_count;
                tileset_idx += 1;
            }
            else if (child_name == "layer")
            {
                ice::TileLayer& layer = tilemap_info.layers[layer_idx];
                layer.tile_count = 0;

                bake_layer_tiles(node_child, tilemap_info, tilemap_info.tilesets, tilemap_info.terrain, layer, layer_tiles);
                layer.tile_offset = layer_tiles_offset;

                layer_tiles += layer.tile_count;
                layer_tiles_offset += layer.tile_count;
                layer_idx += 1;
            }

            detail::next_sibling(node_child, node_child);
        }
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

        BakeResult result = BakeResult::Failure_InvalidData;

        rapidxml::xml_document<>* doc = asset_alloc.make<rapidxml::xml_document<>>();
        doc->parse<0>(raw_doc);
        if (doc->first_node() != nullptr)
        {
            ice::TileMapInfo tilemap_info{ };
            if (gather_tilemap_info(doc->first_node(), tilemap_info))
            {
                ice::u32 total_tilemap_bytes = alignof(ice::TileSet) + alignof(ice::TileLayer);
                total_tilemap_bytes += sizeof(ice::TileMap);
                total_tilemap_bytes += sizeof(ice::TileSet) * tilemap_info.tileset_count;
                total_tilemap_bytes += sizeof(ice::TileLayer) * tilemap_info.layer_count;
                total_tilemap_bytes += sizeof(ice::Tile) * tilemap_info.tile_count;
                total_tilemap_bytes += sizeof(ice::TileTerrain) * tilemap_info.terrain_count;

                void* tilemap_data = asset_alloc.allocate(total_tilemap_bytes);

                ice::TileMap* tilemap = reinterpret_cast<ice::TileMap*>(tilemap_data);
                ice::TileSet* tilesets = reinterpret_cast<ice::TileSet*>(
                    ice::memory::ptr_align_forward(tilemap + 1, alignof(ice::TileSet))
                );
                ice::TileLayer* layers = reinterpret_cast<ice::TileLayer*>(
                    ice::memory::ptr_align_forward(tilesets + tilemap_info.tileset_count, alignof(ice::TileLayer))
                );
                ice::TileTerrain* terrain_tiles = reinterpret_cast<ice::TileTerrain*>(layers + tilemap_info.layer_count);
                ice::Tile* tiles = reinterpret_cast<ice::Tile*>(terrain_tiles + tilemap_info.terrain_count);

                result = BakeResult::Success;
                for (ice::u32 idx = 0; idx < tilemap_info.tileset_count; ++idx)
                {
                    ice::Resource* dependency_resource = resource_system.find_relative(resource, tilemap_info.tileset_info[idx].image);
                    if (dependency_resource == nullptr)
                    {
                        TILED_LOG(LogSeverity::Error, "Invalid TMX resource, failed to find image resource at: {}", tilemap_info.tileset_info[idx].image);
                        result = BakeResult::Failure_MissingDependencies;
                        break;
                    }

                    ice::Asset dependency_asset = asset_system.load(AssetType::Texture, dependency_resource);
                    if (dependency_asset == Asset::Invalid)
                    {
                        TILED_LOG(LogSeverity::Error, "Invalid TMX resource, failed to load image asset for image at: {}", tilemap_info.tileset_info[idx].image);
                        result = BakeResult::Failure_MissingDependencies;
                        break;
                    }

                    // Baked assets keep the Asset name hash in the same location where later the Asset object is stored.
                    tilesets[idx].element_size = tilemap_info.tileset_info[idx].element_size;
                    tilesets[idx].asset = static_cast<ice::Asset>(ice::stringid_hash(ice::asset_name(dependency_asset)));
                }

                if (result == BakeResult::Success)
                {
                    ICE_ASSERT(
                        ice::memory::ptr_distance(tilemap_data, tiles + tilemap_info.tile_count) <= total_tilemap_bytes,
                        "Allocation was to small for the gathered tilemap data."
                    );

                    tilemap_info.fixture_count = ice::addressof(tilemap->fixture_count);
                    tilemap_info.tilesets = tilesets;
                    tilemap_info.layers = layers;
                    tilemap_info.terrain = terrain_tiles;
                    tilemap_info.tiles = tiles;

                    tilemap->tile_size = tilemap_info.tile_size;
                    tilemap->tileset_count = tilemap_info.tileset_count;
                    tilemap->layer_count = tilemap_info.layer_count;
                    tilemap->terrain_count = tilemap_info.terrain_count;
                    tilemap->fixture_count = 0;
                    tilemap->tilesets = nullptr;
                    tilemap->layers = nullptr;
                    tilemap->terrain = nullptr;
                    tilemap->tiles = nullptr;

                    bake_tilemap_asset(doc->first_node(), tilemap_info);

                    // Change pointers into offset values
                    tilemap->tilesets = reinterpret_cast<ice::TileSet*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, tilesets))
                    );
                    tilemap->layers = reinterpret_cast<ice::TileLayer*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, layers))
                    );
                    tilemap->terrain = reinterpret_cast<ice::TileTerrain*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, terrain_tiles))
                    );
                    tilemap->tiles = reinterpret_cast<ice::Tile*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, tiles))
                    );

                    // Update the output Memory object.
                    asset_data.location = tilemap_data;
                    asset_data.size = total_tilemap_bytes;
                    asset_data.alignment = alignof(ice::TileMap);
                }
                else
                {
                    asset_alloc.deallocate(tilemap_data);
                }
            }
        }

        asset_alloc.destroy(doc);
        asset_alloc.deallocate(resource_copy);
        return result;
    }

    bool detail::get_child(rapidxml::xml_node<> const* node, rapidxml::xml_node<> const*& out_node, char const* name) noexcept
    {
        out_node = nullptr;
        if (node != nullptr)
        {
            out_node = node->first_node(name);
        }
        return out_node != nullptr;
    }

    bool detail::get_attrib(rapidxml::xml_node<> const* node, rapidxml::xml_attribute<> const*& out_attrib, char const* name) noexcept
    {
        out_attrib = nullptr;
        if (node != nullptr)
        {
            out_attrib = node->first_attribute(name);
        }
        return out_attrib != nullptr;
    }

    bool detail::next_sibling(rapidxml::xml_node<> const* node, rapidxml::xml_node<> const*& out_node, char const* name) noexcept
    {
        out_node = nullptr;
        if (node != nullptr)
        {
            out_node = node->next_sibling(name);
        }
        return out_node != nullptr;
    }

    bool detail::next_attrib(rapidxml::xml_attribute<> const* attrib, rapidxml::xml_attribute<> const*& out_attrib, char const* name) noexcept
    {
        out_attrib = nullptr;
        if (attrib != nullptr)
        {
            out_attrib = attrib->next_attribute(name);
        }
        return out_attrib != nullptr;
    }

    auto detail::node_name(rapidxml::xml_node<> const* node) noexcept -> ice::String
    {
        if (node != nullptr)
        {
            return ice::String{ node->name(), node->name_size() };
        }
        return { };
    }

    template<>
    bool detail::attrib_value<ice::String>(rapidxml::xml_attribute<> const* attrib, ice::String& out_value) noexcept
    {
        if (attrib != nullptr)
        {
            out_value = ice::String{ attrib->value(), attrib->value_size() };
            return true;
        }
        return false;
    }

    template<>
    bool detail::attrib_value<ice::u32>(rapidxml::xml_attribute<> const* attrib, ice::u32& out_value) noexcept
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
    bool detail::attrib_value<ice::f32>(rapidxml::xml_attribute<> const* attrib, ice::f32& out_value) noexcept
    {
        if (attrib != nullptr)
        {
            char const* const val_beg = attrib->value();
            char const* const val_end = val_beg + attrib->value_size();

            return std::from_chars(val_beg, val_end, out_value).ptr == val_end;
        }
        return false;
    }

} // namespace ice
