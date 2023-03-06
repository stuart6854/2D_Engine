#pragma once

#include "core/core.hpp"

struct GLFWwindow;

namespace app::input
{
    class Input
    {
    public:
        Input();
        ~Input();

        /* Initialisation/Shutdown */

        void init(GLFWwindow* window);
        void shutdown();

        /* Commands */

        void new_frame();

        void set_key_state(i32 key, bool is_down);
        void set_btn_state(i32 btn, bool is_down);

        void set_cursor_pos(const glm::vec2& pos);
        void set_scroll_amount(f32 amount);

        /* Getters */

        bool on_key_down(i32 key) const;
        bool on_key_up(i32 key) const;
        bool on_key_held(i32 key) const;

        bool on_ms_btn_down(i32 btn) const;
        bool on_ms_btn_up(i32 btn) const;
        bool on_ms_btn_held(i32 btn) const;

        auto get_cursor_pos() const -> const glm::vec2&;
        auto get_cursor_delta() const -> glm::vec2;

        auto get_scroll_amount() const -> f32;

    private:
        struct InputPimpl;
        Owned<InputPimpl> m_pimpl = nullptr;
    };
}