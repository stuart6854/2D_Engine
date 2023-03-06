#include "world_generator.hpp"

#include "core/core.hpp"
#include "world.hpp"

#include <nlohmann/json.hpp>

#include <stack>
#include <random>
#include <fstream>

namespace app::game
{
    const u32 CELL_TYPE_WATER = 0;
    const u32 CELL_TYPE_GROUND = 1;

    void WorldGenerator::set_world(World& world)
    {
        m_world = &world;

        reset();
    }

    void WorldGenerator::reset()
    {
        std::mt19937 gen(1998);
        std::uniform_real_distribution<> dist(0.0, 1.0);

        m_cells.resize(m_world->get_width() * m_world->get_height());
        for (u32 i = 0; i < m_cells.size(); ++i)
        {
            auto& cell = m_cells[i];
            cell.Coord = m_world->get_coord(i);
            cell.Type = dist(gen) > 0.5f ? CELL_TYPE_GROUND : CELL_TYPE_WATER;
        }

        for (const auto& cell : m_cells)
        {
            auto& world_tile = m_world->get_tile(cell.Coord.x, cell.Coord.y);
            if (cell.Type == CELL_TYPE_WATER)
            {
                world_tile.SpriteName = "water_0";
            }
            else if (cell.Type == CELL_TYPE_GROUND)
            {
                world_tile.SpriteName = "grass_0";
            }
        }
    }

    void WorldGenerator::generate(u32 steps)
    {
        ASSERT(m_world != nullptr);

        for (u32 i = 0; i < steps; ++i)
        {
            step();
        }
    }

    void WorldGenerator::step()
    {
        const std::vector<Cell> temp_cells = m_cells;
        for (i32 y = 0; y < static_cast<i32>(m_world->get_height()); ++y)
        {
            for (i32 x = 0; x < static_cast<i32>(m_world->get_width()); ++x)
            {
                const auto cell_index = m_world->get_index(x, y);

                const bool is_cell_water = temp_cells[cell_index].Type == CELL_TYPE_WATER;
                const bool is_cell_ground = temp_cells[cell_index].Type == CELL_TYPE_GROUND;
                if (is_cell_ground)
                {
                    const u32 water_count = count_neighbours_of_type(temp_cells, x, y, CELL_TYPE_WATER, CELL_TYPE_GROUND);
                    if (water_count >= 5)
                    {
                        m_cells[cell_index].Type = CELL_TYPE_WATER;
                    }
                }
                else if (is_cell_water)
                {
                    const u32 ground_count = count_neighbours_of_type(temp_cells, x, y, CELL_TYPE_GROUND, CELL_TYPE_WATER);
                    if (ground_count >= 5)
                    {
                        m_cells[cell_index].Type = CELL_TYPE_GROUND;
                    }
                }
            }
        }

        m_world->clear();
        for (const auto& cell : m_cells)
        {
            auto& world_tile = m_world->get_tile(cell.Coord.x, cell.Coord.y);
            if (cell.Type == 0)
            {
                world_tile.SpriteName = "water_0";
            }
            else if (cell.Type == 1)
            {
                world_tile.SpriteName = "grass_0";
            }
        }
    }

    auto WorldGenerator::count_neighbours_of_type(const std::vector<Cell>& cells, i32 x, i32 y, u32 type_to_count, u32 out_of_bounds_type)
        -> u32
    {
        u32 count = 0;
        for (i32 ny = y - 1; ny <= y + 1; ++ny)
        {
            for (i32 nx = x - 1; nx <= x + 1; ++nx)
            {
                auto n_index = m_world->get_index(nx, ny);

                u32 cell_type = out_of_bounds_type;
                if (n_index >= 0 && n_index < cells.size())
                {
                    cell_type = cells[n_index].Type;
                }

                if (cell_type == type_to_count)
                {
                    ++count;
                }
            }
        }
        return count;
    }

}