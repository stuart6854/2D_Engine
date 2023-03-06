#include "application.hpp"

#include "core.hpp"
#include "rendering/renderer.hpp"

#include <glm/glm.hpp>

#include <imgui.h>
#include <implot.h>

#include <GLFW/glfw3.h>

#include <iostream>
#include <functional>
#include <filesystem>

static bool g_isAppRunning;

static app::core::Application* s_Instance = nullptr;

struct ScrollingBuffer
{
    app::u32 MaxSize;
    app::u32 Offset;
    ImVector<ImVec2> Data;
    ScrollingBuffer(int max_size = 2000)
    {
        MaxSize = max_size;
        Offset = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y)
    {
        if (Data.size() < static_cast<app::sizet>(MaxSize))
            Data.push_back(ImVec2(x, y));
        else
        {
            Data[Offset] = ImVec2(x, y);
            Offset = (Offset + 1) % MaxSize;
        }
    }
    void Erase()
    {
        if (Data.size() > 0)
        {
            Data.shrink(0);
            Offset = 0;
        }
    }
};

void draw_cpu_frame_graph(app::f32 time, app::f32 delta_time)
{
    static ScrollingBuffer data{};

    data.AddPoint(time, delta_time);

    static float history = 10.0f;

    static ImPlotAxisFlags flags = ImPlotAxisFlags_NoLabel | ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels;
    if (ImPlot::BeginPlot("##graph_frametime_cpu", ImVec2(-1, 150)))
    {
        ImPlot::SetupAxes(nullptr, nullptr, flags, flags);
        ImPlot::SetupAxisLimits(ImAxis_X1, time - history, time, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 1);
        ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL, 0.5f);
        ImPlot::PlotLine("Frame Time", &data.Data[0].x, &data.Data[0].y, data.Data.size(), 0, data.Offset, 2 * sizeof(float));
        ImPlot::EndPlot();
    }
}

namespace app::core
{
    f32 cam_move_speed = 0.1f;
    f32 cam_move_time = 10.0f;
    glm::vec3 cam_pos{ 32, 32, 0 };
    glm::vec3 cam_pos_target = cam_pos;

    f32 cam_zoom_speed = 10.0f;
    f32 cam_ortho_size = 32.0f;
    f32 cam_ortho_size_target = cam_ortho_size;
    f32 cam_max_zoom = 2.0f;
    f32 cam_min_zoom = 100.0f;

    void handle_camera_input(input::Input& input, f32 delta_time)
    {
        // Movement
        if (input.on_key_held(GLFW_KEY_W))
        {
            cam_pos_target.y -= cam_move_speed;
        }
        if (input.on_key_held(GLFW_KEY_S))
        {
            cam_pos_target.y += cam_move_speed;
        }
        if (input.on_key_held(GLFW_KEY_D))
        {
            cam_pos_target.x += cam_move_speed;
        }
        if (input.on_key_held(GLFW_KEY_A))
        {
            cam_pos_target.x -= cam_move_speed;
        }
        cam_pos = glm::mix(cam_pos, cam_pos_target, delta_time * cam_move_time);

        // Zoom
        if (glm::abs(input.get_scroll_amount()) > 0.01f)
        {
            cam_ortho_size_target += -input.get_scroll_amount() * cam_zoom_speed;
            cam_ortho_size_target = glm::clamp(cam_ortho_size_target, cam_max_zoom, cam_min_zoom);
        }
        cam_ortho_size = glm::mix(cam_ortho_size, cam_ortho_size_target, delta_time * cam_move_time);
    }

    Application::Application(const ApplicationInfo& app_info) : m_appInfo(app_info)
    {
        s_Instance = this;
        init();
    }

    Application::~Application()
    {
        shutdown();
        s_Instance = nullptr;
    }

    auto Application::get() -> Application&
    {
        return *s_Instance;
    }

    void Application::run()
    {
        LOG_INFO("Current Directory: <{}>", std::filesystem::current_path().string());

        m_isRunning = true;

        m_world.set_tile_size(1.0f);
        m_world.set_world_size(64, 64);

        m_worldGenerator.set_world(m_world);

        m_worldRenderer.init(m_renderer);
        m_worldRenderer.set_world(m_world);

        // Main loop
        while (m_isRunning && !m_renderer.has_window_requested_close())
        {
            float time = get_time();
            m_deltaTime = time - m_lastFrameTime;
            m_lastFrameTime = time;

            m_fpsAccumulatedTime += m_deltaTime;
            m_fpsFrameCount++;
            if (m_fpsAccumulatedTime >= 1.0f)
            {
                m_fps = static_cast<u32>(m_fpsFrameCount / m_fpsAccumulatedTime);
                m_fpsFrameCount = 0;
                m_fpsAccumulatedTime = 0.0f;
            }

            m_input.new_frame();

            handle_camera_input(m_input, m_deltaTime);

            m_renderer.new_frame(cam_pos, cam_ortho_size);
            m_worldRenderer.render();

            /*m_batch2D.begin_batch();
            for (f32 y = -10.0f; y < 10.0f; y += 0.25f)
            {
                for (f32 x = -10.0f; x < 10.0f; x += 0.25f)
                {
                    glm::vec4 color = { (x + 10) / 20.0f, 0.2f, (y + 10) / 20.0f, 1.0f };
                    m_batch2D.draw_quad({ x, y }, { 0.2f, 0.2f }, color);
                }
            }
            m_batch2D.end_batch();
            m_batch2D.flush();*/

            static bool s_ImGuiShowDemo = false;
            ImGui::ShowDemoWindow(&s_ImGuiShowDemo);

            static bool s_ImPlotShowDemo = false;
            ImPlot::ShowDemoWindow(&s_ImPlotShowDemo);

            if (ImGui::Begin("Debug"))
            {
                ImGui::Text("Frame Time: %ims / %ifps", static_cast<u32>(m_deltaTime * 1000.0f), m_fps);
                ImGui::Text("Memory Usage (bytes): %i", GetAllocationMetrics().CurrentUsage());

                draw_cpu_frame_graph(get_time(), get_delta_time());
            }

            if (ImGui::Begin("Game"))
            {
                if (ImGui::CollapsingHeader("Camera"))
                {
                    ImGui::DragFloat("Move Speed", &cam_move_speed);
                    ImGui::DragFloat("Move Time", &cam_move_time);

                    if (ImGui::DragFloat2("Position", &cam_pos.x))
                    {
                        cam_pos_target = cam_pos;
                    }
                    ImGui::DragFloat("Zoom", &cam_ortho_size);
                }
            }

            if (ImGui::Begin("World"))
            {
                static i32 steps = 1;
                ImGui::DragInt("Steps", &steps, 1.0f, 1, 10);

                if (ImGui::Button("Generate"))
                {
                    m_worldGenerator.reset();
                    m_worldGenerator.generate(steps);
                    m_worldRenderer.force_rebuild();
                }

                if (ImGui::Button("Step"))
                {
                    m_worldGenerator.step();
                    m_worldRenderer.force_rebuild();
                }

                if (ImGui::Button("Reset"))
                {
                    m_worldGenerator.reset();
                    m_worldRenderer.force_rebuild();
                }

                ImGui::Separator();

                ImGui::Text("Rendering");

                ImGui::Text("Vertices: %i", m_worldRenderer.get_vertex_count());
                ImGui::Text("Triangles: %i", m_worldRenderer.get_triangle_count());
            }

            m_renderer.end_frame();
        }
    }

    void Application::exit()
    {
        m_isRunning = false;
    }

    auto Application::get_time() -> f32
    {
        return static_cast<f32>(glfwGetTime());
    }

    auto Application::get_delta_time() -> f32
    {
        return m_deltaTime;
    }

    void Application::init()
    {
        m_renderer.init();
        m_batch2D.init(m_renderer);

        m_input.init(m_renderer.get_window_handle());
    }

    void Application::shutdown()
    {
        m_input.shutdown();

        m_batch2D.shutdown();
        m_renderer.shutdown();

        g_isAppRunning = false;
    }

    auto Application::get_renderer() const -> const gfx::Renderer&
    {
        return m_renderer;
    }

}
