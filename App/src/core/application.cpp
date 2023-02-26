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

namespace app::core
{
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

            glfwPollEvents();

            m_renderer.new_frame();

            m_batch2D.begin_batch();
            for (f32 y = -10.0f; y < 10.0f; y += 0.25f)
            {
                for (f32 x = -10.0f; x < 10.0f; x += 0.25f)
                {
                    glm::vec4 color = { (x + 10) / 20.0f, 0.2f, (y + 10) / 20.0f, 1.0f };
                    m_batch2D.draw_quad({ x, y }, { 0.2f, 0.2f }, color);
                }
            }
            m_batch2D.end_batch();
            m_batch2D.flush();

            static bool s_ImGuiShowDemo = false;
            ImGui::ShowDemoWindow(&s_ImGuiShowDemo);

            static bool s_ImPlotShowDemo = false;
            ImPlot::ShowDemoWindow(&s_ImPlotShowDemo);

            if (ImGui::Begin("Debug"))
            {
                ImGui::Text("Frame Time: %ims / %ifps", static_cast<u32>(m_deltaTime * 1000.0f), m_fps);
                ImGui::Text("Memory Usage (bytes): %i", GetAllocationMetrics().CurrentUsage());
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
        m_batch2D.init();
    }

    void Application::shutdown()
    {
        m_batch2D.shutdown();
        m_renderer.shutdown();

        g_isAppRunning = false;
    }

    auto Application::get_renderer() const -> const gfx::Renderer&
    {
        return m_renderer;
    }

}
