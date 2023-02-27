#pragma once

#include "core/core.hpp"

namespace app::gfx
{
    class Renderer;

    class Batch2D
    {
    public:
        Batch2D();
        ~Batch2D();

        void init(Renderer& renderer);
        void shutdown();

        void begin_batch();
        void end_batch();
        void flush();

        void draw_quad(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);

    private:
        struct BatchData;
        Owned<BatchData> m_pimpl;
    };
}