#include "world_renderer.hpp"

#include "world.hpp"
#include "rendering/renderer.hpp"
#include "rendering/shader.hpp"
#include "rendering/buffer.hpp"

namespace app::game
{
    void WorldRenderer::init(gfx::Renderer& renderer)
    {
        m_renderer = &renderer;

        m_shader = renderer.create_shader();
        m_shader->init("../../assets/shaders/default.vert.spv", "../../assets/shaders/default.frag.spv");

        m_atlas.init(m_renderer, "../../assets/textures/tileset.json");

        m_vertexBuffer = m_renderer->create_buffer();
        m_indexBuffer = m_renderer->create_buffer();
    }

    void WorldRenderer::set_world(World& world)
    {
        m_world = &world;

        force_rebuild();
    }

    void WorldRenderer::force_rebuild()
    {
        m_isDirty = true;
    }

    void WorldRenderer::render()
    {
        if (m_isDirty)
        {
            rebuild_mesh();
        }

        m_renderer->bind_shader(m_shader.get());
        m_renderer->bind_texture(m_shader.get(), m_atlas.get_texture());

        m_renderer->draw_indexed(m_vertexBuffer.get(), m_indexBuffer.get(), m_indexCount);
    }

    auto WorldRenderer::get_vertex_count() const -> u32
    {
        return static_cast<u32>(m_vertices.size());
    }

    auto WorldRenderer::get_triangle_count() const -> u32
    {
        return static_cast<u32>(m_indices.size() / 3);
    }

    void WorldRenderer::rebuild_mesh()
    {
        m_vertices.clear();
        m_indices.clear();
        m_indexCount = 0;

        for (u32 y = 0; y < m_world->get_height(); ++y)
        {
            for (u32 x = 0; x < m_world->get_width(); ++x)
            {
                const auto& tile = m_world->get_tile(x, y);
                const auto position = glm::vec2(tile.Coord) * tile.Size;

                if (tile.SpriteName.empty())
                    continue;

                const auto& sprite = m_atlas.get_sprite(tile.SpriteName);
                const glm::vec2 uv = { sprite.x, sprite.y };

                auto v1 = add_vertex({ position.x, position.y }, { sprite.MinUV.x, sprite.MinUV.y });
                auto v2 = add_vertex({ position.x + tile.Size, position.y }, { sprite.MaxUV.x, sprite.MinUV.y });
                auto v3 = add_vertex({ position.x + tile.Size, position.y + tile.Size }, { sprite.MaxUV.x, sprite.MaxUV.y });
                auto v4 = add_vertex({ position.x, position.y + tile.Size }, { sprite.MinUV.x, sprite.MaxUV.y });

                add_quad(v1, v2, v3, v4);
            }
        }

        const auto vertex_size = sizeof(Vertex) * m_vertices.size();
        if (m_vertexBuffer->get_size() < vertex_size)
        {
            // Recreate vertex buffer
            m_vertexBuffer->init(vertex_size, vk::BufferUsageFlagBits::eVertexBuffer, true);
        }

        const auto index_size = sizeof(u32) * m_indices.size();
        if (m_indexBuffer->get_size() < index_size)
        {
            // Recreate index buffer
            m_indexBuffer->init(index_size, vk::BufferUsageFlagBits::eIndexBuffer, true);
        }

        m_vertexBuffer->write_data(0, sizeof(Vertex) * m_vertices.size(), m_vertices.data());
        m_indexBuffer->write_data(0, sizeof(u32) * m_indices.size(), m_indices.data());

        m_isDirty = false;
    }

    auto WorldRenderer::add_vertex(const glm::vec2& pos, const glm::vec2& uv) -> u32
    {
        auto& vertex = m_vertices.emplace_back();
        vertex.Position = pos;
        vertex.TexCoord = uv;

        return static_cast<u32>(m_vertices.size() - 1);
    }

    void WorldRenderer::add_quad(u32 a, u32 b, u32 c, u32 d)
    {
        m_indices.push_back(a);
        m_indices.push_back(b);
        m_indices.push_back(c);
        m_indices.push_back(c);
        m_indices.push_back(d);
        m_indices.push_back(a);

        m_indexCount += 6;
    }

}
