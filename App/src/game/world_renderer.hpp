#pragma once

#include "core/core.hpp"

#include "texture_atlas.hpp"

namespace app
{
    namespace gfx
    {
        class Renderer;
        class Shader;
        class Buffer;
    }

    namespace game
    {
        class World;

        class WorldRenderer
        {
        public:
            WorldRenderer() = default;
            ~WorldRenderer() = default;

            void init(gfx::Renderer& renderer);

            void set_world(World& world);

            /* Commands */

            void force_rebuild();

            void render();

            auto get_vertex_count() const -> u32;
            auto get_triangle_count() const -> u32;

        private:
            void rebuild_mesh();

            auto add_vertex(const glm::vec2& pos, const glm::vec2& uv) -> u32;
            void add_quad(u32 a, u32 b, u32 c, u32 d);

        private:
            gfx::Renderer* m_renderer = nullptr;
            World* m_world = nullptr;

            Shared<gfx::Shader> m_shader = nullptr;
            TextureAtlas m_atlas{};

            Shared<gfx::Buffer> m_vertexBuffer = nullptr;
            Shared<gfx::Buffer> m_indexBuffer = nullptr;
            u32 m_indexCount = 0;

            struct Vertex
            {
                glm::vec2 Position{};
                glm::vec2 TexCoord{};
            };
            std::vector<Vertex> m_vertices{};
            std::vector<u32> m_indices{};

            bool m_isDirty = false;
        };
    }
}