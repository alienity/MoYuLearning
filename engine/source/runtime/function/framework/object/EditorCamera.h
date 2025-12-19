#pragma once

#include "Camera.h"

namespace MoYu
{
    class EditorCamera : public Camera {
    private:
        enum class EditorMode {
            FLY,
            ORBIT,
            PAN
        } editorMode = EditorMode::FLY;

        glm::float3 orbitCenter = glm::float3(0.0f);
        float orbitDistance = 5.0f;
        float orbitYaw = 45.0f;
        float orbitPitch = 30.0f;

    public:
        void setOrbitCenter(const glm::float3& center) {
            orbitCenter = center;
        }

        void setEditorMode(EditorMode mode) {
            editorMode = mode;

            if (mode == EditorMode::ORBIT) {
                // Calculate current distance to center
                orbitDistance = glm::distance(getPosition(), orbitCenter);

                // Calculate current angles
                glm::float3 direction = glm::normalize(orbitCenter - getPosition());
                orbitYaw = glm::degrees(glm::atan(direction.x, -direction.z));
                orbitPitch = glm::degrees(glm::asin(direction.y));
            }
        }

        void processEditorInput(float deltaTime, float xOffset, float yOffset, float scrollOffset) {
            if (editorMode == EditorMode::FLY) {
                // Standard fly camera control
                processMouseMovement(xOffset, yOffset);
            }
            else if (editorMode == EditorMode::ORBIT) {
                // Orbit camera control
                orbitYaw += xOffset * 0.5f;
                orbitPitch = glm::clamp(orbitPitch - yOffset * 0.5f, -89.0f, 89.0f);
                orbitDistance = glm::max(orbitDistance - scrollOffset, 0.1f);

                // Calculate new position
                float yawRad = glm::radians(orbitYaw);
                float pitchRad = glm::radians(orbitPitch);

                glm::float3 offset(
                    orbitDistance * glm::cos(pitchRad) * glm::sin(yawRad),
                    orbitDistance * glm::sin(pitchRad),
                    -orbitDistance * glm::cos(pitchRad) * glm::cos(yawRad)
                );

                setPosition(orbitCenter + offset);
                lookAt(orbitCenter);
            }
        }

        // Add missing functions that were referenced in camera_component.cpp
        void setPosition(const glm::float3& position) {
            Camera::setPosition(position);
        }

        const glm::float3& getPosition() const {
            return Camera::getPosition();
        }

        void setFront(const glm::float3& front) {
            // This would typically be implemented in the base Camera class
            // For now, we'll provide a minimal implementation
        }

        void setUp(const glm::float3& up) {
            setUpDirection(up);
        }

        const glm::float3& getFront() const {
            return Camera::getFront();
        }

        glm::mat4 getViewMatrix() const {
            return Camera::getViewMatrix();
        }

        void setWorldUp(const glm::float3& worldUp) {
            setUpDirection(worldUp);
        }

        void setYaw(float yaw) {
            setEulerAngles(yaw, getEulerAngles().pitch, getEulerAngles().roll);
        }

        void setPitch(float pitch) {
            setEulerAngles(getEulerAngles().yaw, pitch, getEulerAngles().roll);
        }

        float getYaw() const {
            return getEulerAngles().yaw;
        }

        float getPitch() const {
            return getEulerAngles().pitch;
        }

        void processKeyboard(CameraMovement direction, float deltaTime) {
            Camera::processKeyboard(direction, deltaTime);
        }

        void processMouseMovement(float xOffset, float yOffset) {
            Camera::processMouseMovement(xOffset, yOffset);
        }

        void processMouseScroll(float offset) {
            Camera::processMouseScroll(offset);
        }

        void lookAt(const glm::float3& target) {
            Camera::lookAt(target);
        }

        void setMovementSpeed(float speed) {
            Camera::setMovementSpeed(speed);
        }
    };
} // namespace MoYu