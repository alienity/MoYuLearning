#include "runtime/function/input/input_system.h"

#include "runtime/core/base/macro.h"
#include "runtime/function/global/global_context.h"

#include "engine.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

#include <GLFW/glfw3.h>

// Simple key state tracking
// Using a reasonable size for key tracking (supporting keys 0-512)
static bool key_states[512] = {false};
static bool prev_key_states[512] = {false};

namespace MoYu
{
    unsigned int k_complement_control_command = 0xFFFFFFFF;

    void InputSystem::onKey(int key, int scancode, int action, int mods)
    {
        // Update key state tracking
        if (key >= 0 && key < 512) {
            if (action == GLFW_PRESS || action == GLFW_REPEAT) {
                key_states[key] = true;
            } else if (action == GLFW_RELEASE) {
                key_states[key] = false;
            }
        }

        if (!g_is_editor_mode)
        {
            onKeyInGameMode(key, scancode, action, mods);
        }
    }

    void InputSystem::onKeyInGameMode(int key, int scancode, int action, int mods)
    {
        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_ESCAPE:
                    // close();
                    break;
                case GLFW_KEY_R:
                    break;
                case GLFW_KEY_A:
                    m_game_command |= (unsigned int)GameCommand::left;
                    break;
                case GLFW_KEY_S:
                    m_game_command |= (unsigned int)GameCommand::backward;
                    break;
                case GLFW_KEY_W:
                    m_game_command |= (unsigned int)GameCommand::forward;
                    break;
                case GLFW_KEY_D:
                    m_game_command |= (unsigned int)GameCommand::right;
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                    m_game_command |= (unsigned int)GameCommand::squat;
                    break;
                case GLFW_KEY_LEFT_ALT: {
                    std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
                    window_system->setFocusMode(!window_system->getFocusMode());
                }
                break;
                case GLFW_KEY_LEFT_SHIFT:
                    m_game_command |= (unsigned int)GameCommand::sprint;
                    break;
                default:
                    break;
            }
        }
        else if (action == GLFW_RELEASE)
        {
            switch (key)
            {
                case GLFW_KEY_ESCAPE:
                    // close();
                    break;
                case GLFW_KEY_R:
                    break;
                case GLFW_KEY_W:
                    m_game_command &= (k_complement_control_command ^ (unsigned int)GameCommand::forward);
                    break;
                case GLFW_KEY_S:
                    m_game_command &= (k_complement_control_command ^ (unsigned int)GameCommand::backward);
                    break;
                case GLFW_KEY_A:
                    m_game_command &= (k_complement_control_command ^ (unsigned int)GameCommand::left);
                    break;
                case GLFW_KEY_D:
                    m_game_command &= (k_complement_control_command ^ (unsigned int)GameCommand::right);
                    break;
                case GLFW_KEY_LEFT_CONTROL:
                    m_game_command &= (k_complement_control_command ^ (unsigned int)GameCommand::squat);
                    break;
                case GLFW_KEY_LEFT_SHIFT:
                    m_game_command &= (k_complement_control_command ^ (unsigned int)GameCommand::sprint);
                    break;
                default:
                    break;
            }
        }
    }

    void InputSystem::onCursorPos(double current_cursor_x, double current_cursor_y)
    {
        if (g_runtime_global_context.m_window_system->getFocusMode())
        {
            m_cursor_delta_x = m_last_cursor_x - current_cursor_x;
            m_cursor_delta_y = m_last_cursor_y - current_cursor_y;
        }
        m_last_cursor_x = current_cursor_x;
        m_last_cursor_y = current_cursor_y;
    }

    void InputSystem::onMouseButton(int button, int action, int mods)
    {
        // Handle mouse button events if needed
    }

    void InputSystem::onScroll(double xoffset, double yoffset)
    {
        // Store scroll delta for use in other systems
        m_cursor_delta_scroll = static_cast<float>(yoffset);
    }

    void InputSystem::clear()
    {
        m_cursor_delta_x = 0;
        m_cursor_delta_y = 0;
        m_cursor_delta_scroll = 0; // Clear scroll delta
        
        // Copy current key states to previous key states
        for (int i = 0; i < 512; ++i) {
            prev_key_states[i] = key_states[i];
        }
    }

    void InputSystem::calculateCursorDeltaAngles()
    {
        std::array<int, 2> window_size = g_runtime_global_context.m_window_system->getWindowSize();

        if (window_size[0] < 1 || window_size[1] < 1)
        {
            return;
        }

        std::shared_ptr<RenderCamera> render_camera = g_runtime_global_context.m_render_system->getRenderCamera();

        const float& fovy = render_camera->fovy();
        const float  fovx = fovy * render_camera->asepct();

        float cursor_delta_x(MoYu::degreesToRadians(m_cursor_delta_x));
        float cursor_delta_y(MoYu::degreesToRadians(m_cursor_delta_y));

        m_cursor_delta_yaw   = (cursor_delta_x / (float)window_size[0]) * fovx;
        m_cursor_delta_pitch = -(cursor_delta_y / (float)window_size[1]) * fovy;
    }

    void InputSystem::initialize()
    {
        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
        ASSERT(window_system);

        window_system->registerOnKeyFunc(std::bind(&InputSystem::onKey,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2,
                                                   std::placeholders::_3,
                                                   std::placeholders::_4));
        window_system->registerOnCursorPosFunc(
            std::bind(&InputSystem::onCursorPos, this, std::placeholders::_1, std::placeholders::_2));
        
        // Register mouse button and scroll callbacks
        window_system->registerOnMouseButtonFunc(
            std::bind(&InputSystem::onMouseButton, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
        window_system->registerOnScrollFunc(
            std::bind(&InputSystem::onScroll, this, std::placeholders::_1, std::placeholders::_2));
        
        // Initialize key states
        for (int i = 0; i < 512; ++i) {
            key_states[i] = false;
            prev_key_states[i] = false;
        }
    }

    void InputSystem::tick()
    {
        calculateCursorDeltaAngles();
        clear();

        std::shared_ptr<WindowSystem> window_system = g_runtime_global_context.m_window_system;
        if (window_system->getFocusMode())
        {
            m_game_command &= (k_complement_control_command ^ (unsigned int)GameCommand::invalid);
        }
        else
        {
            m_game_command |= (unsigned int)GameCommand::invalid;
        }
    }

    bool InputSystem::isKeyPressing(KeyBoardButton key) const
    {
        int keyCode = static_cast<int>(key);
        return (keyCode >= 0 && keyCode < 512) ? key_states[keyCode] : false;
    }

    bool InputSystem::isKeyReleased(KeyBoardButton key) const
    {
        int keyCode = static_cast<int>(key);
        return (keyCode >= 0 && keyCode < 512) ? (!key_states[keyCode] && prev_key_states[keyCode]) : false;
    }
} // namespace MoYu