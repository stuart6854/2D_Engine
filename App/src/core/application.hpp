#pragma once

#include <imgui.h>
#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <functional>

struct GLFWwindow;

namespace app::core
{
    struct ApplicationInfo
    {
        std::string name = "Application";
        uint32_t width = 1600;
        uint32_t height = 900;
    };

    class Application
    {
    public:
        Application(const ApplicationInfo& app_info = {});
        ~Application();

        static auto get() -> Application&;

        void run();

        void exit();

        auto get_time() -> float;

        auto get_window_handle() const
        {
            return m_windowHandle;
        }

        static auto get_instance() -> VkInstance;
        static auto get_physical_device() -> VkPhysicalDevice;
        static auto get_device() -> VkDevice;

    private:
        void init();
        void shutdown();

    private:
        ApplicationInfo m_appInfo{};
        GLFWwindow* m_windowHandle{};
        bool m_isRunning = false;

        float m_deltaTime = 0.0f;
        float m_frameTime = 0.0f;
        float m_lastFrameTime = 0.0f;
    };

}