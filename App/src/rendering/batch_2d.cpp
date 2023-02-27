#include "batch_2d.hpp"

#include "renderer.hpp"
#include "buffer.hpp"

namespace app::gfx
{
    static sizet MaxQuadCount = 10000;
    static sizet MaxVertexCount = MaxQuadCount * 4;
    static sizet MaxIndexCount = MaxQuadCount * 6;
    static sizet MaxTextures = 32;  // TODO: Query vulkan for max textures

    struct Vertex
    {
        glm::vec3 Position;
        glm::vec4 Color;
        glm::vec2 TexCoord;
        f32 TexIndex;
    };

    struct Batch2D::BatchData
    {
        bool initialised = false;

        Renderer* renderer = nullptr;

        Shared<Buffer> vertexBuffer = nullptr;
        Shared<Buffer> indexBuffer = nullptr;

        // Texture whiteTexture;

        u32 indexCount = 0;  // How many indices to draw when flushing

        std::vector<Vertex> quadBuffer{};

        // std::array<Texture, MaxTextures> textureSlots;
        u32 textureSlotIndex = 1;
    };

    Batch2D::Batch2D() : m_pimpl(new Batch2D::BatchData) {}

    Batch2D::~Batch2D()
    {
        shutdown();
    }

    void Batch2D::init(Renderer& renderer)
    {
        ASSERT(m_pimpl->initialised == false);
        m_pimpl->initialised = true;

        m_pimpl->renderer = &renderer;

        // Pre-Allocate vertex buffer
        m_pimpl->quadBuffer.reserve(MaxVertexCount);

        // Create Quad buffers
        m_pimpl->vertexBuffer = m_pimpl->renderer->create_buffer();
        m_pimpl->vertexBuffer->init(sizeof(Vertex) * MaxVertexCount, vk::BufferUsageFlagBits::eVertexBuffer, true);

        // Pre-Generate quad indices
        std::vector<u32> indices(MaxIndexCount);
        u32 offset = 0;
        for (sizet i = 0; i < MaxIndexCount; i += 6)
        {
            indices[i + 0] = 0 + offset;
            indices[i + 1] = 1 + offset;
            indices[i + 2] = 2 + offset;

            indices[i + 3] = 2 + offset;
            indices[i + 4] = 3 + offset;
            indices[i + 5] = 0 + offset;

            offset += 4;
        }
        m_pimpl->indexBuffer = m_pimpl->renderer->create_buffer();
        m_pimpl->indexBuffer->init(
            sizeof(u32) * MaxIndexCount, vk::BufferUsageFlagBits::eIndexBuffer, true);  // #TODO: Remove force mappable
        m_pimpl->indexBuffer->write_data(0, sizeof(u32) * indices.size(), indices.data());

        // Create 1x1 white texture
        // u32 color = 0xffffffff;
        // #TODO: Create texture

        // m_pimpl->textureSlots[0] = m_pimpl->whiteTexture;
        for (sizet i = 1; i < MaxTextures; ++i)
        {
            // m_pimpl->textureSlots[i]
        }
    }

    void Batch2D::shutdown()
    {
        m_pimpl->vertexBuffer = nullptr;
        m_pimpl->indexBuffer = nullptr;

        // Clear data
        m_pimpl->quadBuffer.clear();
        m_pimpl->quadBuffer.shrink_to_fit();
    }

    void Batch2D::begin_batch()
    {
        m_pimpl->quadBuffer.clear();
    }

    void Batch2D::end_batch()
    {
        sizet size = m_pimpl->quadBuffer.size() * sizeof(Vertex);
        m_pimpl->vertexBuffer->write_data(0, size, m_pimpl->quadBuffer.data());
    }

    void Batch2D::flush()
    {
        // #TODO: Update texture descriptor binding

        m_pimpl->renderer->draw_indexed(m_pimpl->vertexBuffer.get(), m_pimpl->indexBuffer.get(), m_pimpl->indexCount);

        m_pimpl->indexCount = 0;
        m_pimpl->textureSlotIndex = 1;
    }

    void Batch2D::draw_quad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color)
    {
        if (m_pimpl->indexCount >= MaxIndexCount)
        {
            end_batch();
            flush();
            begin_batch();
        }

        f32 textureIndex = 0.0f;
         
        auto* vert = &m_pimpl->quadBuffer.emplace_back();
        vert->Position = { position.x, position.y, 0.0f };
        vert->Color = color;
        vert->TexCoord = { 0.0f, 0.0f };
        vert->TexIndex = textureIndex;

        vert = &m_pimpl->quadBuffer.emplace_back();
        vert->Position = { position.x + size.x, position.y, 0.0f };
        vert->Color = color;
        vert->TexCoord = { 1.0f, 0.0f };
        vert->TexIndex = textureIndex;

        vert = &m_pimpl->quadBuffer.emplace_back();
        vert->Position = { position.x + size.x, position.y + size.y, 0.0f };
        vert->Color = color;
        vert->TexCoord = { 1.0f, 1.0f };
        vert->TexIndex = textureIndex;

        vert = &m_pimpl->quadBuffer.emplace_back();
        vert->Position = { position.x, position.y + size.y, 0.0f };
        vert->Color = color;
        vert->TexCoord = { 0.0f, 1.0f };
        vert->TexIndex = textureIndex;

        m_pimpl->indexCount += 6;
    }

}
