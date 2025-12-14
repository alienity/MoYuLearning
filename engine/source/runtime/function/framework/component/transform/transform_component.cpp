#include "runtime/function/framework/component/transform/transform_component.h"
#include "runtime/resource/res_type/components/transform.h"
#include "runtime/resource/asset_manager/asset_manager.h"
#include "runtime/engine.h"

namespace MoYu
{
    void TransformComponent::postLoadResource(std::weak_ptr<GObject> object, const std::string json_data)
    {
        m_object = object;

        TransformRes transform_res = AssetManager::loadJson<TransformRes>(json_data);

        Transform m_transform = {};

        m_transform.setPosition((&transform_res)->m_position);
        m_transform.setScale((&transform_res)->m_scale);
        m_transform.setRotation((&transform_res)->m_rotation);

        m_transform_buffer[m_current_index] = m_transform;
        m_transform_buffer[m_next_index]    = m_transform;

        markInit();
    }

    void TransformComponent::save(ComponentDefinitionRes& out_component_res)
    {
        TransformRes transform_res = {};
        (&transform_res)->m_position = m_transform_buffer[m_next_index].getPosition();
        (&transform_res)->m_scale    = m_transform_buffer[m_next_index].getScale();
        (&transform_res)->m_rotation = m_transform_buffer[m_next_index].getRotation();

        out_component_res.m_type_name = "TransformComponent";
        out_component_res.m_component_name = this->m_component_name;
        out_component_res.m_component_json_data = AssetManager::saveJson(transform_res);
    }

    void TransformComponent::setPosition(const glm::float3& new_translation)
    {
        m_transform_buffer[m_next_index].setPosition(new_translation);

        markDirty();
        markWorldTransformDirty();
    }

    void TransformComponent::setScale(const glm::float3& new_scale)
    {
        m_transform_buffer[m_next_index].setScale(new_scale);
        
        markDirty();
        markWorldTransformDirty();
    }

    void TransformComponent::setRotation(const glm::quat& new_rotation)
    {
        m_transform_buffer[m_next_index].setRotation(new_rotation);

        markDirty();
        markWorldTransformDirty();
    }

    void TransformComponent::setRotation(const glm::float3& new_eulerAngles)
    {
        m_transform_buffer[m_next_index].setRotation(new_eulerAngles);

        markDirty();
        markWorldTransformDirty();
    }

    const glm::float4x4 TransformComponent::getMatrixWorld()
    {
        // If the world matrix needs to be updated, then update it
        if (m_world_transform_dirty) 
        {
            updateWorldMatrix();
        }
        return m_matrix_world;
    }

    const bool TransformComponent::isMatrixDirty() const
    {
        return m_matrix_world_prev != m_matrix_world;
    }

    void TransformComponent::preTick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        m_matrix_world_prev = m_matrix_world;

        // Check if itself or parent node has changed, if so, update the world matrix
        if (m_world_transform_dirty)
        {
            updateWorldMatrix();
        }

        m_transform_buffer[m_current_index] = m_transform_buffer[m_next_index];

        std::swap(m_current_index, m_next_index);
    }

    void TransformComponent::tick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        //m_matrix_world_prev = m_matrix_world;

        //if (TransformComponent::isDirtyRecursively(this))
        //{
        //    // update transform component, dirty flag will be reset in mesh component
        //    UpdateWorldMatrixRecursively(this);
        //}

        //m_transform_buffer[m_current_index] = m_transform_buffer[m_next_index];

        //std::swap(m_current_index, m_next_index);

        //if (TransformComponent::isDirtyRecursively(this))
        //{
        //    // update transform component, dirty flag will be reset in mesh component
        //    //tryUpdateRigidBodyComponent();
        //}

        //if (g_is_editor_mode)
        //{
        //    m_transform_buffer[m_next_index] = m_transform;
        //}

        //markIdle();
    }
    
    void TransformComponent::lateTick(float delta_time)
    {
        if (m_object.expired() || this->isNone())
            return;

        markIdle();
    }

    void TransformComponent::markWorldTransformDirty()
    {
        if (m_world_transform_dirty) 
        {
            // Already marked as dirty, no need to mark again
            return;
        }
        
        m_world_transform_dirty = true;
        
        // Also mark all child nodes' world transform as dirty
        if (auto object = m_object.lock()) 
        {
            auto children = object->getChildren();
            for (auto& child : children) 
            {
                if (auto childTransform = child->getTransformComponent().lock()) 
                {
                    childTransform->markWorldTransformDirty();
                }
            }
        }
    }

    void TransformComponent::updateWorldMatrix() const
    {
        // If there's already a valid cache and not marked as dirty, return the cached value directly
        if (m_world_transform_cache_valid && !m_world_transform_dirty) 
        {
            return;
        }

        // Calculate local matrix
        glm::float4x4 localMatrix = ((Transform&)m_transform_buffer[m_current_index]).getMatrix();
        
        // If there's no parent object, the world matrix is the local matrix
        if (m_object.expired()) 
        {
            m_matrix_world = localMatrix;
        } 
        else 
        {
            auto object = m_object.lock();
            auto parent = object->getParent();
            
            // If there's no parent object, the world matrix is the local matrix
            if (!parent) 
            {
                m_matrix_world = localMatrix;
            } 
            else 
            {
                // Get the transform component of the parent object
                auto parentTransform = parent->getTransformComponent().lock();
                if (parentTransform) 
                {
                    // World matrix = Parent object's world matrix * Local matrix
                    m_matrix_world = parentTransform->getMatrixWorld() * localMatrix;
                } 
                else 
                {
                    m_matrix_world = localMatrix;
                }
            }
        }
        
        // Update cache status
        m_world_transform_dirty = false;
        m_world_transform_cache_valid = true;
    }

} // namespace MoYu