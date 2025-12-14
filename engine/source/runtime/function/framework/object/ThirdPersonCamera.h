#pragma once

#include "Camera.h"

namespace MoYu
{
    class ThirdPersonCamera {
    private:
        Camera m_camera;
        glm::float3 m_targetPosition;
        float m_distance = 5.0f;
        float m_height = 2.0f;
        float m_rotation = 0.0f;

    public:
        void update(float deltaTime) {
            // Calculate camera position from target position and angle
            glm::float3 cameraOffset(
                glm::sin(glm::radians(m_rotation)) * m_distance,
                m_height,
                glm::cos(glm::radians(m_rotation)) * m_distance
            );

            // Set camera position and orientation
            m_camera.setPosition(m_targetPosition + cameraOffset);
            m_camera.lookAt(m_targetPosition);
        }

        void rotateAroundTarget(float degrees) {
            m_rotation += degrees;
            m_rotation = fmodf(m_rotation, 360.0f);
        }

        void zoom(float amount) {
            m_distance = glm::clamp(m_distance - amount, 1.0f, 20.0f);
        }

        void setTarget(const glm::float3& target) {
            m_targetPosition = target;
        }

        const Camera& getCamera() const { return m_camera; }
    };

} // namespace MoYu
