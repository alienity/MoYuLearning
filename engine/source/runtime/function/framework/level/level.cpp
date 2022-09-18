#include "runtime/function/framework/level/level.h"

#include "runtime/core/base/macro.h"

#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/resource/res_type/common/level.h"

#include "runtime/engine.h"
#include "runtime/function/character/character.h"
#include "runtime/function/framework/object/object.h"

#include <limits>

namespace Pilot
{
    Level::~Level() { clear(); }

    void Level::clear()
    {
        m_current_active_character.reset();
        m_current_root_nodes.clear();
        m_gobjects.clear();
    }

    std::weak_ptr<GObject> Level::createObject(GObjectID parentID)
    {
        GObjectID object_id = ObjectIDAllocator::alloc();
        while (m_gobjects.find(object_id) != m_gobjects.end())
        {
            object_id = ObjectIDAllocator::alloc();
        }
        ASSERT(object_id != k_invalid_gobject_id);

        std::shared_ptr<GObject> gobject;
        try
        {
            gobject = std::make_shared<GObject>(object_id, shared_from_this());
            gobject->setParent(parentID);

            m_gobjects.emplace(object_id, gobject);
            return std::weak_ptr<GObject>(gobject);
        }
        catch (const std::bad_alloc&)
        {
            LOG_FATAL("cannot allocate memory for new gobject");
            return std::weak_ptr<GObject>();
        }
    }

    std::weak_ptr<GObject> Level::instantiateObject(ObjectInstanceRes& object_instance_res)
    {
        ASSERT(m_gobjects.find(object_instance_res.m_id) == m_gobjects.end());

        std::shared_ptr<GObject> gobject = std::make_shared<GObject>(shared_from_this());
        
        bool is_loaded = gobject->load(object_instance_res);
        if (is_loaded)
        {
            // add to the root nodes
            if (object_instance_res.m_parent_id == k_invalid_gobject_id)
                this->m_current_root_nodes.push_back(object_instance_res.m_id);

            m_gobjects.emplace(object_instance_res.m_id, gobject);
            return std::weak_ptr<GObject>(gobject);
        }
        else
        {
            LOG_ERROR("loading object " + object_instance_res.m_name + " failed");
            return std::weak_ptr<GObject>();
        }
    }

    bool Level::load(const std::string& level_res_url)
    {
        LOG_INFO("loading level: {}", level_res_url);

        m_level_res_url = level_res_url;

        LevelRes   level_res;
        const bool is_load_success = g_runtime_global_context.m_asset_manager->loadAsset(level_res_url, level_res);
        if (is_load_success == false)
        {
            return false;
        }

        for (ObjectInstanceRes& object_instance_res : level_res.m_objects)
        {
            instantiateObject(object_instance_res);
        }

        // create active character
        for (const auto& object_pair : m_gobjects)
        {
            std::shared_ptr<GObject> object = object_pair.second;
            if (object == nullptr)
                continue;

            if (level_res.m_character_name == object->getName())
            {
                m_current_active_character = std::make_shared<Character>(object);
                break;
            }
        }

        m_is_loaded = true;

        LOG_INFO("level load succeed");

        return true;
    }

    void Level::unload()
    {
        clear();
        LOG_INFO("unload level: {}", m_level_res_url);
    }

    bool Level::save()
    {
        LOG_INFO("saving level: {}", m_level_res_url);
        LevelRes output_level_res;

        const size_t                    object_cout    = m_gobjects.size();
        std::vector<ObjectInstanceRes>& output_objects = output_level_res.m_objects;
        output_objects.resize(object_cout);

        size_t object_index = 0;
        for (const auto& id_object_pair : m_gobjects)
        {
            if (id_object_pair.second)
            {
                id_object_pair.second->save(output_objects[object_index]);
                ++object_index;
            }
        }

        const bool is_save_success =
            g_runtime_global_context.m_asset_manager->saveAsset(output_level_res, m_level_res_url);

        if (is_save_success == false)
        {
            LOG_ERROR("failed to save {}", m_level_res_url);
        }
        else
        {
            LOG_INFO("level save succeed");
        }

        return is_save_success;
    }

    void Level::tick(float delta_time)
    {
        if (!m_is_loaded)
        {
            return;
        }

        for (const auto& id_object_pair : m_gobjects)
        {
            assert(id_object_pair.second);
            if (id_object_pair.second)
            {
                id_object_pair.second->tick(delta_time);
            }
        }
        if (m_current_active_character && g_is_editor_mode == false)
        {
            m_current_active_character->tick(delta_time);
        }

    }

    std::weak_ptr<GObject> Level::getGObjectByID(GObjectID go_id) const
    {
        auto iter = m_gobjects.find(go_id);
        if (iter != m_gobjects.end())
        {
            return iter->second;
        }

        return std::weak_ptr<GObject>();
    }

    void Level::moveGObjectByID(GObjectID from_id, GObjectID to_parent_id)
    {

    }

    void Level::deleteGObjectByID(GObjectID go_id)
    {
        auto iter = m_gobjects.find(go_id);
        if (iter != m_gobjects.end())
        {
            std::shared_ptr<GObject> object = iter->second;
            if (object)
            {
                if (m_current_active_character && m_current_active_character->getObjectID() == object->getID())
                {
                    m_current_active_character->setObject(nullptr);
                }
            }
        }

        m_gobjects.erase(go_id);
    }

} // namespace Pilot
