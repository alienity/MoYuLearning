#include "editor/include/editor_input_manager.h"

#include "editor/include/editor.h"
#include "editor/include/editor_global_context.h"
#include "editor/include/editor_scene_manager.h"

#include "runtime/engine.h"
#include "runtime/function/framework/level/level.h"
#include "runtime/function/framework/world/world_manager.h"
#include "runtime/function/global/global_context.h"
#include "runtime/function/input/input_system.h"

#include "runtime/function/render/render_camera.h"
#include "runtime/function/render/render_system.h"
#include "runtime/function/render/window_system.h"

#include <sstream>

namespace MoYu
{
    void EditorInputManager::initialize() { registerInput(); }

    void EditorInputManager::tick(float delta_time) { processEditorCommand(); }

    void EditorInputManager::registerInput()
    {
        g_editor_global_context.m_window_system->registerOnResetFunc(std::bind(&EditorInputManager::onReset, this));
        g_editor_global_context.m_window_system->registerOnCursorPosFunc(
            std::bind(&EditorInputManager::onCursorPos, this, std::placeholders::_1, std::placeholders::_2));
        g_editor_global_context.m_window_system->registerOnCursorEnterFunc(
            std::bind(&EditorInputManager::onCursorEnter, this, std::placeholders::_1));
        g_editor_global_context.m_window_system->registerOnScrollFunc(
            std::bind(&EditorInputManager::onScroll, this, std::placeholders::_1, std::placeholders::_2));
        g_editor_global_context.m_window_system->registerOnWindowCloseFunc(
            std::bind(&EditorInputManager::onWindowClosed, this));
        g_editor_global_context.m_window_system->registerOnKeyFunc(std::bind(&EditorInputManager::onKey,
                                                                             this,
                                                                             std::placeholders::_1,
                                                                             std::placeholders::_2,
                                                                             std::placeholders::_3,
                                                                             std::placeholders::_4));
    }

    void EditorInputManager::updateCursorOnAxis(glm::float2 cursor_uv)
    {
        //if (g_editor_global_context.m_scene_manager->getEditorCamera())
        //{
        //    glm::float2 window_size(m_engine_window_size.x, m_engine_window_size.y);
        //    m_cursor_on_axis = g_editor_global_context.m_scene_manager->updateCursorOnAxis(cursor_uv, window_size);
        //}
    }

    void EditorInputManager::processEditorCommand()
    {
        float           camera_speed  = m_camera_speed;
        std::shared_ptr editor_camera = g_editor_global_context.m_scene_manager->getEditorCamera();
        glm::quat       camera_rotate = glm::inverse(editor_camera->rotation());
        glm::float3     camera_relative_pos(0, 0, 0);

        if ((unsigned int)EditorCommand::camera_foward & m_editor_command)
        {
            camera_relative_pos += camera_rotate * glm::float3 {0, 0, -camera_speed};
        }
        if ((unsigned int)EditorCommand::camera_back & m_editor_command)
        {
            camera_relative_pos += camera_rotate * glm::float3 {0, 0, camera_speed};
        }
        if ((unsigned int)EditorCommand::camera_left & m_editor_command)
        {
            camera_relative_pos += camera_rotate * glm::float3 {-camera_speed, 0, 0};
        }
        if ((unsigned int)EditorCommand::camera_right & m_editor_command)
        {
            camera_relative_pos += camera_rotate * glm::float3 {camera_speed, 0, 0};
        }
        if ((unsigned int)EditorCommand::camera_up & m_editor_command)
        {
            camera_relative_pos += glm::float3 {0, camera_speed, 0};
        }
        if ((unsigned int)EditorCommand::camera_down & m_editor_command)
        {
            camera_relative_pos += glm::float3 {0, -camera_speed, 0};
        }
        if ((unsigned int)EditorCommand::delete_object & m_editor_command)
        {
            g_editor_global_context.m_scene_manager->onDeleteSelectedGObject();
        }

        editor_camera->move(camera_relative_pos);
    }

    void EditorInputManager::onKeyInEditorMode(int key, int scancode, int action, int mods)
    {
        if (!isCursorInRect(m_engine_window_pos, m_engine_window_size))
            return;

        if (action == GLFW_PRESS)
        {
            switch (key)
            {
                case GLFW_KEY_A:
                    m_editor_command |= (unsigned int)EditorCommand::camera_left;
                    break;
                case GLFW_KEY_S:
                    m_editor_command |= (unsigned int)EditorCommand::camera_back;
                    break;
                case GLFW_KEY_W:
                    m_editor_command |= (unsigned int)EditorCommand::camera_foward;
                    break;
                case GLFW_KEY_D:
                    m_editor_command |= (unsigned int)EditorCommand::camera_right;
                    break;
                case GLFW_KEY_E:
                    m_editor_command |= (unsigned int)EditorCommand::camera_up;
                    break;
                case GLFW_KEY_Q:
                    m_editor_command |= (unsigned int)EditorCommand::camera_down;
                    break;
                case GLFW_KEY_T:
                    m_editor_command |= (unsigned int)EditorCommand::translation_mode;
                    break;
                case GLFW_KEY_R:
                    m_editor_command |= (unsigned int)EditorCommand::rotation_mode;
                    break;
                case GLFW_KEY_C:
                    m_editor_command |= (unsigned int)EditorCommand::scale_mode;
                    break;
                case GLFW_KEY_DELETE:
                    m_editor_command |= (unsigned int)EditorCommand::delete_object;
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
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::exit);
                    break;
                case GLFW_KEY_A:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_left);
                    break;
                case GLFW_KEY_S:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_back);
                    break;
                case GLFW_KEY_W:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_foward);
                    break;
                case GLFW_KEY_D:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_right);
                    break;
                case GLFW_KEY_E:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_up);
                    break;
                case GLFW_KEY_Q:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::camera_down);
                    break;
                case GLFW_KEY_T:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::translation_mode);
                    break;
                case GLFW_KEY_R:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::rotation_mode);
                    break;
                case GLFW_KEY_C:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::scale_mode);
                    break;
                case GLFW_KEY_DELETE:
                    m_editor_command &= (k_complement_control_command ^ (unsigned int)EditorCommand::delete_object);
                    break;
                default:
                    break;
            }
        }
    }

    void EditorInputManager::onKey(int key, int scancode, int action, int mods)
    {
        if (g_is_editor_mode)
        {
            onKeyInEditorMode(key, scancode, action, mods);
        }
    }

    void EditorInputManager::onReset()
    {
        // to do
    }

    void EditorInputManager::onCursorPos(double xpos, double ypos)
    {
        if (!g_is_editor_mode)
            return;

        float angularVelocity =
            180.0f / MOYU_MAX(m_engine_window_size.x, m_engine_window_size.y); // 180 degrees while moving full screen
        if (m_mouse_x >= 0.0f && m_mouse_y >= 0.0f)
        {
            if (isCursorInRect(m_engine_window_pos, m_engine_window_size))
            {
                if (g_editor_global_context.m_window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
                {
                    glfwSetInputMode(
                        g_editor_global_context.m_window_system->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    
                    float xoffset = xpos - m_mouse_x;
                    float yoffset = ypos - m_mouse_y; // reversed since y-coordinates range from bottom to top
                    xoffset *= angularVelocity;
                    yoffset *= angularVelocity;

                    auto _camera = g_editor_global_context.m_scene_manager->getEditorCamera();
                    _camera->rotate(glm::float2(xoffset, yoffset));
                }
                else if (g_editor_global_context.m_window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_LEFT))
                {
                    /*
                    g_editor_global_context.m_scene_manager->moveEntity(
                        xpos,
                        ypos,
                        m_mouse_x,
                        m_mouse_y,
                        m_engine_window_pos,
                        m_engine_window_size,
                        m_cursor_on_axis,
                        g_editor_global_context.m_scene_manager->getSelectedObjectMatrix());
                    glfwSetInputMode(
                        g_editor_global_context.m_window_system->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                    */
                }
                else
                {
                    glfwSetInputMode(
                        g_editor_global_context.m_window_system->getWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);

                    if (isCursorInRect(m_engine_window_pos, m_engine_window_size))
                    {
                        glm::float2 cursor_uv = glm::float2((m_mouse_x - m_engine_window_pos.x) / m_engine_window_size.x,
                                                    (m_mouse_y - m_engine_window_pos.y) / m_engine_window_size.y);
                        updateCursorOnAxis(cursor_uv);
                    }
                }
            }
        }
        m_mouse_x = xpos;
        m_mouse_y = ypos;
    }

    void EditorInputManager::onCursorEnter(int entered)
    {
        if (!entered) // lost focus
        {
            m_mouse_x = m_mouse_y = -1.0f;
        }
    }

    void EditorInputManager::onScroll(double xoffset, double yoffset)
    {
        if (!g_is_editor_mode)
        {
            return;
        }

        if (isCursorInRect(m_engine_window_pos, m_engine_window_size))
        {
            if (g_editor_global_context.m_window_system->isMouseButtonDown(GLFW_MOUSE_BUTTON_RIGHT))
            {
                if (yoffset > 0)
                {
                    m_camera_speed *= 1.2f;
                }
                else
                {
                    m_camera_speed *= 0.8f;
                }
            }
            else
            {
                g_editor_global_context.m_scene_manager->getEditorCamera()->zoom(
                    (float)yoffset * 2.0f); // wheel scrolled up = zoom in by 2 extra degrees
            }
        }
    }

    void EditorInputManager::onWindowClosed() { g_editor_global_context.m_engine_runtime->shutdownEngine(); }

    bool EditorInputManager::isCursorInRect(glm::float2 pos, glm::float2 size) const
    {
        return pos.x <= m_mouse_x && m_mouse_x <= pos.x + size.x && pos.y <= m_mouse_y && m_mouse_y <= pos.y + size.y;
    }
} // namespace MoYu