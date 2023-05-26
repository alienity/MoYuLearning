#pragma once
#include "runtime/resource/res_type/common_serializer.h"

namespace MoYu
{
    struct DirectionalLightParameter
    {
        Color color = Color::White;
        float intensity {1.0f};
        
        bool    shadows {false};
        Vector2 shadow_bounds {512, 512};
        float   shadow_near_plane {0.1f};
        float   shadow_far_plane {500.0f};
        Vector2 shadowmap_size {512, 512};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DirectionalLightParameter,
                                       color,
                                       intensity,
                                       shadows,
                                       shadow_bounds,
                                       shadow_near_plane,
                                       shadow_far_plane,
                                       shadowmap_size)

    struct PointLightParameter
    {
        Color color = Color::White;
        float intensity {1.0f};

        float falloff_radius {5.0f};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(PointLightParameter, color, intensity, falloff_radius)

    struct SpotLightParameter
    {
        Color color = Color::White;
        float intensity {1.0f};

        float falloff_radius {5.0f};
        float inner_angle {45.0f};
        float outer_angle {60.0f};

        bool    shadows {false};
        Vector2 shadow_bounds {32, 32};
        float   shadow_near_plane {0.1f};
        float   shadow_far_plane {32.0f};
        Vector2 shadowmap_size {256, 256};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SpotLightParameter,
                                       color,
                                       intensity,
                                       falloff_radius,
                                       inner_angle,
                                       outer_angle,
                                       shadows,
                                       shadow_bounds,
                                       shadow_near_plane,
                                       shadow_far_plane,
                                       shadowmap_size)

    struct LightComponentRes
    {
        std::string               m_LightParamName {""};
        DirectionalLightParameter m_DirectionLightParam {};
        PointLightParameter       m_PointLightParam {};
        SpotLightParameter        m_SpotLightParam {};
    };
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LightComponentRes,
                                       m_LightParamName,
                                       m_DirectionLightParam,
                                       m_PointLightParam,
                                       m_SpotLightParam)

} // namespace MoYu