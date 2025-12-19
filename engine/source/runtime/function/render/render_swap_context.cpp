#include "runtime/function/render/render_swap_context.h"

#include <utility>

namespace MoYu
{
    void GameObjectResourceDesc::add(GameObjectDesc desc) { m_game_object_descs.push_back(desc); }

    bool GameObjectResourceDesc::isEmpty() const { return m_game_object_descs.empty(); }

    GameObjectDesc GameObjectResourceDesc::getNextProcessObject()
    {
        if (!m_game_object_descs.empty())
        {
            return m_game_object_descs.front();
        }
        else
        {
            return GameObjectDesc();
        }
    }

    void GameObjectResourceDesc::popProcessObject() { m_game_object_descs.pop_front(); }

    RenderSwapData& RenderSwapContext::getLogicSwapData() { 
        std::lock_guard<std::mutex> lock(m_swap_mutex);
        return m_swap_data[m_logic_swap_data_index]; 
    }

    RenderSwapData& RenderSwapContext::getRenderSwapData() { 
        std::lock_guard<std::mutex> lock(m_swap_mutex);
        return m_swap_data[m_render_swap_data_index]; 
    }

    void RenderSwapContext::swapLogicRenderData()
    {
        std::lock_guard<std::mutex> lock(m_swap_mutex);
        if (isReadyToSwap())
        {
            swap();
        }
    }

    bool RenderSwapContext::isReadyToSwap() const
    {
        return !(m_swap_data[m_render_swap_data_index].m_game_object_resource_desc.has_value() ||
                 m_swap_data[m_render_swap_data_index].m_game_object_to_delete.has_value() ||
                 m_swap_data[m_render_swap_data_index].m_camera_swap_data.has_value());
    }

    void RenderSwapContext::resetGameObjectResourceSwapData()
    {
        std::lock_guard<std::mutex> lock(m_swap_mutex);
        m_swap_data[m_render_swap_data_index].m_game_object_resource_desc.reset();
    }

    void RenderSwapContext::resetGameObjectToDelete()
    {
        std::lock_guard<std::mutex> lock(m_swap_mutex);
        m_swap_data[m_render_swap_data_index].m_game_object_to_delete.reset();
    }

    void RenderSwapContext::resetCameraSwapData()
    {
        std::lock_guard<std::mutex> lock(m_swap_mutex);
        m_swap_data[m_render_swap_data_index].m_camera_swap_data.reset();
    }

    void RenderSwapContext::swap()
    {
        resetGameObjectResourceSwapData();
        resetGameObjectToDelete();
        resetCameraSwapData();
        std::swap(m_logic_swap_data_index, m_render_swap_data_index);
    }

    void RenderSwapData::addDirtyGameObject(GameObjectDesc desc)
    {
        if (m_game_object_resource_desc.has_value())
        {
            m_game_object_resource_desc->add(desc);
        }
        else
        {
            GameObjectResourceDesc go_descs;
            go_descs.add(desc);
            m_game_object_resource_desc = go_descs;
        }
    }

    void RenderSwapData::addDeleteGameObject(GameObjectDesc desc)
    {
        if (m_game_object_to_delete.has_value())
        {
            m_game_object_to_delete->add(desc);
        }
        else
        {
            GameObjectResourceDesc go_descs;
            go_descs.add(desc);
            m_game_object_to_delete = go_descs;
        }
    }
} // namespace MoYu