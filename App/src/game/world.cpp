#include "world.hpp"

namespace app::game
{
    void World::set_world_size(u32 width, u32 height)
    {
        m_worldWidth = width;
        m_worldHeight = height;

        const u32 size = m_worldWidth * m_worldHeight;
        m_tiles.resize(size);

        for (u32 i = 0; i < size; ++i)
        {
            auto& tile = m_tiles[i];
            tile.Size = m_tileSize;
            tile.Coord = get_coord(i);
        }
    }

    void World::set_tile_size(f32 size)
    {
        m_tileSize = size;

        for (auto& tile : m_tiles)
        {
            tile.Size = m_tileSize;
        }
    }

    void World::clear()
    {
        for (u32 i = 0; i < m_tiles.size(); ++i)
        {
            auto& tile = m_tiles[i];
            tile.Coord = get_coord(i);
            tile.Size = m_tileSize;
            tile.SpriteName = "";
        }
    }

    auto World::get_width() const -> u32
    {
        return m_worldWidth;
    }

    auto World::get_height() const -> u32
    {
        return m_worldHeight;
    }

    auto World::get_index(u32 x, u32 y) const -> u32
    {
        return x + m_worldWidth * y;
    }

    auto World::get_coord(u32 index) const -> glm::ivec2
    {
        return { index % m_worldWidth, index / m_worldWidth };
    }

    bool World::is_valid_coord(const glm::uvec2 coord)
    {
        return coord.x >= 0 && coord.x < m_worldWidth && coord.y >= 0 && coord.y < m_worldHeight;
    }

    auto World::get_tile(u32 x, u32 y) -> Tile&
    {
        const auto index = get_index(x, y);
        ASSERT(index >= 0 && index < m_tiles.size());

        return m_tiles[index];
    }

}
