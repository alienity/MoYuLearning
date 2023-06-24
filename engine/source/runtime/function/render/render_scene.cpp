#include "runtime/function/render/render_scene.h"
#include "runtime/function/render/glm_wrapper.h"
#include "runtime/function/render/render_resource.h"

namespace MoYu
{
    void RenderScene::updateLight(SceneLight sceneLight, SceneTransform sceneTransform)
    {
        Matrix4x4 model_matrix = sceneTransform.m_transform_matrix;
        
        Vector3    scale;
        Quaternion rotation;
        Vector3    translation;
        model_matrix.decompose(scale, rotation, translation);

        Vector3 direction = rotation * Vector3::Forward;

        if (sceneLight.m_light_type == LightType::AmbientLight)
        {
            m_ambient_light.m_color = sceneLight.ambient_light.m_color;

            m_ambient_light.m_identifier = sceneLight.m_identifier;
            m_ambient_light.m_position   = translation;
        }
        else if (sceneLight.m_light_type == LightType::DirectionLight)
        {
            Matrix4x4 dirLightViewMat     = Matrix4x4::lookAt(translation, direction, Vector3::Up);
            Matrix4x4 dirLightProjMat     = Matrix4x4::createOrthographic(sceneLight.direction_light.m_shadow_bounds.x,
                                                                      sceneLight.direction_light.m_shadow_bounds.y,
                                                                      -sceneLight.direction_light.m_shadow_near_plane,
                                                                      -sceneLight.direction_light.m_shadow_far_plane);
            Matrix4x4 dirLightViewProjMat = dirLightProjMat * dirLightViewMat;

            m_directional_light.m_identifier           = sceneLight.m_identifier;
            m_directional_light.m_position             = translation;
            m_directional_light.m_shadow_view_proj_mat = dirLightViewProjMat;
            m_directional_light.m_color                = sceneLight.direction_light.m_color;
            m_directional_light.m_intensity            = sceneLight.direction_light.m_intensity;
            m_directional_light.m_shadowmap            = sceneLight.direction_light.m_shadowmap;
            m_directional_light.m_shadow_bounds        = sceneLight.direction_light.m_shadow_bounds;
            m_directional_light.m_shadow_near_plane    = sceneLight.direction_light.m_shadow_near_plane;
            m_directional_light.m_shadow_far_plane     = sceneLight.direction_light.m_shadow_far_plane;
            m_directional_light.m_shadowmap_size       = sceneLight.direction_light.m_shadowmap_size;
        }
        else if (sceneLight.m_light_type == LightType::PointLight)
        {
            int index_finded = -1;
            for (int i = 0; i < m_point_light_list.size(); i++)
            {
                if (m_point_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    index_finded = i;
                    break;
                }
            }
            if (index_finded == -1)
            {
                InternalPointLight internalPointLight = {};
                internalPointLight.m_identifier       = sceneLight.m_identifier;
                internalPointLight.m_position         = translation;
                internalPointLight.m_color            = sceneLight.point_light.m_color;
                internalPointLight.m_intensity        = sceneLight.point_light.m_intensity;
                internalPointLight.m_radius           = sceneLight.point_light.m_radius;
                m_point_light_list.push_back(internalPointLight);
            }
            else
            {
                m_point_light_list[index_finded].m_identifier = sceneLight.m_identifier;
                m_point_light_list[index_finded].m_position   = translation;
                m_point_light_list[index_finded].m_color      = sceneLight.point_light.m_color;
                m_point_light_list[index_finded].m_intensity  = sceneLight.point_light.m_intensity;
                m_point_light_list[index_finded].m_radius     = sceneLight.point_light.m_radius;
            }
        }
        else if (sceneLight.m_light_type == LightType::SpotLight)
        {
            int index_finded = -1;
            for (int i = 0; i < m_spot_light_list.size(); i++)
            {
                if (m_spot_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    index_finded = i;
                    break;
                }
            }

            Matrix4x4 spotLightViewMat    = Matrix4x4::lookAt(translation, direction, Vector3::Up);
            Matrix4x4 spotLightProjMat    = Matrix4x4::createPerspective(sceneLight.spot_light.m_shadow_bounds.x,
                                                                      sceneLight.spot_light.m_shadow_bounds.y,
                                                                      -sceneLight.spot_light.m_shadow_near_plane,
                                                                      -sceneLight.spot_light.m_shadow_far_plane);
            Matrix4x4 spotLightViewProjMat = spotLightProjMat * spotLightViewMat;

            if (index_finded == -1)
            {
                InternalSpotLight internalSpotLight      = {};
                internalSpotLight.m_identifier           = sceneLight.m_identifier;
                internalSpotLight.m_position             = translation;
                internalSpotLight.m_direction            = direction;
                internalSpotLight.m_shadow_view_proj_mat = spotLightViewProjMat;
                
                internalSpotLight.m_color                = sceneLight.spot_light.m_color;
                internalSpotLight.m_intensity            = sceneLight.spot_light.m_intensity;
                internalSpotLight.m_radius               = sceneLight.spot_light.m_radius;
                internalSpotLight.m_inner_radians        = sceneLight.spot_light.m_inner_radians;
                internalSpotLight.m_outer_radians        = sceneLight.spot_light.m_outer_radians;
                internalSpotLight.m_shadowmap            = sceneLight.spot_light.m_shadowmap;
                internalSpotLight.m_shadow_bounds        = sceneLight.spot_light.m_shadow_bounds;
                internalSpotLight.m_shadow_near_plane    = sceneLight.spot_light.m_shadow_near_plane;
                internalSpotLight.m_shadow_far_plane     = sceneLight.spot_light.m_shadow_far_plane;
                internalSpotLight.m_shadowmap_size       = sceneLight.spot_light.m_shadowmap_size;


                m_spot_light_list.push_back(internalSpotLight);
            }
            else
            {
                m_spot_light_list[index_finded].m_identifier           = sceneLight.m_identifier;
                m_spot_light_list[index_finded].m_position             = translation;
                m_spot_light_list[index_finded].m_direction            = direction;
                m_spot_light_list[index_finded].m_shadow_view_proj_mat = spotLightViewProjMat;

                m_spot_light_list[index_finded].m_color                = sceneLight.spot_light.m_color;
                m_spot_light_list[index_finded].m_intensity            = sceneLight.spot_light.m_intensity;
                m_spot_light_list[index_finded].m_radius               = sceneLight.spot_light.m_radius;
                m_spot_light_list[index_finded].m_inner_radians        = sceneLight.spot_light.m_inner_radians;
                m_spot_light_list[index_finded].m_outer_radians        = sceneLight.spot_light.m_outer_radians;
                m_spot_light_list[index_finded].m_shadowmap            = sceneLight.spot_light.m_shadowmap;
                m_spot_light_list[index_finded].m_shadow_bounds        = sceneLight.spot_light.m_shadow_bounds;
                m_spot_light_list[index_finded].m_shadow_near_plane    = sceneLight.spot_light.m_shadow_near_plane;
                m_spot_light_list[index_finded].m_shadow_far_plane     = sceneLight.spot_light.m_shadow_far_plane;
                m_spot_light_list[index_finded].m_shadowmap_size       = sceneLight.spot_light.m_shadowmap_size;
            }
        }
    }

    void RenderScene::updateMeshRenderer(SceneMeshRenderer sceneMeshRenderer, SceneTransform sceneTransform, std::shared_ptr<RenderResource> m_render_resource)
    {
        const Matrix4x4 model_matrix = sceneTransform.m_transform_matrix;
        const Matrix4x4 model_matrix_inverse = GLMUtil::inverseMat4x4(model_matrix);

        int mesh_finded = -1;
        std::vector<CachedMeshRenderer>& _mesh_renderers = m_mesh_renderers;
        for (int j = 0; j < _mesh_renderers.size(); j++)
        {
            if (_mesh_renderers[j].cachedSceneMeshrenderer.m_identifier == sceneMeshRenderer.m_identifier)
            {
                mesh_finded = j;
                break;
            }
        }
        if (mesh_finded == -1)
        {
            CachedMeshRenderer cachedMeshRenderer;
            m_render_resource->updateInternalMeshRenderer(
                sceneMeshRenderer, cachedMeshRenderer.cachedSceneMeshrenderer, cachedMeshRenderer.internalMeshRenderer);
            cachedMeshRenderer.internalMeshRenderer.model_matrix         = model_matrix;
            cachedMeshRenderer.internalMeshRenderer.model_matrix_inverse = model_matrix_inverse;

            _mesh_renderers.push_back(cachedMeshRenderer);
        }
        else
        {
            m_render_resource->updateInternalMeshRenderer(sceneMeshRenderer,
                                                          _mesh_renderers[mesh_finded].cachedSceneMeshrenderer,
                                                          _mesh_renderers[mesh_finded].internalMeshRenderer);
            _mesh_renderers[mesh_finded].internalMeshRenderer.model_matrix         = model_matrix;
            _mesh_renderers[mesh_finded].internalMeshRenderer.model_matrix_inverse = model_matrix_inverse;
        }
    }

    void RenderScene::updateCamera(SceneCamera sceneCamera, SceneTransform sceneTransform)
    {
        //const Matrix4x4 model_matrix = sceneTransform.m_transform_matrix;
        //const Matrix4x4 model_matrix_inverse = GLMUtil::inverseMat4x4(model_matrix);

        const Vector3 position = sceneTransform.m_position;
        const Quaternion rotation = sceneTransform.m_rotation;

        const Vector3 direction = rotation * Vector3::Forward;

        const Matrix4x4 viewMatrix = Matrix4x4::lookAt(position, direction, Vector3::Up);
        const Matrix4x4 viewMatrixInv = viewMatrix.inverse();

        Matrix4x4 projMatrix = Matrix4x4::Identity;
        if (sceneCamera.m_projType == CameraProjType::Perspective)
        {
            projMatrix = Matrix4x4::createPerspective(
                sceneCamera.m_width, sceneCamera.m_height, sceneCamera.m_znear, sceneCamera.m_zfar);
        }
        else
        {
            projMatrix = Matrix4x4::createOrthographic(
                sceneCamera.m_width, sceneCamera.m_height, sceneCamera.m_znear, sceneCamera.m_zfar);
        }

        m_camera.m_identifier = sceneCamera.m_identifier;
        m_camera.m_projType   = sceneCamera.m_projType;
        m_camera.m_position   = position;
        m_camera.m_rotation   = rotation;

        m_camera.m_width  = sceneCamera.m_width;
        m_camera.m_height = sceneCamera.m_height;
        m_camera.m_znear  = sceneCamera.m_znear;
        m_camera.m_zfar   = sceneCamera.m_zfar;
        m_camera.m_aspect = sceneCamera.m_width / sceneCamera.m_height;
        m_camera.m_fovY   = sceneCamera.m_fovY;

        m_camera.m_ViewMatrix    = viewMatrix;
        m_camera.m_ViewMatrixInv = viewMatrixInv;

        m_camera.m_ProjMatrix    = projMatrix;
        m_camera.m_ProjMatrixInv = GLMUtil::inverseMat4x4(projMatrix);

        m_camera.m_ViewProjMatrix    = projMatrix * viewMatrix;
        m_camera.m_ViewProjMatrixInv = m_camera.m_ProjMatrixInv * m_camera.m_ViewMatrixInv;

    }

    void RenderScene::removeLight(SceneLight sceneLight)
    {
        if (sceneLight.m_light_type == LightType::AmbientLight)
        {
            m_ambient_light = {};
        }
        else if (sceneLight.m_light_type == LightType::DirectionLight)
        {
            m_directional_light = {};
        }
        else if (sceneLight.m_light_type == LightType::PointLight)
        {
            for (int i = 0; i < m_point_light_list.size(); i++)
            {
                if (m_point_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    m_point_light_list.erase(m_point_light_list.begin() + i);
                    break;
                }
            }
        }
        else if (sceneLight.m_light_type == LightType::SpotLight)
        {
            for (int i = 0; i < m_spot_light_list.size(); i++)
            {
                if (m_spot_light_list[i].m_identifier == sceneLight.m_identifier)
                {
                    m_spot_light_list.erase(m_spot_light_list.begin() + i);
                    break;
                }
            }
        }

    }

    void RenderScene::removeMeshRenderer(SceneMeshRenderer sceneMeshRenderer)
    {
        std::vector<CachedMeshRenderer>& _mesh_renderers = m_mesh_renderers;
        for (int j = 0; j < _mesh_renderers.size(); j++)
        {
            if (_mesh_renderers[j].cachedSceneMeshrenderer.m_identifier == sceneMeshRenderer.m_identifier)
            {
                _mesh_renderers.erase(_mesh_renderers.begin() + j);
                break;
            }
        }
    }

    void RenderScene::removeCamera(SceneCamera sceneCamera)
    {
        m_camera = {};
    }
}
