#pragma once

#ifndef GLM_FORCE_RADIANS
#define GLM_FORCE_RADIANS 1
#endif

#ifndef GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 1
#endif

#include "runtime/core/math/moyu_math.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_clip_space.hpp>

#include <tuple>

namespace MoYu
{
    class GLMUtil
    {
    public:
        static Vector3   toVec3(glm::vec3 v) { return Vector3(v.x, v.y, v.z); }
        static glm::vec3 fromVec3(Vector3 v) { return glm::vec3(v.x, v.y, v.z); }

        static Vector4   toVec4(glm::vec4 v) { return Vector4(v.x, v.y, v.z, v.w); }
        static glm::vec4 fromVec4(Vector4 v) { return glm::vec4(v.x, v.y, v.z, v.w); }

        static Quaternion toQuat(glm::quat q) { return Quaternion(q.w, q.x, q.y, q.z); }
        static glm::quat  fromQuat(Quaternion q) { return glm::quat(q.w, q.x, q.y, q.z); }

        static Matrix4x4   toMat4x4(const glm::mat4x4& m);
        static glm::mat4x4 fromMat4x4(const Matrix4x4& m);

        static Matrix4x4 inverseMat4x4(const Matrix4x4& m);

        static std::tuple<Quaternion, Vector3, Vector3> decomposeMat4x4(const Matrix4x4& m);

        static Matrix4x4 lookAt(Vector3 eye, Vector3 gaze, Vector3 up);
        static Matrix4x4 perspectiveProj(float fovy, float aspect, float zNear, float zFar);
        static Matrix4x4 orthographicProj(float left, float right, float bottom, float top, float zNear, float zFar);

    };

    // hlsl��col-major�����Դ���shader��ʱ�򣬱���ת��
    inline Matrix4x4 GLMUtil::toMat4x4(const glm::mat4x4& m)
    {
        return Matrix4x4 {m[0][0],
                          m[1][0],
                          m[2][0],
                          m[3][0],
                          m[0][1],
                          m[1][1],
                          m[2][1],
                          m[3][1],
                          m[0][2],
                          m[1][2],
                          m[2][2],
                          m[3][2],
                          m[0][3],
                          m[1][3],
                          m[2][3],
                          m[3][3]};
    }

    inline glm::mat4x4 GLMUtil::fromMat4x4(const Matrix4x4& m)
    {
        return glm::mat4x4 {m[0][0],
                            m[1][0],
                            m[2][0],
                            m[3][0],
                            m[0][1],
                            m[1][1],
                            m[2][1],
                            m[3][1],
                            m[0][2],
                            m[1][2],
                            m[2][2],
                            m[3][2],
                            m[0][3],
                            m[1][3],
                            m[2][3],
                            m[3][3]};
    }

    inline Matrix4x4 GLMUtil::inverseMat4x4(const Matrix4x4& m)
    {
        const glm::mat4x4 _mat4 = fromMat4x4(m);
        const glm::mat4x4 _mat4_inverse = glm::inverse(_mat4);
        return toMat4x4(_mat4_inverse);
    }

    inline std::tuple<Quaternion, Vector3, Vector3> GLMUtil::decomposeMat4x4(const Matrix4x4& m)
    {
        glm::mat4 m_trans_glm = GLMUtil::fromMat4x4(m);

        glm::vec3 model_scale;
        glm::quat model_rotation;
        glm::vec3 model_translation;
        glm::vec3 model_skew;
        glm::vec4 model_perspective;
        glm::decompose(m_trans_glm, model_scale, model_rotation, model_translation, model_skew, model_perspective);

        Vector3    t = GLMUtil::toVec3(model_translation);
        Quaternion r = GLMUtil::toQuat(model_rotation);
        Vector3    s = GLMUtil::toVec3(model_scale);

        return std::tuple<Quaternion, Vector3, Vector3> {r, t, s};
    }

    inline Matrix4x4 GLMUtil::lookAt(Vector3 eye, Vector3 gaze, Vector3 up)
    {
        glm::vec3 _eye = GLMUtil::fromVec3(eye);
        glm::vec3 _center = _eye + GLMUtil::fromVec3(gaze);
        glm::vec3 _up = GLMUtil::fromVec3(up);
        Matrix4x4 viewMat = GLMUtil::toMat4x4(glm::lookAtRH(_eye, _center, _up));
        return viewMat;
    }

    inline Matrix4x4 GLMUtil::perspectiveProj(float fovy, float aspect, float zNear, float zFar)
    {
        glm::mat4 _perspectProj = glm::perspectiveRH_ZO(fovy, aspect, zNear, zFar);
        Matrix4x4 projMat = GLMUtil::toMat4x4(_perspectProj);
        return projMat;
    }

    inline Matrix4x4 GLMUtil::orthographicProj(float left, float right, float bottom, float top, float zNear, float zFar)
    {
        glm::mat4 _orthoProj = glm::orthoRH_ZO(left, right, bottom, right, zNear, zFar);
        Matrix4x4 projMat = GLMUtil::toMat4x4(_orthoProj);
        return projMat;
    }

} // namespace MoYu