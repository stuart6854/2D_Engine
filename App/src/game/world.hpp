#pragma once

#include "core/core.hpp"

#include <glm/ext/vector_int2.hpp>

#include <vector>

namespace app::game
{
    struct Tile
    {
        f32 Size = 1.0f;
        glm::ivec2 Coord{};

        // #TODO: Temp
        std::string SpriteName{};

        // BaseFloorType - Original rerrain type
        // FloorType -
        // Object - Generated/Placed/Built object
    };

    class World
    {
    public:
        World() = default;
        ~World() = default;

        void set_world_size(u32 width, u32 height);
        void set_tile_size(f32 size);

        void clear();

        /* Getters */

        auto get_width() const -> u32;
        auto get_height() const -> u32;

        auto get_index(u32 x, u32 y) const -> u32;
        auto get_coord(u32 index) const -> glm::ivec2;

        bool is_valid_coord(const glm::uvec2 coord);

        auto get_tile(u32 x, u32 y) -> Tile&;

    private:
        u32 m_worldWidth = 1;
        u32 m_worldHeight = 1;
        std::vector<Tile> m_tiles{};

        f32 m_tileSize = 1.0f;
    };
}