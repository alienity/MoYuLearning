#pragma once

#include "runtime/core/math/moyu_math2.h"

#include <functional>
#include <vector>
#include <memory>
#include <array>


namespace MoYu
{
    // Camera types
    enum class CameraType {
        PERSPECTIVE,  // Perspective projection
        ORTHOGRAPHIC  // Orthographic projection
    };

    // Camera movement modes
    enum class CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    class Camera {
    public:
        // Constructor
        Camera(
            const glm::float3& position = glm::float3(0.0f, 0.0f, 3.0f),
            float yaw = -90.0f,  // Default facing -Z, so yaw=-90 degrees
            float pitch = 0.0f,
            float fov = 45.0f,
            float aspectRatio = 16.0f/9.0f,
            float nearPlane = 0.1f,
            float farPlane = 1000.0f
        ) : m_position(position),
            m_yaw(yaw),
            m_pitch(pitch),
            m_fov(fov),
            m_aspectRatio(aspectRatio),
            m_nearPlane(nearPlane),
            m_farPlane(farPlane),
            m_cameraType(CameraType::PERSPECTIVE),
            m_movementSpeed(2.5f),
            m_rotationSpeed(1.0f),
            m_mouseSensitivity(0.1f),
            m_zoomSpeed(0.5f),
            m_smoothFactor(0.1f),
            m_isSmoothEnabled(false)
        {
            updateCameraVectors();
        }

        // ======================
        // Basic property setting and getting
        // ======================

        void setPosition(const glm::float3& position) { 
            m_position = position; 
            updateViewMatrix();
        }
        
        const glm::float3& getPosition() const { return m_position; }
        
        void setEulerAngles(float yaw, float pitch, float roll = 0.0f) {
            m_yaw = yaw;
            m_pitch = glm::clamp(pitch, -89.99f, 89.99f);
            m_roll = roll;
            updateCameraVectors();
        }
        
        MoYu::EulerAngle getEulerAngles() const {
            return EulerAngle(m_yaw, m_pitch, m_roll); 
        }
        
        void setUpDirection(const glm::float3& up) { 
            m_worldUp = glm::normalize(up); 
            updateCameraVectors();
        }
        
        const glm::float3& getUpDirection() const { return m_worldUp; }
        
        void setMovementSpeed(float speed) { m_movementSpeed = speed; }
        float getMovementSpeed() const { return m_movementSpeed; }
        
        void setRotationSpeed(float speed) { m_rotationSpeed = speed; }
        float getRotationSpeed() const { return m_rotationSpeed; }
        
        void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }
        float getMouseSensitivity() const { return m_mouseSensitivity; }
        
        void setSmoothFactor(float factor) { 
            m_smoothFactor = glm::clamp(factor, 0.0f, 1.0f); 
        }
        
        float getSmoothFactor() const { return m_smoothFactor; }
        
        void enableSmoothing(bool enable) { m_isSmoothEnabled = enable; }
        bool isSmoothingEnabled() const { return m_isSmoothEnabled; }

        // ======================
        // Projection settings
        // ======================

        void setProjectionParameters(
            float fov, 
            float aspectRatio, 
            float nearPlane, 
            float farPlane
        ) {
            m_fov = fov;
            m_aspectRatio = aspectRatio;
            m_nearPlane = nearPlane;
            m_farPlane = farPlane;
            updateProjectionMatrix();
        }
        
        void setPerspective(float fov, float aspectRatio, float nearPlane, float farPlane) {
            m_cameraType = CameraType::PERSPECTIVE;
            setProjectionParameters(fov, aspectRatio, nearPlane, farPlane);
        }
        
        void setOrthographic(
            float left, float right, 
            float bottom, float top, 
            float nearPlane, float farPlane
        ) {
            m_cameraType = CameraType::ORTHOGRAPHIC;
            m_orthoLeft = left;
            m_orthoRight = right;
            m_orthoBottom = bottom;
            m_orthoTop = top;
            m_nearPlane = nearPlane;
            m_farPlane = farPlane;
            updateProjectionMatrix();
        }
        
        void setAspectRatio(float aspectRatio) {
            m_aspectRatio = aspectRatio;
            updateProjectionMatrix();
        }
        
        float getFOV() const { return m_fov; }
        float getAspectRatio() const { return m_aspectRatio; }
        float getNearPlane() const { return m_nearPlane; }
        float getFarPlane() const { return m_farPlane; }
        CameraType getCameraType() const { return m_cameraType; }

        // ======================
        // Camera controls
        // ======================

        /**
        * Process keyboard input for camera movement
        * 
        * @param direction Movement direction
        * @param deltaTime Frame time interval
        */
        void processKeyboard(CameraMovement direction, float deltaTime) {
            float velocity = m_movementSpeed * deltaTime;
            
            if (direction == CameraMovement::FORWARD)
                m_position += m_front * velocity;
            else if (direction == CameraMovement::BACKWARD)
                m_position -= m_front * velocity;
            else if (direction == CameraMovement::LEFT)
                m_position -= m_right * velocity;
            else if (direction == CameraMovement::RIGHT)
                m_position += m_right * velocity;
            else if (direction == CameraMovement::UP)
                m_position += m_worldUp * velocity;
            else if (direction == CameraMovement::DOWN)
                m_position -= m_worldUp * velocity;
                
            updateViewMatrix();
        }

        /**
        * Process mouse movement for camera rotation
        * 
        * @param xOffset Mouse X-axis offset
        * @param yOffset Mouse Y-axis offset
        * @param constrainPitch Whether to constrain pitch angle
        */
        void processMouseMovement(float xOffset, float yOffset, bool constrainPitch = true) {
            xOffset *= m_mouseSensitivity;
            yOffset *= m_mouseSensitivity;
            
            m_yaw += xOffset;
            m_pitch -= yOffset; // Note: Y-axis direction is typically opposite to screen coordinate system
            
            if (constrainPitch) {
                m_pitch = glm::clamp(m_pitch, -89.99f, 89.99f);
            }
            
            updateCameraVectors();
        }

        /**
        * Process mouse scrolling for camera zooming
        * 
        * @param yOffset Scroll offset
        */
        void processMouseScroll(float yOffset) {
            if (m_cameraType == CameraType::PERSPECTIVE) {
                // Modify FOV to achieve zooming
                m_fov -= yOffset * m_zoomSpeed;
                m_fov = glm::clamp(m_fov, 1.0f, 90.0f);
            } else {
                // In orthographic projection, modify the view range
                float zoomAmount = yOffset * 0.5f;
                m_orthoLeft -= zoomAmount;
                m_orthoRight += zoomAmount;
                m_orthoBottom -= zoomAmount;
                m_orthoTop += zoomAmount;
            }
            
            updateProjectionMatrix();
        }

        /**
        * Make the camera look at a specific target point
        * 
        * @param target Target point
        * @param up Custom up direction vector
        */
        void lookAt(const glm::float3& target, const glm::float3& up = glm::float3(0.0f, 1.0f, 0.0f)) {
            glm::float3 direction = glm::normalize(target - m_position);
            
            // Calculate Euler angles
            float yaw = glm::degrees(glm::atan(direction.x, -direction.z));
            float pitch = glm::degrees(glm::asin(glm::clamp(direction.y, -1.0f, 1.0f)));
            
            setEulerAngles(yaw, pitch);
            m_worldUp = glm::normalize(up);
            
            // Recalculate camera vectors
            updateCameraVectors();
        }

        /**
        * Set camera orientation from direction vector
        * 
        * @param direction Direction vector
        * @param up Up direction vector
        */
        void setDirection(const glm::float3& direction, const glm::float3& up = glm::float3(0.0f, 1.0f, 0.0f)) {
            glm::float3 dir = glm::normalize(direction);
            float yaw = glm::degrees(glm::atan(dir.x, -dir.z));
            float pitch = glm::degrees(glm::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
            
            setEulerAngles(yaw, pitch);
            m_worldUp = glm::normalize(up);
            updateCameraVectors();
        }

        // ======================
        // Camera vector getters
        // ======================

        const glm::float3& getFront() const { return m_front; }
        const glm::float3& getRight() const { return m_right; }
        const glm::float3& getUp() const { return m_up; }

        /**
        * Get camera forward vector (fast version)
        */
        glm::float3 getForward() const {
            MoYu::MYQuaternion::fastYawPitchToForward(m_yaw, m_pitch);
        }

        /**
        * Get camera right vector in world space
        */
        glm::float3 getWorldRight() const {
            glm::float3 worldUp(0.0f, 1.0f, 0.0f);
            return glm::normalize(glm::cross(getForward(), worldUp));
        }

        // ======================
        // Matrix getters
        // ======================

        const glm::mat4& getViewMatrix() const { 
            return m_viewMatrix; 
        }
        
        const glm::mat4& getProjectionMatrix() const { 
            return m_projectionMatrix; 
        }
        
        glm::mat4 getViewProjectionMatrix() const {
            return m_projectionMatrix * m_viewMatrix;
        }
        
        /**
        * Rebuild view matrix
        */
        void updateViewMatrix() {
            m_viewMatrix = MoYu::MYMatrix4x4::lookAtRH(m_position, m_position + m_front, m_up);
        }
        
        /**
        * Rebuild projection matrix
        */
        void updateProjectionMatrix() {
            if (m_cameraType == CameraType::PERSPECTIVE) {
                m_projectionMatrix = MoYu::MYMatrix4x4::perspectiveFOV(
                    glm::radians(m_fov),
                    m_aspectRatio,
                    m_nearPlane,
                    m_farPlane
                );
            } else {
                m_projectionMatrix = MoYu::MYMatrix4x4::orthographic(
                    m_orthoLeft, m_orthoRight,
                    m_orthoBottom, m_orthoTop,
                    m_nearPlane, m_farPlane
                );
            }
        }

        // ======================
        // Advanced features
        // ======================

        /**
        * Generate a ray from the camera to a screen point
        * 
        * @param normalizedScreenCoords Normalized screen coordinates [-1,1]
        * @return Ray in world space
        */
        Ray screenPointToRay(const glm::vec2& normalizedScreenCoords) const {
            // Convert normalized device coordinates to clip space
            glm::float4 clipCoords(normalizedScreenCoords.x, normalizedScreenCoords.y, -1.0f, 1.0f);
            
            // Convert to camera space
            glm::mat4 invProjection = glm::inverse(m_projectionMatrix);
            glm::float4 cameraCoords = invProjection * clipCoords;
            cameraCoords.z = -1.0f; // Set to near clipping plane
            cameraCoords.w = 0.0f;
            
            // Convert to world space
            glm::mat4 invView = glm::inverse(m_viewMatrix);
            glm::float4 worldCoords = invView * cameraCoords;
            glm::float3 rayDirection = glm::normalize(glm::float3(worldCoords));
            
            return Ray{ m_position, rayDirection };
        }
        
        /**
        * Get frustum planes
        */
        std::array<glm::float4, 6> getFrustumPlanes() const {
            std::array<glm::float4, 6> planes{};
            glm::mat4 viewProjection = m_projectionMatrix * m_viewMatrix;
            
            // Extract planes
            planes[0] = viewProjection[3] + viewProjection[0]; // Right
            planes[1] = viewProjection[3] - viewProjection[0]; // Left
            planes[2] = viewProjection[3] + viewProjection[1]; // Top
            planes[3] = viewProjection[3] - viewProjection[1]; // Bottom
            planes[4] = viewProjection[3] + viewProjection[2]; // Near
            planes[5] = viewProjection[3] - viewProjection[2]; // Far
            
            // Normalize
            for (auto& plane : planes) {
                glm::float3 normal(plane.x, plane.y, plane.z);
                float length = glm::length(normal);
                plane /= length;
            }
            
            return planes;
        }
        
        /**
        * Check if sphere is inside frustum
        */
        bool isSphereVisible(const BSphere& sphere) const {
            auto planes = getFrustumPlanes();
            
            for (const auto& plane : planes) {
                float distance = glm::dot(glm::float3(plane), sphere.center) + plane.w;
                if (distance < -sphere.radius) {
                    return false; // Sphere is outside the plane
                }
            }
            
            return true;
        }
        
        /**
        * Camera interpolation
        */
        void interpolate(const Camera& target, float t) {
            // Position interpolation
            m_position = glm::mix(m_position, target.m_position, t);
            
            // Orientation interpolation (using quaternions to avoid gimbal lock)
            glm::quat sourceQuat = glm::quat(glm::float3(
                glm::radians(m_pitch),
                glm::radians(m_yaw),
                glm::radians(m_roll)
            ));
            
            glm::quat targetQuat = glm::quat(glm::float3(
                glm::radians(target.m_pitch),
                glm::radians(target.m_yaw),
                glm::radians(target.m_roll)
            ));
            
            glm::quat resultQuat = glm::slerp(sourceQuat, targetQuat, t);
            glm::float3 eulerAngles = glm::eulerAngles(resultQuat);
            
            m_pitch = glm::degrees(eulerAngles.x);
            m_yaw = glm::degrees(eulerAngles.y);
            m_roll = glm::degrees(eulerAngles.z);
            
            // Constrain pitch angle
            m_pitch = glm::clamp(m_pitch, -89.99f, 89.99f);
            
            // Update camera vectors
            updateCameraVectors();
        }
        
        /**
        * Set camera to follow target
        * 
        * @param target Target position
        * @param offset Offset relative to target
        */
        void followTarget(const glm::float3& target, const glm::float3& offset) {
            // Calculate desired position
            glm::float3 desiredPosition = target + offset;
            
            if (m_isSmoothEnabled) {
                // Smooth following
                m_position = glm::mix(m_position, desiredPosition, m_smoothFactor);
            } else {
                // Immediate following
                m_position = desiredPosition;
            }
            
            updateViewMatrix();
        }

        // ======================
        // Debugging features
        // ======================

        /**
        * Get camera information (for debugging)
        */
        std::string getDebugInfo() const {
            char buffer[256];
            snprintf(buffer, sizeof(buffer),
                "Position: (%.2f, %.2f, %.2f)\n"
                "Angles: Yaw=%.2f, Pitch=%.2f, Roll=%.2f\n"
                "Front: (%.2f, %.2f, %.2f)\n"
                "FOV: %.2f, Aspect: %.2f",
                m_position.x, m_position.y, m_position.z,
                m_yaw, m_pitch, m_roll,
                m_front.x, m_front.y, m_front.z,
                m_fov, m_aspectRatio
            );
            return std::string(buffer);
        }

    private:
        // Camera attributes
        glm::float3 m_position;
        glm::float3 m_front;
        glm::float3 m_up;
        glm::float3 m_right;
        glm::float3 m_worldUp;
        
        float m_yaw;
        float m_pitch;
        float m_roll;
        
        // Projection parameters
        CameraType m_cameraType;
        float m_fov;
        float m_aspectRatio;
        float m_nearPlane;
        float m_farPlane;
        
        // Orthographic projection parameters
        float m_orthoLeft = -10.0f;
        float m_orthoRight = 10.0f;
        float m_orthoBottom = -10.0f;
        float m_orthoTop = 10.0f;
        
        // Speed and sensitivity
        float m_movementSpeed;
        float m_rotationSpeed;
        float m_mouseSensitivity;
        float m_zoomSpeed;
        
        // Smoothing parameters
        float m_smoothFactor;
        bool m_isSmoothEnabled;
        
        // Matrices
        glm::mat4 m_viewMatrix;
        glm::mat4 m_projectionMatrix;
        
        /**
        * Update camera front, right, and up vectors
        */
        void updateCameraVectors() {
            // Calculate front vector based on Euler angles
            float yawRad = glm::radians(m_yaw);
            float pitchRad = glm::radians(m_pitch);
            
            m_front.x = glm::cos(pitchRad) * glm::sin(yawRad);
            m_front.y = glm::sin(pitchRad);
            m_front.z = -glm::cos(pitchRad) * glm::cos(yawRad); // Negative sign because front is -Z
            m_front = glm::normalize(m_front);
            
            // Recalculate right and up vectors
            m_right = glm::normalize(glm::cross(m_front, m_worldUp));
            m_up = glm::normalize(glm::cross(m_right, m_front));
            
            // Update view matrix
            updateViewMatrix();
        }
    };
} // namespace MoYu
