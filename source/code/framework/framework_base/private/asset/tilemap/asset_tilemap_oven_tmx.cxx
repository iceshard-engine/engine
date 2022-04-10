#include "asset_tilemap.hxx"

#include <ice/game_tilemap.hxx>

#include <ice/uri.hxx>
#include <ice/resource.hxx>
#include <ice/resource_meta.hxx>
#include <ice/resource_tracker.hxx>
#include <ice/task_sync_wait.hxx>

#include <ice/asset_storage.hxx>

#include <ice/stack_string.hxx>
#include <ice/log.hxx>

#include <rapidxml/rapidxml.hpp>
#undef assert

#define TILED_LOG(severity, format, ...) \
    ICE_LOG(severity, LogTag_TiledOven, format, ##__VA_ARGS__)

#if ISP_WINDOWS

namespace ice
{

    namespace detail
    {

        inline auto tmx_make_tileset_id(
            ice::u8 tileset_idx,
            ice::u8 tile_flip,
            ice::u16 tile_x,
            ice::u16 tile_y
        ) noexcept -> ice::TileSetID
        {
            ice::u8 iceshard_tile_flip = 0x0;
            iceshard_tile_flip |= (tile_flip & 0b0010) << 1; // diagonal flip -> flip top right with bottom left (rotate left)
            iceshard_tile_flip |= (tile_flip & 0b0100) >> 2; // vertical flip -> flip along X axis
            iceshard_tile_flip |= (tile_flip & 0b1000) >> 2; // horizontal flip -> flip along Y axis

            return ice::make_tileset_id(tileset_idx, iceshard_tile_flip, tile_x, tile_y);
        }

        struct TileCollisionInfo
        {
            ice::TileCollision collision;
            ice::u32 group_id;
        };

        struct TileSetInfo
        {
            ice::u16 gid;
            ice::u16 columns;
            ice::vec2f element_size;
            ice::Utf8String image;

            ice::u32 tile_collision_object_count;
            ice::u32 tile_object_vertex_count;
        };

        struct TileMapInfo
        {
            ice::vec2f map_size;
            ice::vec2f tile_size;

            ice::u32 tileset_count;
            ice::detail::TileSetInfo tileset_info[16];
            ice::detail::TileCollisionInfo* collisions_info;

            ice::u32 layer_count;
            ice::u32 tile_count;
            ice::u32 object_count;
            ice::u32 tile_collision_count;
            ice::u32 object_vertice_count;

            ice::u32* map_collision_count;

            ice::TileSet* tilesets;
            ice::TileLayer* layers;
            ice::Tile* tiles;
            ice::TileObject* objects;
            ice::TileCollision* tile_collisions;
            ice::vec2f* object_vertices;
        };

        constexpr auto x = sizeof(TileMapInfo);

        bool get_child(
            rapidxml::xml_node<> const* parent_node,
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
        bool attrib_value<ice::Utf8String>(rapidxml::xml_attribute<> const* attrib, ice::Utf8String& out_value) noexcept;

        template<>
        bool attrib_value<ice::u32>(rapidxml::xml_attribute<> const* attrib, ice::u32& out_value) noexcept;

        template<>
        bool attrib_value<ice::f32>(rapidxml::xml_attribute<> const* attrib, ice::f32& out_value) noexcept;

    } // namespace detail

    using ice::detail::TileMapInfo;



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

            while (std::isdigit(*val_beg) == false && val_beg != val_end)
            {
                val_beg += 1;
            }

            ice::u32 value = 0;
            if (std::from_chars(val_beg, val_end, value).ec == std::errc{ 0 })
            {
                ice::forward<Fn>(tileid_callback)(value);
            }

            beg = end + 1;
            end = csv_data.find_first_of(',', beg);
        }

        {
            char const* val_beg = csv_data.data() + beg;
            char const* val_end = csv_data.data() + node_data->value_size();

            while (std::isdigit(*val_beg) == false && val_beg != val_end)
            {
                val_beg += 1;
            }

            ice::u32 value = 0;
            if (std::from_chars(val_beg, val_end, value).ec == std::errc{ 0 })
            {
                ice::forward<Fn>(tileid_callback)(value);
            }
        }
    }

    template<typename Fn>
    void parse_points_string(
        ice::String points,
        Fn&& point_callback
    ) noexcept
    {
        char const* const data = points.data();

        ice::u32 beg = 0;
        ice::u32 end = points.find_first_of(' ');
        while (end != ice::string_npos)
        {
            char const* val_beg = data + beg;
            char const* val_delim = data + points.find_first_of(',', beg);
            char const* val_end = data + end;

            while (std::isdigit(*val_beg) == false && val_beg != val_end)
            {
                val_beg += 1;
            }

            ice::vec2f value{ 0 };
            bool result = std::from_chars(val_beg, val_delim, value.x).ec == std::errc{ 0 };
            result &= std::from_chars(val_delim + 1, val_end, value.y).ec == std::errc{ 0 };

            if (result)
            {
                ice::forward<Fn>(point_callback)(value);
            }

            beg = end + 1;
            end = points.find_first_of(' ', beg);
        }

        {
            char const* val_beg = data + beg;
            char const* val_delim = data + points.find_first_of(',', beg);
            char const* val_end = data + points.size();

            while (std::isdigit(*val_beg) == false && val_beg != val_end)
            {
                val_beg += 1;
            }

            ice::vec2f value{ 0 };
            bool result = std::from_chars(val_beg, val_delim, value.x).ec == std::errc{ 0 };
            result &= std::from_chars(val_delim + 1, val_end, value.y).ec == std::errc{ 0 };

            if (result)
            {
                ice::forward<Fn>(point_callback)(value);
            }
        }
    }

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

        ice::Utf8String image_source;
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

        ice::u32 tile_collision_count = 0;
        ice::u32 collision_object_count = 0;
        ice::u32 vertice_count = 0;

        rapidxml::xml_node<> const* node_tile;
        if (detail::get_child(node_tileset, node_tile, "tile"))
        {
            while (node_tile != nullptr)
            {
                rapidxml::xml_node<> const* node_object;
                if (detail::get_child(node_tile, node_object, "objectgroup"))
                {
                    detail::get_attrib(node_object, attrib, "id");

                    ice::u32 group_id = 0;
                    detail::attrib_value(attrib, group_id);

                    detail::get_child(node_object, node_object, "object");

                    while (node_object != nullptr)
                    {
                        collision_object_count += 1;

                        rapidxml::xml_node<> const* node_polygon;
                        if (detail::get_child(node_object, node_polygon, "ploygon"))
                        {
                            ice::String points;
                            detail::get_attrib(node_polygon, attrib, "points");
                            detail::attrib_value(attrib, points);

                            parse_points_string(
                                points,
                                [&](ice::vec2f point) noexcept
                                {
                                    vertice_count += 1;
                                }
                            );
                        }
                        else
                        {
                            vertice_count += 4;
                        }
                        tile_collision_count += 1;

                        detail::next_sibling(node_object, node_object, "object");
                    }
                }

                detail::next_sibling(node_tile, node_tile);
            }
        }

        tilemap_info.tileset_info[tileset_idx].tile_object_vertex_count = vertice_count;
        tilemap_info.tileset_info[tileset_idx].tile_collision_object_count = collision_object_count;

        tilemap_info.tile_collision_count += tile_collision_count;
        tilemap_info.object_vertice_count += vertice_count;
        tilemap_info.object_count += collision_object_count;
        tilemap_info.tileset_count += 1;

        if (attrib == nullptr)
        {
            TILED_LOG(LogSeverity::Error, "Invalid TMX resource, failed to read one or multiple 'tileset' attributes.");
        }
        return attrib != nullptr;
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

        parse_node_data_csv(
            node_data,
            [&](ice::u32 tile_value) noexcept
            {
                tilemap_info.tile_count += (tile_value != 0);
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

    void bake_tileset_collision_objects(
        rapidxml::xml_node<> const* node_tileset,
        ice::TileMapInfo const& tilemap_info,
        ice::u32 tileset_index,
        ice::detail::TileCollisionInfo* collision_infos,
        ice::TileCollision*& collisions,
        ice::u32 object_offset,
        ice::TileObject* objects,
        ice::u32 vertice_offset,
        ice::vec2f* vertices
    ) noexcept
    {
        ice::u32 const tileset_columns = tilemap_info.tileset_info[tileset_index].columns;
        ice::detail::TileCollisionInfo* const collision_infos_beg = collision_infos;

        ice::u32 collision_count = 0;

        rapidxml::xml_node<> const* node_tile;
        if (detail::get_child(node_tileset, node_tile, "tile"))
        {
            while (node_tile != nullptr)
            {
                rapidxml::xml_attribute<> const* attrib;
                detail::get_attrib(node_tile, attrib, "id");

                ice::u32 local_tile_id;
                detail::attrib_value(attrib, local_tile_id);

                rapidxml::xml_node<> const* node_object;
                if (detail::get_child(node_tile, node_object, "objectgroup"))
                {
                    ice::TileSetID const tile_id = ice::detail::tmx_make_tileset_id(
                        static_cast<ice::u8>(tileset_index),
                        0,
                        static_cast<ice::u16>(local_tile_id % tileset_columns),
                        static_cast<ice::u16>(local_tile_id / tileset_columns)
                    );

                    ice::u32 group_id = 0;
                    detail::get_attrib(node_object, attrib, "id");
                    detail::attrib_value(attrib, group_id);

                    detail::get_child(node_object, node_object, "object");

                    while (node_object != nullptr)
                    {
                        objects->vertex_count = 0;
                        objects->vertex_offset = vertice_offset;

                        ice::vec2f pos;
                        ice::vec2f size;

                        bool valid_object = true;
                        valid_object &= detail::get_attrib(node_object, attrib, "x");
                        detail::attrib_value(attrib, pos.x);
                        valid_object &= detail::next_attrib(attrib, attrib, "y");
                        detail::attrib_value(attrib, pos.y);
                        bool valid_square = detail::next_attrib(attrib, attrib, "width");
                        detail::attrib_value(attrib, size.x);
                        valid_square &= detail::next_attrib(attrib, attrib, "height");
                        detail::attrib_value(attrib, size.y);

                        ice::vec2f const y_offset{ 0.f, tilemap_info.tile_size.y };

                        if (valid_object && valid_square)
                        {
                            ice::vec2f const half = size / 2.f;

                            vertices[0] = ice::vec2f { pos.x, tilemap_info.tile_size.y - pos.y } + vec2f{ 0.f, 0.f };
                            vertices[1] = ice::vec2f { pos.x, tilemap_info.tile_size.y - pos.y } + ice::vec2f{ size.x, 0.f };
                            vertices[2] = ice::vec2f { pos.x, tilemap_info.tile_size.y - pos.y } + ice::vec2f{ size.x, -size.y };
                            vertices[3] = ice::vec2f { pos.x, tilemap_info.tile_size.y - pos.y } + ice::vec2f{ 0.f, -size.y };

                            objects->vertex_count = 4;
                        }
                        else if (valid_object)
                        {
                            rapidxml::xml_node<> const* node_polygon;
                            if (detail::get_child(node_object, node_polygon, "polygon"))
                            {
                                ice::String points;
                                detail::get_attrib(node_polygon, attrib, "points");
                                detail::attrib_value(attrib, points);

                                objects->vertex_count = 0;

                                parse_points_string(
                                    points,
                                    [&](ice::vec2f point) noexcept
                                    {
                                        //vertices[objects->vertex_count] = pos + (y_offset + vec2f{ point.x, -point.y });

                                        vertices[objects->vertex_count] = ice::vec2f { pos.x, tilemap_info.tile_size.y - pos.y } + vec2f{point.x, -point.y};
                                        objects->vertex_count += 1;
                                    }
                                );
                            }
                        }

                        collisions->object_offset = object_offset;
                        collisions->tile_id = tile_id;

                        collision_infos->collision = *collisions;
                        collision_infos->group_id = group_id;

                        collision_infos += 1;
                        collisions += 1;

                        vertice_offset += objects->vertex_count;
                        vertices += objects->vertex_count;
                        object_offset += 1;
                        objects += 1;

                        detail::next_sibling(node_object, node_object, "object");
                    }
                }

                detail::next_sibling(node_tile, node_tile);
            }
        }
    }

    void bake_layer_tiles(
        rapidxml::xml_node<> const* node_layer,
        ice::TileMapInfo const& tilemap_info,
        ice::TileSet const* tilesets,
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
        parse_node_data_csv(
            node_data,
            [&](ice::u32 tile_value) noexcept
            {
                if (tile_value != 0)
                {
                    ice::u8 const value_flips = static_cast<ice::u8>((tile_value & 0xe000'0000) >> 28);
                    ice::u32 tile_local_id = (tile_value & 0x1fff'ffff);

                    ice::u32 const tileset_idx = find_local_id(tile_local_id);
                    ICE_ASSERT(tileset_idx != ~0, "Node global ID {} couldn not be moved to tileset local ID.", tile_local_id);
                    ice::u16 const tileset_columns = tilemap_info.tileset_info[tileset_idx].columns;

                    ice::u16 const tile_id = static_cast<ice::u16>(tile_local_id);
                    ice::u32 const tile_index = total_tile_count;

                    ice::u32 const tile_x = (tile_index % layer.size.x);
                    ice::u32 const tile_y = layer.size.y - (tile_index / layer.size.x) - 1;

                    layer_tiles->offset = (tile_y << 16) | (0x0000'ffff & tile_x);
                    layer_tiles->tile_id = ice::detail::tmx_make_tileset_id(
                        static_cast<ice::u8>(tileset_idx),
                        value_flips,
                        tile_id % tileset_columns,
                        tile_id / tileset_columns
                    );

                    ice::TileCollision const* tile_collissions = tilemap_info.tile_collisions;
                    for (ice::u32 idx = 0; idx < tilemap_info.tile_collision_count; ++idx)
                    {
                        if (layer_tiles->tile_id == tile_collissions[idx].tile_id)
                        {
                            *tilemap_info.map_collision_count += 1;
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
        ice::Allocator& alloc,
        rapidxml::xml_node<> const* node_map,
        ice::TileMapInfo const& tilemap_info
    ) noexcept
    {
        ice::u32 layer_idx = 0;
        ice::u32 layer_tiles_offset = 0;

        ice::u32 tileset_idx = 0;
        ice::u32 objects_offset = 0;
        ice::u32 object_vertice_offset = 0;

        ice::Tile* layer_tiles = tilemap_info.tiles;
        ice::TileObject* objects = tilemap_info.objects;
        ice::vec2f* object_vertices = tilemap_info.object_vertices;
        ice::TileCollision* tile_collisions = tilemap_info.tile_collisions;

        ice::detail::TileCollisionInfo* tile_collision_info = reinterpret_cast<ice::detail::TileCollisionInfo*>(
            alloc.allocate(sizeof(ice::detail::TileCollisionInfo) * tilemap_info.tile_collision_count, alignof(ice::detail::TileCollisionInfo))
        );

        rapidxml::xml_node<> const* node_child;
        detail::get_child(node_map, node_child);
        while (node_child != nullptr)
        {
            ice::String const child_name = detail::node_name(node_child);
            if (child_name == "tileset")
            {
                bake_tileset_collision_objects(
                    node_child,
                    tilemap_info,
                    tileset_idx,
                    tile_collision_info,
                    tile_collisions,
                    objects_offset,
                    objects,
                    object_vertice_offset,
                    object_vertices
                );

                ice::u32 const object_count = tilemap_info.tileset_info[tileset_idx].tile_collision_object_count;
                ice::u32 const vertex_count = tilemap_info.tileset_info[tileset_idx].tile_object_vertex_count;

                objects_offset += object_count;
                objects += object_count;

                object_vertice_offset += vertex_count;
                object_vertices += vertex_count;

                tileset_idx += 1;
            }
            else if (child_name == "layer")
            {
                ice::TileLayer& layer = tilemap_info.layers[layer_idx];
                layer.tile_count = 0;

                bake_layer_tiles(node_child, tilemap_info, tilemap_info.tilesets, layer, layer_tiles);
                layer.tile_offset = layer_tiles_offset;

                layer_tiles += layer.tile_count;
                layer_tiles_offset += layer.tile_count;
                layer_idx += 1;
            }

            detail::next_sibling(node_child, node_child);
        }

        alloc.destroy(tile_collision_info);

        ice::u32 const tile_collision_count = tile_collisions - tilemap_info.tile_collisions;
        ICE_ASSERT(tile_collision_count <= tilemap_info.tile_collision_count, "");
    }

    bool asset_tilemap_oven_tmx(
        void* userdata,
        ice::Allocator& asset_alloc,
        ice::ResourceTracker const& resource_tracker,
        ice::Resource_v2 const& resource,
        ice::Data resource_data,
        ice::Memory& out_data
    ) noexcept
    {
        Metadata const& resource_meta = resource.metadata();

        void* resource_copy = asset_alloc.allocate(resource_data.size + 1, resource_data.alignment);
        ice::memcpy(resource_copy, resource_data.location, resource_data.size);

        char* const raw_doc = reinterpret_cast<char*>(resource_copy);
        raw_doc[resource_data.size] = '\0';

        bool result = false;

        rapidxml::xml_document<>* doc = asset_alloc.make<rapidxml::xml_document<>>();
        doc->parse<0>(raw_doc);
        if (doc->first_node() != nullptr)
        {
            ice::TileMapInfo* tilemap_info_ptr = asset_alloc.make<ice::TileMapInfo>();
            ice::TileMapInfo& tilemap_info = *tilemap_info_ptr;

            if (gather_tilemap_info(doc->first_node(), tilemap_info))
            {
                ice::u32 total_tilemap_bytes = alignof(ice::TileSet) + alignof(ice::TileLayer);
                total_tilemap_bytes += sizeof(ice::TileMap);
                total_tilemap_bytes += sizeof(ice::TileSet) * tilemap_info.tileset_count;

                // Storage for tileset asset names
                ice::u32 total_tileset_assets_size = 0;
                for (ice::u32 idx = 0; idx < tilemap_info.tileset_count; ++idx)
                {
                    ice::ResourceHandle* const self = resource_tracker.find_resource(resource.uri());
                    ice::ResourceHandle* const image_res = resource_tracker.find_resource_relative(
                        ice::URI{ ice::scheme_file,  tilemap_info.tileset_info[idx].image },
                        self
                    );

                    if (image_res == nullptr)
                    {
                        result = false;
                        break;
                    }

                    ice::Utf8String origin = ice::resource_path(image_res);
                    ice::Utf8String name = origin.substr(0, origin.find_last_of('.'));
                    total_tileset_assets_size += name.size() + 1;
                }

                total_tilemap_bytes += total_tileset_assets_size;
                total_tilemap_bytes += sizeof(ice::TileLayer) * tilemap_info.layer_count;

                total_tilemap_bytes += sizeof(ice::Tile) * tilemap_info.tile_count;
                total_tilemap_bytes += sizeof(ice::TileObject) * tilemap_info.object_count;
                total_tilemap_bytes += sizeof(ice::TileCollision) * tilemap_info.tile_collision_count;
                total_tilemap_bytes += sizeof(ice::vec2f) * tilemap_info.object_vertice_count;

                // This ensures we dont need to care about alignment of the below 3 types
                static_assert(alignof(ice::Tile) == 4 && sizeof(ice::Tile) % 4 == 0);
                static_assert(alignof(ice::TileObject) == 4 && sizeof(ice::TileObject) % 4 == 0);
                static_assert(alignof(ice::TileCollision) == 4 && sizeof(ice::TileCollision) % 4 == 0);
                static_assert(alignof(ice::vec2f) == 4 && sizeof(ice::vec2f) % 4 == 0);

                void* tilemap_data = asset_alloc.allocate(total_tilemap_bytes);

                ice::TileMap* tilemap = reinterpret_cast<ice::TileMap*>(tilemap_data);
                ice::TileSet* tilesets = reinterpret_cast<ice::TileSet*>(
                    ice::memory::ptr_align_forward(tilemap + 1, alignof(ice::TileSet))
                );
                ice::c8utf* tileset_assets = reinterpret_cast<ice::c8utf*>(
                    ice::memory::ptr_align_forward(tilesets + tilemap_info.tileset_count, 1)
                );
                ice::TileLayer* layers = reinterpret_cast<ice::TileLayer*>(
                    ice::memory::ptr_align_forward(tileset_assets + total_tileset_assets_size, alignof(ice::TileLayer))
                );
                ice::Tile* tiles = reinterpret_cast<ice::Tile*>(layers + tilemap_info.layer_count);
                ice::TileObject* objects = reinterpret_cast<ice::TileObject*>(tiles + tilemap_info.tile_count);
                ice::TileCollision* tile_collisions = reinterpret_cast<ice::TileCollision*>(objects + tilemap_info.object_count);
                ice::vec2f* object_vertices = reinterpret_cast<ice::vec2f*>(tile_collisions + tilemap_info.tile_collision_count);

                result = true;

                ice::u32 asset_str_offset = 0;
                for (ice::u32 idx = 0; idx < tilemap_info.tileset_count; ++idx)
                {
                    ice::ResourceHandle* const self = resource_tracker.find_resource(resource.uri());
                    ice::ResourceHandle* const image_res = resource_tracker.find_resource_relative(
                        ice::URI{ ice::scheme_file,  tilemap_info.tileset_info[idx].image },
                        self
                    );

                    if (image_res == nullptr)
                    {
                        result = false;
                        break;
                    }

                    ice::Utf8String origin = ice::resource_path(image_res);
                    ice::Utf8String name = origin.substr(0, origin.find_last_of('.'));
                    ice::c8utf const* tileset_asset_str = name.data();
                    ice::u32 const tileset_asset_len = name.size();

                    ice::memcpy(
                        tileset_assets,
                        tileset_asset_str,
                        tileset_asset_len
                    );

                    tilesets[idx].element_size = tilemap_info.tileset_info[idx].element_size;

                    ice::u32* asset_loc = reinterpret_cast<ice::u32*>(&tilesets[idx].asset);// = ice::Utf8String{ tileset_assets, tileset_asset_len };
                    asset_loc[0] = asset_str_offset;
                    asset_loc[1] = tileset_asset_len;

                    tileset_assets += tileset_asset_len;
                    asset_str_offset += tileset_asset_len;
                }

                if (result == true)
                {
                    ICE_ASSERT(
                        ice::memory::ptr_distance(tilemap_data, tiles + tilemap_info.tile_count) <= total_tilemap_bytes,
                        "Allocation was to small for the gathered tilemap data."
                    );

                    //tilemap_info.tile_collision_count = ice::addressof(tilemap->tile_collisions);
                    tilemap_info.tilesets = tilesets;
                    tilemap_info.layers = layers;
                    tilemap_info.tiles = tiles;
                    tilemap_info.objects = objects;
                    tilemap_info.tile_collisions = tile_collisions;
                    tilemap_info.object_vertices = object_vertices;
                    tilemap_info.map_collision_count = &tilemap->map_collision_count;

                    tilemap->tile_size = tilemap_info.tile_size;
                    tilemap->tileset_count = tilemap_info.tileset_count;
                    tilemap->layer_count = tilemap_info.layer_count;
                    tilemap->objects_count = tilemap_info.object_count;
                    tilemap->tile_collision_count = tilemap_info.tile_collision_count;
                    tilemap->map_collision_count = 0;
                    tilemap->tilesets = nullptr;
                    tilemap->layers = nullptr;
                    tilemap->tiles = nullptr;
                    tilemap->objects = nullptr;
                    tilemap->tile_collisions = nullptr;
                    tilemap->object_vertices = nullptr;

                    bake_tilemap_asset(asset_alloc, doc->first_node(), tilemap_info);

                    // Change pointers into offset values
                    tilemap->tilesets = reinterpret_cast<ice::TileSet*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, tilesets))
                    );
                    tilemap->layers = reinterpret_cast<ice::TileLayer*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, layers))
                    );
                    tilemap->tiles = reinterpret_cast<ice::Tile*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, tiles))
                    );
                    tilemap->objects = reinterpret_cast<ice::TileObject*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, objects))
                    );
                    tilemap->tile_collisions = reinterpret_cast<ice::TileCollision*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, tile_collisions))
                    );
                    tilemap->object_vertices = reinterpret_cast<ice::vec2f*>(
                        static_cast<ice::uptr>(ice::memory::ptr_distance(tilemap_data, object_vertices))
                    );

                    // Update the output Memory object.
                    out_data.location = tilemap_data;
                    out_data.size = total_tilemap_bytes;
                    out_data.alignment = alignof(ice::TileMap);
                }
                else
                {
                    asset_alloc.deallocate(tilemap_data);
                }
            }

            asset_alloc.destroy(tilemap_info_ptr);
        }

        asset_alloc.destroy(doc);
        asset_alloc.deallocate(resource_copy);
        return result;
    }

    bool detail::get_child(rapidxml::xml_node<> const* parent_node, rapidxml::xml_node<> const*& out_node, char const* name) noexcept
    {
        out_node = nullptr;
        if (parent_node != nullptr)
        {
            out_node = parent_node->first_node(name);
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
    bool detail::attrib_value<ice::Utf8String>(rapidxml::xml_attribute<> const* attrib, ice::Utf8String& out_value) noexcept
    {
        if (attrib != nullptr)
        {
            out_value = ice::Utf8String{ (char8_t*) attrib->value(), attrib->value_size() };
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

#endif
