#pragma once

#include "core/core.hpp"

struct GLFWwindow;

namespace app::gfx
{
    struct RenderMetrics
    {
        sizet TotalAllocatedMem = 0;
        sizet TotalFreedMem = 0;

        sizet GetMemoryUsage() const
        {
            return TotalAllocatedMem - TotalFreedMem;
        }

        u32 DrawCallCount = 0;
        u32 TriangleCount = 0;
    };

    class Shader;
    class Buffer;
    class Texture;

    class Renderer
    {
    public:
        static auto GetMetrics() -> const RenderMetrics&;
        static void ResetMetrics();

        Renderer();
        ~Renderer();

        /* Initialisation / Shutdown*/

        void init();
        void shutdown();

        /* Getters */

        auto get_window_handle() const -> GLFWwindow*;

        bool has_window_requested_close();

        /* Commands */

        auto create_shader() const -> Shared<Shader>;
        auto create_buffer() const -> Shared<Buffer>;
        auto create_texture() const -> Shared<Texture>;

        void new_frame(const glm::vec3 cam_pos, f32 cam_ortho_size);
        void end_frame();

        void bind_shader(Shader* shader);
        void bind_texture(Shader* shader, Texture* texture);

        void set_push_constants(Shader* shader, u32 size, const void* data);

        void draw_indexed(Buffer* vertex_buffer, Buffer* index_buffer, u32 index_count);

    private:
        struct RendererPimpl;
        Owned<RendererPimpl> m_pimpl;
    };
}