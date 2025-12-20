#pragma once

#include "runtime/core/math/moyu_math2.h"

namespace MoYu
{
    enum class LightType {
        DIRECTIONAL,
        POINT,
        SPOT
    };

    // Base light class
    class Light {
    protected:
        Color m_color = Color(1.0f, 1.0f, 1.0f, 1.0f);
        float m_intensity = 1.0f;
        bool m_shadowsEnabled = false;
        
    public:
        virtual ~Light() = default;
        
        // Get light type
        virtual LightType getType() const = 0;
        
        // Generic property getters/setters
        void setColor(const Color& color) { 
            m_color = color;
        }
        
        const Color& getColor() const { return m_color; }
        
        void setIntensity(float intensity) { m_intensity = intensity; }
        float getIntensity() const { return m_intensity; }
        
        void setShadowsEnabled(bool enabled) { m_shadowsEnabled = enabled; }
        bool isShadowsEnabled() const { return m_shadowsEnabled; }
    };

    // Directional light class
    class DirectionalLight : public Light {
    private:
        glm::vec3 m_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        float m_maxShadowDistance = 100.0f;
        
    public:
        LightType getType() const override { return LightType::DIRECTIONAL; }
        
        void setDirection(const glm::vec3& direction) { 
            m_direction = direction;
        }
        
        const glm::vec3& getDirection() const { return m_direction; }
        
        void setMaxShadowDistance(float distance) { m_maxShadowDistance = distance; }
        float getMaxShadowDistance() const { return m_maxShadowDistance; }
    };

    // Point light class
    class PointLight : public Light {
    private:
        glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        float m_radius = 10.0f;
        
    public:
        LightType getType() const override { return LightType::POINT; }
        
        void setPosition(const glm::vec3& position) { 
            m_position = position;
        }
        
        const glm::vec3& getPosition() const { return m_position; }
        
        void setRadius(float radius) { m_radius = radius; }
        float getRadius() const { return m_radius; }
    };

    // Spot light class
    class SpotLight : public Light {
    private:
        glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 0.0f);
        glm::vec3 m_direction = glm::vec3(0.0f, -1.0f, 0.0f);
        float m_spotAngle = 30.0f;
        float m_innerSpotPercent = 0.5f;
        
    public:
        LightType getType() const override { return LightType::SPOT; }
        
        void setPosition(const glm::vec3& position) { 
            m_position = position;
        }
        
        const glm::vec3& getPosition() const { return m_position; }
        
        void setDirection(const glm::vec3& direction) { 
            m_direction = direction;
        }
        
        const glm::vec3& getDirection() const { return m_direction; }
        
        void setSpotAngle(float angle) { m_spotAngle = angle; }
        float getSpotAngle() const { return m_spotAngle; }
        
        void setInnerSpotPercent(float percent) { m_innerSpotPercent = percent; }
        float getInnerSpotPercent() const { return m_innerSpotPercent; }
    };
} // namespace MoYu