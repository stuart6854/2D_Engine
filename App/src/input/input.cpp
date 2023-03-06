#include "input.hpp"

#include <GLFW/glfw3.h>

#include <imgui/imgui_impl_glfw.h>

namespace app::input
{
    struct Input::InputPimpl
    {
        GLFWwindow* window = nullptr;

        struct State
        {
            std::array<bool, GLFW_KEY_LAST> keys = {};
            std::array<bool, GLFW_MOUSE_BUTTON_LAST> btns = {};
            glm::vec2 cursorPos = {};
            f32 scrollAmount = 0.0f;
        };

        State currentState{};
        State lastState{};
    };

    Input::Input() : m_pimpl(new InputPimpl){};

    Input::~Input() = default;

    void Input::init(GLFWwindow* window)
    {
        glfwSetWindowUserPointer(window, this);

        glfwSetKeyCallback(window,
                           [](GLFWwindow* window, i32 key, i32 /*scancode*/, i32 action, i32 /*mods*/)
                           {
                               if (key == GLFW_KEY_UNKNOWN)
                               {
                                   return;
                               }

                               auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                               input->set_key_state(key, action != GLFW_RELEASE);
                           });

        glfwSetMouseButtonCallback(window,
                                   [](GLFWwindow* window, i32 btn, i32 action, i32 /*mods*/)
                                   {
                                       auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                                       input->set_btn_state(btn, action != GLFW_RELEASE);
                                   });

        glfwSetCursorPosCallback(window,
                                 [](GLFWwindow* window, f64 x, f64 y)
                                 {
                                     auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                                     input->set_cursor_pos({ x, y });
                                 });

        glfwSetScrollCallback(window,
                              [](GLFWwindow* window, f64 /*xOffset*/, f64 yOffset)
                              {
                                  auto* input = static_cast<Input*>(glfwGetWindowUserPointer(window));
                                  input->set_scroll_amount(static_cast<f32>(yOffset));
                              });

        ImGui_ImplGlfw_InitForVulkan(window, true);
    }

    void Input::shutdown() {}

    void Input::new_frame()
    {
        m_pimpl->lastState = m_pimpl->currentState;
        m_pimpl->currentState.scrollAmount = 0.0f;

        glfwPollEvents();
    }

    void Input::set_key_state(i32 key, bool is_down)
    {
        m_pimpl->currentState.keys[key] = is_down;
    }

    void Input::set_btn_state(i32 btn, bool is_down)
    {
        m_pimpl->currentState.btns[btn] = is_down;
    }

    void Input::set_cursor_pos(const glm::vec2& pos)
    {
        m_pimpl->currentState.cursorPos = pos;
    }

    void Input::set_scroll_amount(f32 amount)
    {
        m_pimpl->currentState.scrollAmount = amount;
    }

    bool Input::on_key_down(i32 key) const
    {
        const bool curr_state = m_pimpl->currentState.keys[key];
        const bool last_state = m_pimpl->lastState.keys[key];
        return curr_state && !last_state;
    }

    bool Input::on_key_up(i32 key) const
    {
        const bool curr_state = m_pimpl->currentState.keys[key];
        const bool last_state = m_pimpl->lastState.keys[key];
        return !curr_state && last_state;
    }

    bool Input::on_key_held(i32 key) const
    {
        const bool curr_state = m_pimpl->currentState.keys[key];
        const bool last_state = m_pimpl->lastState.keys[key];
        return curr_state && last_state;
    }

    bool Input::on_ms_btn_down(i32 btn) const
    {
        const bool curr_state = m_pimpl->currentState.btns[btn];
        const bool last_state = m_pimpl->lastState.btns[btn];
        return curr_state && !last_state;
    }

    bool Input::on_ms_btn_up(i32 btn) const
    {
        const bool curr_state = m_pimpl->currentState.btns[btn];
        const bool last_state = m_pimpl->lastState.btns[btn];
        return !curr_state && last_state;
    }

    bool Input::on_ms_btn_held(i32 btn) const
    {
        const bool curr_state = m_pimpl->currentState.btns[btn];
        const bool last_state = m_pimpl->lastState.btns[btn];
        return curr_state && last_state;
    }

    auto Input::get_cursor_pos() const -> const glm::vec2&
    {
        return m_pimpl->currentState.cursorPos;
    }

    auto Input::get_cursor_delta() const -> glm::vec2
    {
        return m_pimpl->lastState.cursorPos - m_pimpl->currentState.cursorPos;
    }

    auto Input::get_scroll_amount() const -> f32
    {
        return m_pimpl->currentState.scrollAmount;
    }

}
