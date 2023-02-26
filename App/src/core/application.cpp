#include "application.hpp"

#include "core.hpp"
#include "rendering/renderer.hpp"

#include "imgui/imgui_impl_vulkan.h"
#include "imgui/imgui_impl_glfw.h"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>

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

        // ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        // ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        // ImGuiIO& io = ImGui::GetIO();

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

#if 0
        // Setup GLFW window
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
        {
            std::cerr << "Could not initalize GLFW!\n";
            return;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_windowHandle = glfwCreateWindow(m_appInfo.width, m_appInfo.height, m_appInfo.name.c_str(), NULL, NULL);

        // Setup Vulkan
        if (!glfwVulkanSupported())
        {
            std::cerr << "GLFW: Vulkan not supported!\n";
            return;
        }
        uint32_t extensions_count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&extensions_count);
        SetupVulkan(extensions, extensions_count);

        // Create Window Surface
        VkSurfaceKHR surface;
        VkResult err = glfwCreateWindowSurface(g_Instance, m_windowHandle, nullptr, &surface);
        check_vk_result(err);

        // Create Framebuffers
        int w, h;
        glfwGetFramebufferSize(m_windowHandle, &w, &h);
        ImGui_ImplVulkanH_Window* wd = &g_MainWindowData;
        SetupVulkanWindow(wd, surface, w, h);

        s_AllocatedCommandBuffers.resize(wd->ImageCount);
        s_ResourceFreeQueue.resize(wd->ImageCount);

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
        // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;    // Enable Docking
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;  // Enable Multi-Viewport / Platform Windows
        // io.ConfigViewportsNoAutoMerge = true;
        // io.ConfigViewportsNoTaskBarIcon = true;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();
        // ImGui::StyleColorsClassic();

        // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
        ImGuiStyle& style = ImGui::GetStyle();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(m_windowHandle, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = g_Instance;
        init_info.PhysicalDevice = g_PhysicalDevice;
        init_info.Device = g_Device;
        init_info.QueueFamily = g_QueueFamily;
        init_info.Queue = g_Queue;
        init_info.PipelineCache = g_PipelineCache;
        init_info.DescriptorPool = g_DescriptorPool;
        init_info.Subpass = 0;
        init_info.MinImageCount = g_MinImageCount;
        init_info.ImageCount = wd->ImageCount;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = check_vk_result;
        ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

        // Upload Fonts
        {
            // Use any command queue
            VkCommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;
            VkCommandBuffer command_buffer = wd->Frames[wd->FrameIndex].CommandBuffer;

            err = vkResetCommandPool(g_Device, command_pool, 0);
            check_vk_result(err);
            VkCommandBufferBeginInfo begin_info = {};
            begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            err = vkBeginCommandBuffer(command_buffer, &begin_info);
            check_vk_result(err);

            ImGui_ImplVulkan_CreateFontsTexture(command_buffer);

            VkSubmitInfo end_info = {};
            end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            end_info.commandBufferCount = 1;
            end_info.pCommandBuffers = &command_buffer;
            err = vkEndCommandBuffer(command_buffer);
            check_vk_result(err);
            err = vkQueueSubmit(g_Queue, 1, &end_info, VK_NULL_HANDLE);
            check_vk_result(err);

            err = vkDeviceWaitIdle(g_Device);
            check_vk_result(err);
            ImGui_ImplVulkan_DestroyFontUploadObjects();
        }
#endif
    }

    void Application::shutdown()
    {
        m_batch2D.shutdown();
        m_renderer.shutdown();

#if 0
        // Cleanup
        VkResult err = vkDeviceWaitIdle(g_Device);
        check_vk_result(err);

        // Free resources in queue
        for (auto& queue : s_ResourceFreeQueue)
        {
            for (auto& func : queue)
                func();
        }
        s_ResourceFreeQueue.clear();

        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        CleanupVulkanWindow();
        CleanupVulkan();

        glfwDestroyWindow(m_windowHandle);
        glfwTerminate();
#endif

        g_isAppRunning = false;
    }

    auto Application::get_renderer() const -> const gfx::Renderer&
    {
        return m_renderer;
    }

}
