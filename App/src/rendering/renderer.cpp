#include "renderer.hpp"

#include "device.hpp"
#include "shader.hpp"
#include "buffer.hpp"
#include "texture.hpp"

#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include <implot.h>

#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#define APP_ENABLE_IMGUI

namespace app::gfx
{
    static RenderMetrics s_renderMetrics{};

    auto Renderer::GetMetrics() -> const RenderMetrics&
    {
        return s_renderMetrics;
    }

    void Renderer::ResetMetrics()
    {
        s_renderMetrics = {};
    }

    struct Renderer::RendererPimpl
    {
        GLFWwindow* windowHandle = nullptr;

        Device device{};

        Shared<Shader> defaultShader = nullptr;
    };

    Renderer::Renderer() : m_pimpl(new RendererPimpl) {}

    Renderer::~Renderer()
    {
        shutdown();
    }

    void Renderer::init()
    {
        ASSERT(glfwInit() == GLFW_TRUE);

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_pimpl->windowHandle = glfwCreateWindow(1600, 900, "2D Engine", nullptr, nullptr);

        glfwSetWindowUserPointer(m_pimpl->windowHandle, this);

        auto native_window_handle = glfwGetWin32Window(m_pimpl->windowHandle);
        m_pimpl->device.init(native_window_handle);

#ifdef APP_ENABLE_IMGUI
        // Initialise ImGui
        {
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
            ImGui_ImplGlfw_InitForVulkan(m_pimpl->windowHandle, true);
            ImGui_ImplVulkan_InitInfo init_info = {};
            init_info.Instance = m_pimpl->device.get_instance();
            init_info.PhysicalDevice = m_pimpl->device.get_physical_device();
            init_info.Device = m_pimpl->device.get_device();
            init_info.QueueFamily = m_pimpl->device.get_graphics_family();
            init_info.Queue = m_pimpl->device.get_graphics_queue();
            init_info.DescriptorPool = m_pimpl->device.get_descriptor_pool();
            init_info.Subpass = 0;
            init_info.MinImageCount = 2;
            init_info.ImageCount = m_pimpl->device.get_swapchain_image_count();
            init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            init_info.Allocator = nullptr;
            init_info.CheckVkResultFn = nullptr;
            init_info.UseDynamicRendering = true;
            init_info.ColorAttachmentFormat = static_cast<VkFormat>(m_pimpl->device.get_swapchain_format());
            ImGui_ImplVulkan_Init(&init_info, nullptr);

            // Upload fonts

            auto cmd = m_pimpl->device.begin_single_use_cmd();

            ImGui_ImplVulkan_CreateFontsTexture(cmd);

            m_pimpl->device.end_single_use_cmd(cmd);
        }

        // Initialize ImPlot
        {
            ImPlot::CreateContext();
        }
#endif

        m_pimpl->defaultShader = create_shader();
        m_pimpl->defaultShader->init("../../assets/shaders/default.vert.spv", "../../assets/shaders/default.frag.spv");
    }

    void Renderer::shutdown()
    {
        if (!m_pimpl->device)
        {
            return;
        }

        m_pimpl->device.wait_idle();

        m_pimpl->defaultShader = nullptr;

#ifdef APP_ENABLE_IMGUI
        ImPlot::DestroyContext();
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif

        m_pimpl->device.shutdown();

        glfwDestroyWindow(m_pimpl->windowHandle);
        m_pimpl->windowHandle = nullptr;

        glfwTerminate();
    }

    bool Renderer::has_window_requested_close()
    {
        return glfwWindowShouldClose(m_pimpl->windowHandle);
    }

    auto Renderer::create_shader() const -> Shared<Shader>
    {
        return CreateShared<Shader>(&m_pimpl->device);
    }

    auto Renderer::create_buffer() const -> Shared<Buffer>
    {
        return CreateShared<Buffer>(&m_pimpl->device);
    }

    auto Renderer::create_texture() const -> Shared<Texture>
    {
        return CreateShared<Texture>(&m_pimpl->device);
    }

    void Renderer::new_frame()
    {
        s_renderMetrics.DrawCallCount = 0;
        s_renderMetrics.TriangleCount = 0;

        m_pimpl->device.new_frame();

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        m_pimpl->device.begin_backbuffer_pass({ 0.45f, 0.55f, 0.60f, 1.0f });

        auto cmd = m_pimpl->device.get_current_cmd();

        vk::Viewport viewport{};
        viewport.setWidth(1600);
        viewport.setHeight(900);
        cmd.setViewport(0, viewport);

        vk::Rect2D scissor{};
        scissor.setExtent({ 1600, 900 });
        cmd.setScissor(0, scissor);

        bind_shader(m_pimpl->defaultShader.get());

        const float aspect_ratio = 1600.0f / 900.0f;
        static const float ortho_size = 10.0f * 0.5f;
        glm::mat4 push_data[2];
        push_data[0] = glm::orthoLH_ZO(-ortho_size * aspect_ratio, ortho_size * aspect_ratio, -ortho_size, ortho_size, 0.0f, 1.0f);
        push_data[1] = glm::mat4(1.0f);

        set_push_constants(m_pimpl->defaultShader.get(), sizeof(glm::mat4) * 2, push_data);
    }

    void Renderer::end_frame()
    {
        auto cmd = m_pimpl->device.get_current_cmd();

        ImGui::Render();
        auto* imgui_draw_data = ImGui::GetDrawData();
        const bool is_window_minimized = imgui_draw_data->DisplaySize.x <= 0.0f || imgui_draw_data->DisplaySize.y <= 0.0f;
        if (!is_window_minimized)
            ImGui_ImplVulkan_RenderDrawData(imgui_draw_data, cmd);

        m_pimpl->device.end_backbuffer_pass();

        m_pimpl->device.flush_frame();
    }

    void Renderer::bind_shader(Shader* shader)
    {
        ASSERT(shader != nullptr && shader->is_valid());

        auto cmd = m_pimpl->device.get_current_cmd();

        cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, shader->get_pipeline());
    }

    void Renderer::bind_texture(Shader* shader, Texture* texture)
    {
        ASSERT(shader != nullptr && shader->is_valid());
        ASSERT(texture != nullptr && texture->is_valid());

        auto cmd = m_pimpl->device.get_current_cmd();

        auto layout = shader->get_layout();
        auto sets = { texture->get_set() };
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, layout, 0, sets, {});
    }

    void Renderer::set_push_constants(Shader* shader, u32 size, const void* data)
    {
        if (shader == nullptr || size == 0 || data == nullptr)
        {
            return;
        }

        auto cmd = m_pimpl->device.get_current_cmd();

        cmd.pushConstants(shader->get_layout(), vk::ShaderStageFlagBits::eVertex, 0, size, data);
    }

    void Renderer::draw_indexed(Buffer* vertex_buffer, Buffer* index_buffer, u32 index_count)
    {
        auto cmd = m_pimpl->device.get_current_cmd();
        cmd.bindVertexBuffers(0, vertex_buffer->get_buffer(), { 0 });
        cmd.bindIndexBuffer(index_buffer->get_buffer(), 0, vk::IndexType::eUint32);
        cmd.drawIndexed(index_count, 1, 0, 0, 0);

        s_renderMetrics.DrawCallCount++;
        s_renderMetrics.TriangleCount += index_count / 3;
    }

}
