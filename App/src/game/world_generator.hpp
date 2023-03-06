#pragma once

#include "core/core.hpp"
#include "perlin_noise.hpp"

#include <vector>
#include <string>
#include <random>

namespace app::game
{
    class World;

    class WorldGenerator
    {
    public:
        WorldGenerator() = default;
        ~WorldGenerator() = default;

        void set_world(World& world);

        void reset();

        void generate(u32 steps);
        void step();

    private:
        struct Cell;

        auto count_neighbours_of_type(const std::vector<Cell>& cells, i32 x, i32 y, u32 type_to_count, u32 out_of_bounds_type) -> u32;

    private:
        World* m_world = nullptr;
        siv::PerlinNoise m_noise{};

        struct Cell
        {
            glm::ivec2 Coord{};
            i32 Type = -1;
        };
        std::vector<Cell> m_cells{};
    };
}