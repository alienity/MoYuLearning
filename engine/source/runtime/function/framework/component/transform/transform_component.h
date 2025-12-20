#pragma once

#include "runtime/core/math/moyu_math2.h"
#include "runtime/function/framework/component/component.h"
#include "runtime/function/framework/object/object.h"

#include <memory>
#include <string>

namespace MoYu
{
    class TransformComponent : public Component
    {
    public:
        TransformComponent() { m_component_name = "TransformComponent"; };

        void postLoadResource(std::weak_ptr<GObject> object, const std::string json_data) override;

        void save(ComponentDefinitionRes& out_component_res) override;

        virtual void markToErase() override {};

        virtual glm::float3 getPosition() const { return m_transform_buffer[m_current_index].getPosition(); }
        virtual glm::float3 getScale() const { return m_transform_buffer[m_current_index].getScale(); }
        virtual glm::quat   getRotation() const { return m_transform_buffer[m_current_index].getRotation(); }

        virtual void setPosition(const glm::float3& new_translation);
        virtual void setScale(const glm::float3& new_scale);
        virtual void setRotation(const glm::quat& new_rotation);
        virtual void setRotation(const glm::float3& new_eulerAngles);

        virtual const Transform& getTransformConst() const { return m_transform_buffer[m_current_index]; }

        // for editor
        virtual Transform& getTransform() { return m_transform_buffer[m_next_index]; }

        virtual const glm::float4x4 getMatrix() const { return ((Transform&)m_transform_buffer[m_current_index]).getMatrix(); }

        virtual const glm::float4x4 getMatrixWorld();
        
        // Get local transformation matrix (relative to parent node)
        virtual const glm::float4x4 getLocalMatrix() const { return ((Transform&)m_transform_buffer[m_current_index]).getMatrix(); }

        virtual const bool isMatrixDirty() const;

        void preTick(float delta_time) override;
        void tick(float delta_time) override;
        void lateTick(float delta_time) override;

        // New: Get the status of whether the world matrix needs to be updated
        virtual bool isWorldTransformDirty() const { return m_world_transform_dirty; }
        
        // New: Mark world transform as dirty
        virtual void markWorldTransformDirty();

    private:
        Transform m_transform_buffer[2];
        uint32_t  m_current_index {0};
        uint32_t  m_next_index {1};
        
        // Cache world matrix and dirty flag
        mutable glm::float4x4 m_matrix_world = MYMatrix4x4::Identity;
        mutable glm::float4x4 m_matrix_world_prev = MYMatrix4x4::Identity;
        mutable bool m_world_transform_dirty = true; // Whether the world matrix needs to be recalculated
        mutable bool m_world_transform_cache_valid = false; // Whether the world matrix cache is valid
        
        // Internal implementation for updating the world matrix
        void updateWorldMatrix() const;
    };
} // namespace MoYu