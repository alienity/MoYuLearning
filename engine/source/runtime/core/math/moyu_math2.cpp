#include "moyu_math2.h"

namespace MoYu
{
    //------------------------------------------------------------------------------------------

    /*
     * returns n! / d!
     */
    constexpr float factorial(size_t n, size_t d) {
        d = std::max(size_t(1), d);
        n = std::max(size_t(1), n);
        float r = 1.0;
        if (n == d) {
            // intentionally left blank
        }
        else if (n > d) {
            for (; n > d; n--) {
                r *= n;
            }
        }
        else {
            for (; d > n; d--) {
                r *= d;
            }
            r = 1.0f / r;
        }
        return r;
    }

    //------------------------------------------------------------------------------------------



    //------------------------------------------------------------------------------------------

    glm::float2 MYFloat2::Zero(0.f, 0.f);
    glm::float2 MYFloat2::One(1.f, 1.f);
    glm::float2 MYFloat2::UnitX(1.f, 0.f);
    glm::float2 MYFloat2::UnitY(0.f, 1.f);

    glm::float3 MYFloat3::Zero(0.f, 0.f, 0.f);
    glm::float3 MYFloat3::One(1.f, 1.f, 1.f);
    glm::float3 MYFloat3::UnitX(1.f, 0.f, 0.f);
    glm::float3 MYFloat3::UnitY(0.f, 1.f, 0.f);
    glm::float3 MYFloat3::UnitZ(0.f, 0.f, 1.f);
    glm::float3 MYFloat3::Up(0.f, 1.f, 0.f);
    glm::float3 MYFloat3::Down(0.f, -1.f, 0.f);
    glm::float3 MYFloat3::Right(1.f, 0.f, 0.f);
    glm::float3 MYFloat3::Left(-1.f, 0.f, 0.f);
    glm::float3 MYFloat3::Forward(0.f, 0.f, -1.f);
    glm::float3 MYFloat3::Backward(0.f, 0.f, 1.f);

    glm::float4 MYFloat4::Zero(0.f, 0.f, 0.f, 0.f);
    glm::float4 MYFloat4::One(1.f, 1.f, 1.f, 1.f);
    glm::float4 MYFloat4::UnitX(1.f, 0.f, 0.f, 0.f);
    glm::float4 MYFloat4::UnitY(0.f, 1.f, 0.f, 0.f);
    glm::float4 MYFloat4::UnitZ(0.f, 0.f, 1.f, 0.f);
    glm::float4 MYFloat4::UnitW(0.f, 0.f, 0.f, 1.f);

    glm::float3x3 MYMatrix3x3::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    glm::float3x3 MYMatrix3x3::Identity(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);

    glm::float4x4 MYMatrix4x4::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    glm::float4x4 MYMatrix4x4::Identity(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

    glm::quat MYQuaternion::Identity(1.f, 0.f, 0.f, 0.f);

    //------------------------------------------------------------------------------------------


    float MYFloat2::signedAngle(glm::float2 v1, glm::float2 v2)
    {
        glm::float2 n1 = glm::normalize(v1);
        glm::float2 n2 = glm::normalize(v2);

        float dot = glm::dot(n1, n2);
        if (dot > 1.0f)
            dot = 1.0f;
        if (dot < -1.0f)
            dot = -1.0f;

        float theta = glm::acos(dot);
        float sgn = glm::dot(glm::float2(-n1.y, n1.x), n2);
        if (sgn >= 0.0f)
            return theta;
        else
            return -theta;
    }

    glm::float2 MYFloat2::rotate(glm::float2 v, float theta)
    {
        float cs = glm::cos(theta);
        float sn = glm::sin(theta);
        float x1 = v.x * cs - v.y * sn;
        float y1 = v.x * sn + v.y * cs;
        return glm::float2(x1, y1);
    }

    //------------------------------------------------------------------------------------------

    namespace MYMatrix4x4
    {

        /**
         * @brief :  View Matrix
         *
         * Create view matrix from position and orientation
         *
         * @param position Position of the camera
         * @param orientation Orientation of the camera as quaternion
         * @return glm::float4x4 The resulting View Matrix
         */
        glm::float4x4 createViewMatrixFromQuaternionDirect(const glm::float3& position, const glm::quat& orientation) {
            // Conjugate the quaternion
            float qx = -orientation.x; // Negate vector part
            float qy = -orientation.y;
            float qz = -orientation.z;
            float qw = orientation.w;

            // Calculate products for rotation matrix
            float xx = qx * qx;
            float yy = qy * qy;
            float zz = qz * qz;
            float xy = qx * qy;
            float xz = qx * qz;
            float yz = qy * qz;
            float wx = qw * qx;
            float wy = qw * qy;
            float wz = qw * qz;

            // Initialize view matrix (3x3 rotation part)
            glm::float4x4 view(1.0f);

            // First column (X axis)
            view[0][0] = 1.0f - 2.0f * (yy + zz);
            view[0][1] = 2.0f * (xy - wz);
            view[0][2] = 2.0f * (xz + wy);

            // Second column (Y axis)
            view[1][0] = 2.0f * (xy + wz);
            view[1][1] = 1.0f - 2.0f * (xx + zz);
            view[1][2] = 2.0f * (yz - wx);

            // Third column (Z axis)
            view[2][0] = 2.0f * (xz - wy);
            view[2][1] = 2.0f * (yz + wx);
            view[2][2] = 1.0f - 2.0f * (xx + yy);

            // Translation part: -R^T * position
            glm::float3 translation(
                -(view[0][0] * position.x + view[1][0] * position.y + view[2][0] * position.z),
                -(view[0][1] * position.x + view[1][1] * position.y + view[2][1] * position.z),
                -(view[0][2] * position.x + view[1][2] * position.y + view[2][2] * position.z)
            );

            // Set translation component
            view[3][0] = translation.x;
            view[3][1] = translation.y;
            view[3][2] = translation.z;

            return view;
        }

        /**
         * @brief Right-handed coordinate system perspective projection matrix, Z range [0,1]
         *
         * Maps frustum to NDC space, depth range [0,1]
         * - Near plane (-n) maps to depth 0.0
         * - Far plane (-f) maps to depth 1.0
         *
         * @param fovY Vertical field of view (radians)
         * @param aspect Aspect ratio (width/height)
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Perspective projection matrix
         */
        glm::float4x4 perspectiveRH_ZO(float fovY, float aspect, float near, float far)
        {
            assert(aspect > 0.0f);
            assert(far > near && near > 0.0f);
            assert(fovY > 0.0f && fovY < MoYu::F_PI);

            float tanHalfFov = std::tan(fovY * 0.5f);

            glm::float4x4 proj(0.0f);

            // X-axis scaling factor
            proj[0][0] = 1.0f / (aspect * tanHalfFov);
            proj[0][1] = 0.0f;
            proj[0][2] = 0.0f;
            proj[0][3] = 0.0f;

            // Y-axis scaling factor
            proj[1][0] = 0.0f;
            proj[1][1] = 1.0f / tanHalfFov;
            proj[1][2] = 0.0f;
            proj[1][3] = 0.0f;
            
            // Z-axis transformation for depth range [0,1] from [-near, -far]
            proj[2][0] = 0.0f;
            proj[2][1] = 0.0f;
            proj[2][2] = far / (near - far);    // -(f/(f-n))
            proj[2][3] = -1.0f;                 // W_clip = -Z_view

            // Homogeneous w-coordinate transformation
            proj[3][0] = 0.0f;
            proj[3][1] = 0.0f;
            proj[3][2] = -(far * near) / (far - near); // B = (far*near)/(near-far)
            proj[3][3] = 0.0f;

            return proj;
        }

        /**
         * @brief Right-handed coordinate system perspective projection matrix with reversed Z depth range [0,1]
         *
         * Maps frustum to NDC space, with reversed depth range [1,0]
         * - Near plane (-n) maps to depth 1.0
         * - Far plane (-f) maps to depth 0.0
         *
         * @param fovY Vertical field of view (radians)
         * @param aspect Aspect ratio (width/height)
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Reversed Z perspective projection matrix
         */
        glm::float4x4 perspectiveRH_ZO_ReverseZ(float fovY, float aspect, float near, float far) {
            assert(aspect > 0.0f);
            assert(far > near && near > 0.0f);
            assert(fovY > 0.0f && fovY < MoYu::F_PI);

            float tanHalfFov = std::tan(fovY * 0.5f);
            
            glm::float4x4 proj(0.0f);

            // X-axis scaling factor
            proj[0][0] = 1.0f / (aspect * tanHalfFov);
            proj[0][1] = 0.0f;
            proj[0][2] = 0.0f;
            proj[0][3] = 0.0f;

            // Y-axis scaling factor
            proj[1][0] = 0.0f;
            proj[1][1] = 1.0f / tanHalfFov;
            proj[1][2] = 0.0f;
            proj[1][3] = 0.0f;

            // Z-axis transformation for reversed depth range [1,0] from [-near, -far]
            proj[2][0] = 0.0f;
            proj[2][1] = 0.0f;
            proj[2][2] = near / (far - near);    // A = near/(far-near)
            proj[2][3] = -1.0f;                  // W_clip = -Z_view


            // Homogeneous w-coordinate transformation
            proj[3][0] = 0.0f;
            proj[3][1] = 0.0f;
            proj[3][2] = (near * far) / (far - near); // B = (near*far)/(far-near)
            proj[3][3] = 0.0f;

            return proj;
        }

        /**
         * @brief Right-handed coordinate system perspective projection matrix, Z range [0,1]
         *
         * Maps frustum to NDC space, depth range [0,1]
         * - Near plane (-n) maps to depth 0.0
         * - Far plane (-f) maps to depth 1.0
         *
         * @param fovY Vertical field of view (radians)
         * @param aspect Aspect ratio (width/height)
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Perspective projection matrix
         */
        glm::float4x4 perspectiveFOV(float fovY, float aspect, float near, float far)
        {
#if MOYU_REVERSE_DEPTH
            return perspectiveRH_ZO_ReverseZ(fovY, aspect, near, far);
#else
            return perspectiveRH_ZO(fovY, aspect, near, far);
#endif
        }

        /**
         * @brief Right-handed coordinate system Off-Center perspective projection matrix, Z range [0,1]
         *
         * Maps asymmetric frustum to NDC space, depth range [0,1]
         * - Near plane (-n) maps to depth 0.0
         * - Far plane (-f) maps to depth 1.0
         *
         * @param left near plane left boundary
         * @param right near plane right boundary
         * @param bottom near plane bottom boundary
         * @param top near plane top boundary
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Off-Center perspective projection matrix
         */
        glm::float4x4 perspectiveOffCenterRH_ZO(float left, float right, float bottom, float top, float near, float far) {
            assert(near > 0.0f && far > near);
            assert(right != left && top != bottom);

            glm::float4x4 proj(1.0f);

            // X-axis scaling factor
            proj[0][0] = (2.0f * near) / (right - left);
            proj[0][1] = 0.0f;
            proj[0][2] = 0.0f;
            proj[0][3] = 0.0f;

            // Y-axis scaling factor
            proj[1][0] = 0.0f;
            proj[1][1] = (2.0f * near) / (top - bottom);
            proj[1][2] = 0.0f;
            proj[1][3] = 0.0f;

            // Z-axis transformation for depth range [0,1] from [-near, -far]
            proj[2][0] = (right + left) / (right - left);      // X translation (horizontal offset)
            proj[2][1] = (top + bottom) / (top - bottom);      // Y translation (vertical offset)
            proj[2][2] = far / (near - far);                   // Z scaling (map to [0,1])
            proj[2][3] = -1.0f;                                // W component (W_clip = -z)

            // Homogeneous w-coordinate transformation
            proj[3][0] = 0.0f;
            proj[3][1] = 0.0f;
            proj[3][2] = (far * near) / (near - far);          // Z translation
            proj[3][3] = 0.0f;

            return proj;
        }

        /**
         * @brief Right-handed coordinate system Off-Center perspective projection matrix with reversed Z depth range [0,1]
         *
         * Maps asymmetric frustum to NDC space, with reversed depth range [1,0]
         * - Near plane (-n) maps to depth 1.0
         * - Far plane (-f) maps to depth 0.0
         *
         * @param left near plane left boundary
         * @param right near plane right boundary
         * @param bottom near plane bottom boundary
         * @param top near plane top boundary
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Off-Center perspective projection matrix (reversed Z)
         */
        glm::float4x4 perspectiveOffCenterRH_ZO_ReverseZ(float left, float right, float bottom, float top, float near, float far) {
            assert(near > 0.0f && far > near);
            assert(right != left && top != bottom);

            glm::float4x4 proj(1.0f);

            // X-axis scaling factor
            proj[0][0] = (2.0f * near) / (right - left);
            proj[0][1] = 0.0f;
            proj[0][2] = 0.0f;
            proj[0][3] = 0.0f;

            // Y-axis scaling factor
            proj[1][0] = 0.0f;
            proj[1][1] = (2.0f * near) / (top - bottom);
            proj[1][2] = 0.0f;
            proj[1][3] = 0.0f;

            // Z-axis transformation for reversed depth range [1,0] from [-near, -far]
            proj[2][0] = (right + left) / (right - left);      // X translation (horizontal offset)
            proj[2][1] = (top + bottom) / (top - bottom);      // Y translation (vertical offset)
            proj[2][2] = near / (far - near);                  // Z scaling (Reverse Z mapping)
            proj[2][3] = -1.0f;                                // W component (W_clip = -z)

            // Homogeneous w-coordinate transformation
            proj[3][0] = 0.0f;
            proj[3][1] = 0.0f;
            proj[3][2] = (near * far) / (far - near);          // Z translation (Reverse Z mapping)
            proj[3][3] = 0.0f;

            return proj;
        }

        /**
         * @brief Right-handed coordinate system Off-Center perspective projection matrix, Z range [0,1]
         *
         * Maps asymmetric frustum to NDC space, depth range [0,1]
         * - Near plane (-n) maps to depth 0.0
         * - Far plane (-f) maps to depth 1.0
         *
         * @param left near plane left boundary
         * @param right near plane right boundary
         * @param bottom near plane bottom boundary
         * @param top near plane top boundary
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Off-Center perspective projection matrix
         */
        glm::float4x4 perspective(float left, float right, float bottom, float top, float near, float far)
        {
#if MOYU_REVERSE_DEPTH
            return perspectiveOffCenterRH_ZO_ReverseZ(left, right, bottom, top, near, far);
#else
            return perspectiveOffCenterRH_ZO(left, right, bottom, top, near, far);
#endif
        }

        glm::float4x4 perspective(float width, float height, float near, float far)
        {
            return perspective(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, near, far);
        }


        /**
         * @brief Right-handed coordinate system orthographic projection matrix, Z range [0,1]
         *
         * Maps viewing volume to NDC space, depth range [0,1]
         * - Near plane (-n) maps to depth 0.0
         * - Far plane (-f) maps to depth 1.0
         *
         * @param left Viewing volume left boundary
         * @param right Viewing volume right boundary
         * @param bottom Viewing volume bottom boundary
         * @param top Viewing volume top boundary
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Orthographic projection matrix
         */
        glm::float4x4 orthoRH_ZO(float left, float right, float bottom, float top, float near, float far) {
            assert(right != left && top != bottom && far != near);

            glm::float4x4 proj(1.0f);

            // X-axis scaling and translation
            proj[0][0] = 2.0f / (right - left);
            proj[0][1] = 0.0f;
            proj[0][2] = 0.0f;
            proj[0][3] = 0.0f;

            // Y-axis scaling and translation
            proj[1][0] = 0.0f;
            proj[1][1] = 2.0f / (top - bottom);
            proj[1][2] = 0.0f;
            proj[1][3] = 0.0f;

            // Z-axis transformation for depth range [0,1] from [-near, -far]
            proj[2][0] = 0.0f;
            proj[2][1] = 0.0f;
            proj[2][2] = -1.0f / (far - near); // Negative sign because RH coordinate system looks along -Z direction
            proj[2][3] = 0.0f;

            // Translation components
            proj[3][0] = -(right + left) / (right - left);   // X translation
            proj[3][1] = -(top + bottom) / (top - bottom);   // Y translation
            proj[3][2] = -near / (far - near);               // Z translation (map to [0,1])
            proj[3][3] = 1.0f;                               // Preserve homogeneous coordinate w component

            return proj;
        }

        /**
         * @brief Right-handed coordinate system orthographic projection matrix, reversed depth, Z range [0,1]
         *
         * Maps viewing volume to NDC space, with reversed depth range [1,0]
         * - Near plane (-n) maps to depth 1.0
         * - Far plane (-f) maps to depth 0.0
         *
         * @param left Viewing volume left boundary
         * @param right Viewing volume right boundary
         * @param bottom Viewing volume bottom boundary
         * @param top Viewing volume top boundary
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Orthographic projection matrix (reversed depth)
         */
        glm::float4x4 orthoRH_ZO_ReverseZ(float left, float right, float bottom, float top, float near, float far) {
            assert(right != left && top != bottom && far != near);

            glm::float4x4 proj(1.0f);

            // X-axis scaling and translation
            proj[0][0] = 2.0f / (right - left);
            proj[0][1] = 0.0f;
            proj[0][2] = 0.0f;
            proj[0][3] = 0.0f;

            // Y-axis scaling and translation
            proj[1][0] = 0.0f;
            proj[1][1] = 2.0f / (top - bottom);
            proj[1][2] = 0.0f;
            proj[1][3] = 0.0f;

            // Z-axis transformation for reversed depth range [1,0] from [-near, -far]
            proj[2][0] = 0.0f;
            proj[2][1] = 0.0f;
            proj[2][2] = 1.0f / (far - near); // Positive sign achieves Reverse Z mapping
            proj[2][3] = 0.0f;

            // Translation components
            proj[3][0] = -(right + left) / (right - left);   // X translation
            proj[3][1] = -(top + bottom) / (top - bottom);   // Y translation
            proj[3][2] = far / (far - near);                 // Z translation (Reverse Z mapping)
            proj[3][3] = 1.0f;                               // Preserve homogeneous coordinate w component

            return proj;
        }

        /**
         * @brief Right-handed coordinate system orthographic projection matrix, Z range [0,1]
         *
         * Maps viewing volume to NDC space, depth range [0,1]
         * - Near plane (-n) maps to depth 0.0
         * - Far plane (-f) maps to depth 1.0
         *
         * @param left Viewing volume left boundary
         * @param right Viewing volume right boundary
         * @param bottom Viewing volume bottom boundary
         * @param top Viewing volume top boundary
         * @param near Near clipping plane distance (positive value)
         * @param far Far clipping plane distance (positive value)
         * @return glm::float4x4 Orthographic projection matrix
         */
        glm::float4x4 orthographic(float left, float right, float bottom, float top, float near, float far)
        {
#if MOYU_REVERSE_DEPTH
            return orthoRH_ZO_ReverseZ(left, right, bottom, top, near, far);
#else
            return orthoRH_ZO(left, right, bottom, top, near, far);
#endif
        }

        glm::float4x4 orthographic(float width, float height, float near, float far)
        {
            return orthographic(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, near, far);
        }


        /**
         * @brief Create View Matrix for right-handed coordinate system
         *
         * In right-handed coordinate system:
         * - X-axis points to the right
         * - Y-axis points upward
         * - Z-axis points into the screen (camera looks in -Z direction)
         *
         * @param eye Camera position (world coordinates)
         * @param center Camera target point (world coordinates)
         * @param up Camera up direction vector (usually (0,1,0))
         * @return glm::float4x4 View Matrix (converts world coordinates to camera coordinates)
         */
        glm::float4x4 lookAtRH_(const glm::float3& eye, const glm::float3& center, const glm::float3& up) {
            // Calculate forward vector (pointing from eye to center)
            glm::float3 forward = glm::normalize(center - eye);

            // Calculate right vector (perpendicular to up and forward)
            glm::float3 right = glm::normalize(glm::cross(forward, up));

            // Recalculate up vector (perpendicular to forward and right)
            glm::float3 newUp = glm::normalize(glm::cross(right, forward));

            // Build rotation matrix
            glm::float4x4 view(1.0f);
            view[0][0] = right.x;
            view[1][0] = right.y;
            view[2][0] = right.z;

            view[0][1] = newUp.x;
            view[1][1] = newUp.y;
            view[2][1] = newUp.z;

            view[0][2] = -forward.x; // Negative Z-axis
            view[1][2] = -forward.y;
            view[2][2] = -forward.z;

            // Build translation matrix
            view[3][0] = -glm::dot(right, eye);
            view[3][1] = -glm::dot(newUp, eye);
            view[3][2] = glm::dot(forward, eye);
            view[3][3] = 1.0f;

            return view;
        }

        glm::float4x4 lookAtRH(const glm::float3& eye, const glm::float3& center, const glm::float3& up)
        {
#if MOYU_USE_GLM_VIEW
            return glm::lookAtRH(eye, center, up);
#else
            return lookAtRH_(eye, center, up);
#endif
        }

        /**
         * @brief Create View Matrix for right-handed coordinate system via Euler angles
         *
         * @param position Camera position
         * @param yaw Y-axis rotation angle (radians, positive angle turns right when looking at -Z)
         * @param pitch X-axis rotation angle (radians, positive angle looks up)
         * @param roll Z-axis rotation angle (radians, usually 0)
         * @return glm::float4x4 View Matrix
         */
        glm::float4x4 viewMatrixFromEulerAngles(const glm::float3& position, float yaw, float pitch, float roll)
        {
            // Convert angles to radians
            float yawRad = glm::radians(yaw);
            float pitchRad = glm::radians(pitch);
            float rollRad = glm::radians(roll);

            // Create quaternion (order: Yaw -> Pitch -> Roll)
            glm::quat quat = glm::quat(glm::vec3(pitchRad, yawRad, rollRad));

            // Get rotation matrix (from camera space to world space)
            glm::float4x4 rotationMatrix = glm::toMat4(quat);

            // In right-handed coordinate system, camera looks along -Z direction
            glm::float3 forward = glm::normalize(glm::float3(rotationMatrix * glm::float4(0, 0, -1, 0)));
            glm::float3 right = glm::normalize(glm::float3(rotationMatrix * glm::float4(1, 0, 0, 0)));
            glm::float3 up = glm::normalize(glm::float3(rotationMatrix * glm::float4(0, 1, 0, 0)));

            // Build view matrix (consistent with GLM's lookAtRH)
            glm::float4x4 view(1.0f);

            view[0][0] = right.x;
            view[1][0] = right.y;
            view[2][0] = right.z;

            view[0][1] = up.x;
            view[1][1] = up.y;
            view[2][1] = up.z;

            view[0][2] = -forward.x;
            view[1][2] = -forward.y;
            view[2][2] = -forward.z;

            view[3][0] = -glm::dot(right, position);
            view[3][1] = -glm::dot(up, position);
            view[3][2] = glm::dot(forward, position);

            return view;
        }

        /**
         * @brief Create View Matrix for right-handed coordinate system from camera position and quaternion
         *
         * In right-handed coordinate system:
         * - X-axis points to the right
         * - Y-axis points upward
         * - Z-axis points into the screen (camera looks in -Z direction)
         *
         * @param position Camera position in world coordinate system
         * @param orientation Quaternion representing camera orientation (rotation from camera local coordinate system to world coordinate system)
         * @return glm::float4x4 Right-handed coordinate system View Matrix
         */
        glm::float4x4 viewMatrixFromQuaternion(const glm::float3& position, const glm::quat& orientation)
        {
            // Convert quaternion to rotation matrix
            glm::float4x4 rotationMatrix = glm::mat4_cast(orientation);

            // In right-handed coordinate system, camera looks along -Z direction by default, up direction is Y-axis
            glm::float3 forward = -glm::normalize(glm::float3(rotationMatrix * glm::float4(0.0f, 0.0f, -1.0f, 0.0f)));
            glm::float3 up = glm::normalize(glm::float3(rotationMatrix * glm::float4(0.0f, 1.0f, 0.0f, 0.0f)));

            // Right vector can be calculated via cross product
            glm::float3 right = glm::normalize(glm::cross(forward, up));
            // Update up vector to ensure all three basis vectors are mutually perpendicular
            up = glm::normalize(glm::cross(right, forward));

            // Build view matrix
            glm::float4x4 viewMatrix(1.0f);

            // Set first column: right vector
            viewMatrix[0][0] = right.x;
            viewMatrix[1][0] = right.y;
            viewMatrix[2][0] = right.z;

            // Set second column: up vector
            viewMatrix[0][1] = up.x;
            viewMatrix[1][1] = up.y;
            viewMatrix[2][1] = up.z;

            // Set third column: forward vector (note the negative sign)
            viewMatrix[0][2] = -forward.x;
            viewMatrix[1][2] = -forward.y;
            viewMatrix[2][2] = -forward.z;

            // Set fourth column: translation components
            viewMatrix[3][0] = -glm::dot(right, position);
            viewMatrix[3][1] = -glm::dot(up, position);
            viewMatrix[3][2] = glm::dot(forward, position); // Note the positive sign here

            return viewMatrix;
        }

        /**
         * @brief Create transformation matrix from position, rotation and scale (right-handed coordinate system)
         *
         * Transformation order: Scale first -> then Rotate -> finally Translate
         * This is the most commonly used transformation order, avoids scaling affecting rotation axes
         *
         * @param position Position vector (x, y, z)
         * @param rotation Rotation quaternion (represents rotation from local coordinate system to world coordinate system)
         * @param scale Scale vector (sx, sy, sz)
         * @return glm::float4x4 4x4 transformation matrix
         */
        glm::float4x4 transformDirect(const glm::float3& position, const glm::quat& rotation, const glm::float3& scale)
        {
			// glm::mat4_cast converts quaternion to 4x4 rotation matrix
            glm::float4x4 rotationMatrix = glm::mat4_cast(rotation);

            // Create final transformation matrix (initialized as identity matrix)
            glm::float4x4 transform(1.0f);

            // Manually apply scale to rotation matrix
            // Note: In GLM, matrices are column-major, so column 0 is x-axis, column 1 is y-axis, column 2 is z-axis
            transform[0][0] = rotationMatrix[0][0] * scale.x;  // Column 0, x-axis scaling
            transform[1][0] = rotationMatrix[1][0] * scale.x;
            transform[2][0] = rotationMatrix[2][0] * scale.x;

            transform[0][1] = rotationMatrix[0][1] * scale.y;  // Column 1, y-axis scaling
            transform[1][1] = rotationMatrix[1][1] * scale.y;
            transform[2][1] = rotationMatrix[2][1] * scale.y;

            transform[0][2] = rotationMatrix[0][2] * scale.z;  // Column 2, z-axis scaling
            transform[1][2] = rotationMatrix[1][2] * scale.z;
            transform[2][2] = rotationMatrix[2][2] * scale.z;

            // Set translation part (column 3)
            transform[3][0] = position.x;
            transform[3][1] = position.y;
            transform[3][2] = position.z;

            // Preserve homogeneous coordinate w=1
            transform[0][3] = 0.0f;
            transform[1][3] = 0.0f;
            transform[2][3] = 0.0f;
            transform[3][3] = 1.0f;

            return transform;
        }

        /**
         * @brief Create transformation matrix from position, rotation and scale (right-handed coordinate system)
         *
         * Transformation order: Scale first -> then Rotate -> finally Translate
         * This is the most commonly used transformation order, avoids scaling affecting rotation axes
         *
         * @param position Position vector (x, y, z)
         * @param rotation Rotation quaternion (represents rotation from local coordinate system to world coordinate system)
         * @param scale Scale vector (sx, sy, sz)
         * @return glm::float4x4 4x4 transformation matrix
         */
        glm::float4x4 transform(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale)
        {
            glm::mat t = glm::translate(glm::float4x4(1.0), position);
            glm::mat r = glm::toMat4(orientation);
            glm::mat s = glm::scale(glm::float4x4(1.0), scale);
            glm::mat trs = t * r * s;
            return trs;
        }

        /** Building an inverse Matrix4 from orientation / scale / position.
        @remarks
        As makeTransform except it build the inverse given the same data as makeTransform, so
        performing -translation, -rotate, 1/scale in that order.
        */
        glm::float4x4 transformInverse(const glm::float3& position, const glm::quat& orientation, const glm::float3& scale)
        {
            glm::mat t_inverse = glm::translate(glm::float4x4(1.0), -position);
            glm::mat r_inverse = glm::toMat4(glm::inverse(orientation));
            glm::mat s_inverse = glm::scale(glm::float4x4(1.0), 1.0f / scale);
            glm::mat trs_inverse = s_inverse * r_inverse * t_inverse;
            return trs_inverse;
        }

    }

    //------------------------------------------------------------------------------------------

    namespace MYQuaternion
    {

        glm::float4x4 viewMatrixFromEulerAngles(const glm::float3& position, const EulerAngle& eulerAngle)
        {
            return MYMatrix4x4::viewMatrixFromEulerAngles(position, eulerAngle.yaw, eulerAngle.pitch, eulerAngle.roll);
        }

        /**
         * Convert quaternion to Euler angles (right-handed coordinate system)
         *
         * Note on rotation order (YXZ Tait-Bryan angles):
         * This function uses INTRINSIC rotation order (local coordinate system):
         * 1. First:  Rotate around LOCAL Z-axis (roll)
         * 2. Second: Rotate around ROTATED X-axis (pitch)
         * 3. Third:  Rotate around TWICE-ROTATED Y-axis (yaw)
         *
         * This matches aerospace "yaw-pitch-roll" convention:
         * - Yaw (heading)    = Y-axis rotation
         * - Pitch (attitude) = X-axis rotation
         * - Roll (bank)      = Z-axis rotation
         *
         * @param q Quaternion (normalized)
         * @return Euler angles (degrees) in YXZ order
         */
        EulerAngle quaternionToEuler(const glm::quat& q) {
            // Ensure quaternion is normalized
            glm::quat normalizedQ = glm::normalize(q);

            // Squared components
            float sqw = normalizedQ.w * normalizedQ.w;
            float sqx = normalizedQ.x * normalizedQ.x;
            float sqy = normalizedQ.y * normalizedQ.y;
            float sqz = normalizedQ.z * normalizedQ.z;

            // Cross products
            float qxy = 2.0f * normalizedQ.x * normalizedQ.y;
            float qxz = 2.0f * normalizedQ.x * normalizedQ.z;
            float qyz = 2.0f * normalizedQ.y * normalizedQ.z;
            float qxw = 2.0f * normalizedQ.x * normalizedQ.w;
            float qyw = 2.0f * normalizedQ.y * normalizedQ.w;
            float qzw = 2.0f * normalizedQ.z * normalizedQ.w;

            float yaw, pitch, roll;

            // Yaw (Y-axis rotation) - CORRECT
            yaw = glm::atan(qyw + qxz, sqw - sqx - sqy + sqz);

            // Pitch (X-axis rotation) - FIXED SIGN
            float sinp = qxw - qyz;  // Correct sign for YXZ order
            if (glm::abs(sinp) >= 1.0f) {
                // Gimbal lock - pitch is б└90 degrees
                pitch = MoYu::gameEngineCopysign(glm::half_pi<float>(), sinp);

                // When gimbal locked, set roll to zero and adjust yaw accordingly
                roll = 0.0f;
                if (sinp < 0.0f) {
                    yaw -= glm::atan(qxy - qzw, sqw - sqx + sqy - sqz);
                }
                else {
                    yaw += glm::atan(qxy - qzw, sqw - sqx + sqy - sqz);
                }
            }
            else {
                pitch = glm::asin(sinp);
                // Roll (Z-axis rotation) - FIXED DENOMINATOR
                roll = glm::atan(qzw + qxy, sqw - sqx + sqy - sqz);
            }

            // Convert to degrees
            yaw = glm::degrees(yaw);
            pitch = glm::degrees(pitch);
            roll = glm::degrees(roll);

            // Normalize to [-180, 180] with better numerical stability
            auto normalizeAngle = [](float angle) {
                while (angle > 180.0f) angle -= 360.0f;
                while (angle < -180.0f) angle += 360.0f;
                return angle;
                };

            return EulerAngle(
                normalizeAngle(yaw),
                normalizeAngle(pitch),
                normalizeAngle(roll)
            );
        }

        /**
         * Convert Euler angles to quaternion (right-handed coordinate system)
         *
         * Note on rotation order (YXZ Tait-Bryan angles):
         * In 3D graphics, YXZ order typically refers to INTRINSIC rotations
         * (applied in local/object coordinate system) in the following sequence:
         * 1. First:  Rotate around LOCAL Z-axis (roll)
         * 2. Second: Rotate around ROTATED X-axis (pitch)
         * 3. Third:  Rotate around TWICE-ROTATED Y-axis (yaw)
         *
         * This is equivalent to the aerospace "yaw-pitch-roll" convention where:
         * - Yaw (heading)    = rotation around Y-axis
         * - Pitch (attitude) = rotation around X-axis
         * - Roll (bank)      = rotation around Z-axis
         *
         * @param euler Euler angles (degrees) in YXZ order
         * @return Quaternion representing the combined rotation
         */
        glm::quat eulerToQuaternion(const EulerAngle& euler) {
            // Convert to radians
            float yawRad = glm::radians(euler.yaw);
            float pitchRad = glm::radians(euler.pitch);
            float rollRad = glm::radians(euler.roll);

            // Calculate trigonometric functions
            float cy = glm::cos(yawRad * 0.5f);
            float sy = glm::sin(yawRad * 0.5f);
            float cp = glm::cos(pitchRad * 0.5f);
            float sp = glm::sin(pitchRad * 0.5f);
            float cr = glm::cos(rollRad * 0.5f);
            float sr = glm::sin(rollRad * 0.5f);

            glm::quat q;

            // YXZ Tait-Bryan angles (intrinsic rotation order: Z-X-Y)
            q.w = cr * cp * cy + sr * sp * sy;
            q.x = cr * sp * cy + sr * cp * sy;
            q.y = cr * cp * sy - sr * sp * cy;
            q.z = sr * cp * cy - cr * sp * sy;

            return glm::normalize(q);
        }

        /**
         * Create quaternion from direction vector (right-handed coordinate system)
         *
         * @param direction Target direction vector (does not need normalization)
         * @param up Reference up vector (defaults to world up (0,1,0))
         * @param forwardReference Reference forward vector (standard for right-handed coordinate system is (0,0,-1))
         * @return Rotation quaternion, rotates forwardReference to direction
         */
        glm::quat directionToQuaternion(
            const glm::float3& direction,
            const glm::float3& up,
            const glm::float3& forwardReference
        ) {
            // Check for zero vector
            if (glm::length(direction) < 0.001f) {
                return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }

            // Normalize inputs
            glm::float3 forward = glm::normalize(direction);
            glm::float3 worldUp = glm::normalize(up);
            glm::float3 refForward = glm::normalize(forwardReference);

            // Handle parallel/anti-parallel cases
            float dot = glm::dot(refForward, forward);
            if (dot > 0.9999f) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            if (dot < -0.9999f) {
                glm::float3 axis = glm::abs(refForward.x) < 0.9f ?
                    glm::normalize(glm::cross(refForward, glm::float3(1.0f, 0.0f, 0.0f))) :
                    glm::normalize(glm::cross(refForward, glm::float3(0.0f, 1.0f, 0.0f)));
                return glm::angleAxis(glm::radians(180.0f), axis);
            }

            // Build target coordinate system - Key correction: use correct cross product order
            glm::float3 right = glm::normalize(glm::cross(forward, worldUp));

            // Handle degenerate case (when forward and worldUp are parallel)
            if (glm::length(right) < 0.001f) {
                glm::float3 tempAxis = glm::abs(forward.x) < 0.9f ?
                    glm::float3(1.0f, 0.0f, 0.0f) :
                    glm::float3(0.0f, 1.0f, 0.0f);
                right = glm::normalize(glm::cross(forward, tempAxis));
            }

            // Calculate corrected up vector (maintain right-handed coordinate system)
            glm::float3 correctedUp = glm::normalize(glm::cross(right, forward));

            // Build rotation matrix
		    // Note: Here we directly build a local-to-world transformation matrix
		    // Since forwardReference=(0,0,-1), we are actually constructing a matrix
		    // that rotates the standard right-handed coordinate system (Z pointing into the screen)
		    // to the target direction
            glm::mat3 rotationMatrix(
                right.x, right.y, right.z,
                correctedUp.x, correctedUp.y, correctedUp.z,
                -forward.x, -forward.y, -forward.z  // Key: use negative sign because forwardReference is (0,0,-1)
            );

            // Convert to quaternion
            return glm::normalize(glm::quat_cast(glm::float4x4(rotationMatrix)));
        }

        /**
         * Get direction vector from quaternion (right-handed coordinate system)
         *
         * @param rotation Rotation quaternion
         * @param forwardReference Reference forward vector (standard for right-handed coordinate system is (0,0,-1))
         * @return Rotated direction vector
         */
        glm::float3 quaternionToDirection(
            const glm::quat& rotation,
            const glm::float3& forwardReference
        ) {
            // Rotate the reference vector
            return glm::normalize(glm::rotate(rotation, forwardReference));
        }

        /**
         * Optimized version: Quickly get forward vector from quaternion (right-handed coordinate system, standard forward (0,0,-1))
         * Avoids overhead of general rotation functions
         */
        glm::float3 fastQuaternionToForward(const glm::quat& q) {
            // Fast calculation of forward vector from quaternion (0,0,-1)
            // x = -2 * (x*z + w*y)
            // y = 2 * (y*z - w*x)
            // z = -(1 - 2*(x*x + y*y))

            float x = q.x;
            float y = q.y;
            float z = q.z;
            float w = q.w;

            return glm::float3(
                -2.0f * (x * z + w * y),
                2.0f * (y * z - w * x),
                -(1.0f - 2.0f * (x * x + y * y))
            );
        }

        /**
         * Calculate Euler angles from direction vector (full version)
         *
         * Note on coordinate system:
         * This function assumes a RIGHT-HANDED coordinate system with Y-up.
         * Default forward direction is -Z (OpenGL convention), but can be customized.
         *
         * Rotation order follows YXZ Tait-Bryan convention (intrinsic rotations):
         * 1. Roll (Z-axis)  - applied first
         * 2. Pitch (X-axis) - applied second
         * 3. Yaw (Y-axis)   - applied last
         *
         * @param direction Direction vector (does not need normalization)
         * @param up Reference up vector (defaults to (0,1,0))
         * @param forwardReference Reference forward vector (standard is (0,0,-1))
         * @return Euler angles (yaw, pitch, roll) in degrees
         */
        EulerAngle directionToEuler(
            const glm::float3& direction,
            const glm::float3& up,
            const glm::float3& forwardReference
        ) {
            // Handle zero direction vector
            float length = glm::length(direction);
            if (length < 0.001f) {
                return EulerAngle(0.0f, 0.0f, 0.0f);
            }

            // Normalize direction vector
            glm::float3 dir = direction / length;

            // Calculate yaw and pitch based on coordinate system
            float yaw, pitch, roll;

            // Special case: when direction is parallel to up vector (gimbal lock)
            glm::float3 rightReference = glm::normalize(glm::cross(forwardReference, up));

            if (glm::abs(glm::dot(dir, up)) > 0.999f) {
                // Direction is nearly parallel to up vector - gimbal lock
                yaw = glm::degrees(glm::atan(dir.x, -dir.z)); // Fallback to XZ projection
                pitch = glm::sign(glm::dot(dir, up)) * 90.0f;
                roll = 0.0f; // Undefined in gimbal lock, set to 0
            }
            else {
                // Calculate yaw using projection onto XZ plane
                glm::float3 xzProjection = glm::float3(dir.x, 0.0f, dir.z);
                if (glm::length(xzProjection) > 0.001f) {
                    xzProjection = glm::normalize(xzProjection);
                    // Calculate angle between projection and reference forward
                    float cosYaw = glm::dot(xzProjection, glm::normalize(glm::float3(forwardReference.x, 0.0f, forwardReference.z)));
                    float sinYaw = glm::dot(glm::cross(glm::float3(forwardReference.x, 0.0f, forwardReference.z), xzProjection), up);
                    yaw = glm::degrees(glm::atan2(sinYaw, cosYaw));
                }
                else {
                    yaw = 0.0f;
                }

                // Calculate pitch as angle between direction and XZ plane
                pitch = glm::degrees(glm::asin(glm::clamp(dir.y, -1.0f, 1.0f)));

                // Calculate roll
                // 1. Calculate right vector (X-axis) - CORRECTED CROSS PRODUCT ORDER
                glm::float3 right = glm::normalize(glm::cross(dir, up)); // cross(forward, up) = right

                // 2. Calculate reference up vector (Y-axis) with no roll
                glm::float3 levelUp = glm::normalize(glm::cross(right, dir)); // cross(right, forward) = up

                // 3. Calculate roll angle between reference up and actual up
                float cosRoll = glm::dot(levelUp, up);
                float sinRoll = glm::dot(glm::cross(levelUp, up), dir); // Direction of rotation around forward axis

                roll = glm::degrees(glm::atan2(sinRoll, cosRoll));
            }

            // Normalize angles to [-180, 180]
            auto normalizeAngle = [](float angle) {
                angle = fmodf(angle, 360.0f);
                if (angle > 180.0f) angle -= 360.0f;
                if (angle < -180.0f) angle += 360.0f;
                return angle;
                };

            // Prevent pitch gimbal lock in edge cases
            pitch = glm::clamp(pitch, -89.99f, 89.99f);

            return EulerAngle(
                normalizeAngle(yaw),
                normalizeAngle(pitch),
                normalizeAngle(roll)
            );
        }

        /**
         * Calculate direction vector from Euler angles
         *
         * Note on coordinate system:
         * This function supports arbitrary forward reference directions while maintaining Y-up convention.
         * The resulting direction vector is calculated by applying yaw (around world Y) and pitch
         * (around local X) rotations to the forwardReference vector.
         *
         * @param euler Euler angles (unit: degrees). Roll is ignored for direction calculation.
         * @param forwardReference Reference forward vector (standard is (0,0,-1) for OpenGL)
         * @param upReference Reference up vector (defaults to (0,1,0), Y-up convention)
         * @return Rotated direction vector (normalized)
         */
        glm::float3 eulerToDirection(
            const EulerAngle& euler,
            const glm::float3& forwardReference,
            const glm::float3& upReference
        ) {
            // Normalize reference vectors
            glm::float3 forward = glm::normalize(forwardReference);
            glm::float3 up = glm::normalize(upReference);

            // Ensure forward and up are perpendicular (within numerical tolerance)
            float dotFU = glm::abs(glm::dot(forward, up));
            if (dotFU > 0.999f) {
                // Forward and up are nearly parallel - need to find an orthogonal up vector
                glm::float3 arbitrary = glm::abs(forward.x) < 0.9f ?
                    glm::float3(1.0f, 0.0f, 0.0f) :
                    glm::float3(0.0f, 1.0f, 0.0f);
                up = glm::normalize(glm::cross(glm::cross(forward, arbitrary), forward));
            }

            // Calculate right vector (X-axis) from forward and up
            glm::float3 right = glm::normalize(glm::cross(forward, up));

            // Convert angles to radians
            float yaw = glm::radians(euler.yaw);
            float pitch = glm::radians(euler.pitch);

            // Calculate direction by applying rotations in YX order:
            // 1. Start with forward reference vector
            // 2. Apply yaw rotation around world up vector
            // 3. Apply pitch rotation around resulting right vector

            // Apply yaw rotation (around world Y/up)
            glm::mat4 yawRotation = glm::rotate(glm::mat4(1.0f), yaw, up);
            glm::float3 yawedForward = glm::normalize(glm::vec3(yawRotation * glm::vec4(forward, 0.0f)));
            glm::float3 yawedRight = glm::normalize(glm::vec3(yawRotation * glm::vec4(right, 0.0f)));

            // Apply pitch rotation (around local X/right)
            glm::mat4 pitchRotation = glm::rotate(glm::mat4(1.0f), pitch, yawedRight);
            glm::float3 direction = glm::normalize(glm::vec3(pitchRotation * glm::vec4(yawedForward, 0.0f)));

            // Handle edge cases when pitch approaches +/-90 degrees (gimbal lock)
            // This ensures numerical stability when cos(pitch) approaches zero
            if (glm::abs(glm::dot(direction, up)) > 0.9999f) {
                direction = glm::normalize(glm::mix(
                    up,          // Looking straight up
                    -up,         // Looking straight down
                    (pitch < 0.0f) ? 1.0f : 0.0f
                ));
            }

            return direction;
        }

        /**
         * High-performance version: Compute yaw and pitch from direction vector (no roll)
         * Suitable for FPS/TPS cameras and other scenarios where roll is not needed
         *
         * @param direction Input direction vector (does not need to be pre-normalized)
         * @param yaw Output yaw angle (in degrees, range [-180, 180])
         * @param pitch Output pitch angle (in degrees, range [-89.99, 89.99])
         */
        void fastDirectionToYawPitch(const glm::vec3& direction, float& yaw, float& pitch) {
            // Normalize input vector
            glm::vec3 dir = glm::normalize(direction);

            // Handle gimbal lock (pitch near б└90бу)
            if (std::abs(dir.y) > 0.9999f) {
                pitch = glm::sign(dir.y) * 89.99f;  // Prevent exact 90бу to avoid issues
                yaw = 0.0f;  // Yaw is undefined at poles, set to 0
                return;
            }

            // Calculate yaw and pitch
            // Note: Using -dir.z because forward direction is -Z axis (OpenGL convention)
            yaw = glm::degrees(glm::atan(dir.x, -dir.z));
            pitch = glm::degrees(glm::asin(dir.y));

            // Optimized angle normalization (avoid fmod performance overhead)
            if (yaw > 180.0f) {
                yaw -= 360.0f;
            }
            else if (yaw < -180.0f) {
                yaw += 360.0f;
            }

            // Clamp pitch range to prevent gimbal lock
            pitch = glm::clamp(pitch, -89.99f, 89.99f);
        }

        /**
         * High-performance version: Calculate forward vector from yaw and pitch
         * Suitable for performance-critical rendering loops
         */
        glm::float3 fastYawPitchToForward(float yawDegrees, float pitchDegrees) {
            float yaw = glm::radians(yawDegrees);
            float pitch = glm::radians(pitchDegrees);

            float cosPitch = glm::cos(pitch);
            return glm::float3(
                glm::sin(yaw) * cosPitch,
                glm::sin(pitch),
                -glm::cos(yaw) * cosPitch  // Negative Z because forward is -Z axis
            );
        }

    }

    
    //------------------------------------------------------------------------------------------

    void Transform::setFromParent(const Transform& parent, const Transform& world) {
        glm::float4x4 parentInv = glm::inverse(((Transform&)parent).getMatrix());
        const glm::float4x4& worldMat = ((Transform&)world).getMatrix();
        glm::float4x4 localMat = parentInv * worldMat;

        glm::float3 skew;
        glm::vec4 perspective;
        glm::decompose(localMat, m_scale, m_rotation, m_position, skew, perspective);
        m_rotation = glm::normalize(m_rotation);

        m_isDirty = false;
        m_matrix = localMat;
    }

    void Transform::recalculateMatrix() {
        m_matrix = MYMatrix4x4::transformDirect(m_position, m_rotation, m_scale);
        m_isDirty = false;
    }


    //------------------------------------------------------------------------------------------
    AABB::AABB(const glm::float3& minpos, const glm::float3& maxpos)
    {
        m_min = minpos;
        m_max = maxpos;
    }

    AABB::AABB(const AABB& other)
    {
        m_min = other.m_min;
        m_max = other.m_max;
    }

    void AABB::merge(const AABB& axis_aligned_box)
    { 
        merge(axis_aligned_box.getMinCorner());
        merge(axis_aligned_box.getMaxCorner());
    }

    void AABB::merge(const glm::float3& new_point)
    {
        m_min = glm::min(m_min, new_point);
        m_max = glm::max(m_max, new_point);
    }

    void AABB::update(const glm::float3& center, const glm::float3& half_extent)
    {
        m_min = center - half_extent;
        m_max = center + half_extent;
    }

    float AABB::minDistanceFromPointSq(const glm::float3& point)
    {
        float dist = 0.0f;

        if (point.x < m_min.x)
        {
           float d = point.x - m_min.x;
           dist += d * d;
        }
        else if (point.x > m_max.x)
        {
           float d = point.x - m_max.x;
           dist += d * d;
        }

        if (point.y < m_min.y)
        {
           float d = point.y - m_min.y;
           dist += d * d;
        }
        else if (point.y > m_max.y)
        {
           float d = point.y - m_max.y;
           dist += d * d;
        }

        if (point.z < m_min.z)
        {
           float d = point.z - m_min.z;
           dist += d * d;
        }
        else if (point.z > m_max.z)
        {
           float d = point.z - m_max.z;
           dist += d * d;
        }

        return dist;
    }

    float AABB::maxDistanceFromPointSq(const glm::float3& point)
    {
        float dist = 0.0f;
        float k;

        k = glm::max(fabsf(point.x - m_min.x), fabsf(point.x - m_max.x));
        dist += k * k;

        k = glm::max(fabsf(point.y - m_min.y), fabsf(point.y - m_max.y));
        dist += k * k;

        k = glm::max(fabsf(point.z - m_min.z), fabsf(point.z - m_max.z));
        dist += k * k;

        return dist;
    }

    bool AABB::isInsideSphereSq(const BSphere& other)
    {
        float radiusSq = other.radius * other.radius;
        return maxDistanceFromPointSq(other.center) <= radiusSq;
    }

    bool AABB::intersects(const BSphere& other)
    {
        float radiusSq = other.radius * other.radius;
        return minDistanceFromPointSq(other.center) <= radiusSq;
    }

    bool AABB::intersects(const AABB& other)
    {
        return !((other.m_max.x < this->m_min.x) || (other.m_min.x > this->m_max.x) || (other.m_max.y < this->m_min.y) ||
                 (other.m_min.y > this->m_max.y) || (other.m_max.z < this->m_min.z) || (other.m_min.z > this->m_max.z));
    }

    bool AABB::intersectRay(const Ray& ray, float& distance)
    {
        const glm::float3 rayOrigin = ray.origin;
        const glm::float3 rayDirection = ray.direction;

        float tmin = -FLT_MAX; // set to -FLT_MAX to get first hit on line
        float tmax = FLT_MAX;  // set to max distance ray can travel

        float _rayOrigin[]    = {rayOrigin.x, rayOrigin.y, rayOrigin.z};
        float _rayDirection[] = {rayDirection.x, rayDirection.y, rayDirection.z};
        float _min[]          = {m_min.x, m_min.y, m_min.z};
        float _max[]          = {m_max.x, m_max.y, m_max.z};

        const float EPSILON = 1e-5f;

        for (int i = 0; i < 3; i++)
        {
           if (fabsf(_rayDirection[i]) < EPSILON)
           {
               // Parallel to the plane
               if (_rayOrigin[i] < _min[i] || _rayOrigin[i] > _max[i])
               {
                   // assert( !k );
                   // hits1_false++;
                   return false;
               }
           }
           else
           {
               float ood = 1.0f / _rayDirection[i];
               float t1  = (_min[i] - _rayOrigin[i]) * ood;
               float t2  = (_max[i] - _rayOrigin[i]) * ood;

               if (t1 > t2)
                   std::swap(t1, t2);

               if (t1 > tmin)
                   tmin = t1;
               if (t2 < tmax)
                   tmax = t2;

               if (tmin > tmax)
               {
                   // assert( !k );
                   // hits1_false++;
                   return false;
               }
           }
        }

        distance = tmin;

        return true;
    }

    glm::float3 IntersectFrustumPlanes(Plane p0, Plane p1, Plane p2)
    {
        glm::float3 n0 = p0.normal;
        glm::float3 n1 = p1.normal;
        glm::float3 n2 = p2.normal;

        float det = glm::dot(glm::cross(n0, n1), n2);
        return (glm::cross(n2, n1) * p0.offset + glm::cross(n0, n2) * p1.offset - glm::cross(n0, n1) * p2.offset) * (1.0f / det);
    }
    
    PlaneIntersectionFlag TestAABBToPlane(const AABB aabb, const Plane p)
    {
        glm::float3 center  = aabb.getCenter();
        glm::float3 extents = aabb.getHalfExtent();

        // Compute signed distance from plane to box center
        float sd = glm::dot(center, p.normal) - p.offset;

        // Compute the projection interval radius of b onto L(t) = b.Center + t * p.Normal
        // Projection radii r_i of the 8 bounding box vertices
        // r_i = dot((V_i - C), n)
        // r_i = dot((C +- e0*u0 +- e1*u1 +- e2*u2 - C), n)
        // Cancel C and distribute dot product
        // r_i = +-(dot(e0*u0, n)) +-(dot(e1*u1, n)) +-(dot(e2*u2, n))
        // We take the maximum position radius by taking the absolute value of the terms, we assume Extents to be
        // positive r = e0*|dot(u0, n)| + e1*|dot(u1, n)| + e2*|dot(u2, n)| When the separating axis vector Normal is
        // not a unit vector, we need to divide the radii by the length(Normal) u0,u1,u2 are the local axes of the box,
        // which is = [(1,0,0), (0,1,0), (0,0,1)] respectively for axis aligned bb
        float r = glm::dot(extents, abs(p.normal));

        if (sd > r)
        {
           return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        }
        if (sd < -r)
        {
           return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        }
        return PLANE_INTERSECTION_INTERSECTING;
    }

    PlaneIntersectionFlag TestBSphereToPlane(const BSphere bsphere, const Plane p)
    {
        // Compute signed distance from plane to sphere center
        float sd = glm::dot(bsphere.center, p.normal) - p.offset;
        if (sd > bsphere.radius)
        {
           return PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        }
        if (sd < -bsphere.radius)
        {
           return PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        }
        return PLANE_INTERSECTION_INTERSECTING;
    }

    ContainmentFlag IsFrustumContainAABB(const Frustum f, const AABB& aabb)
    {
        int  p0         = TestAABBToPlane(aabb, f.Left);
        int  p1         = TestAABBToPlane(aabb, f.Right);
        int  p2         = TestAABBToPlane(aabb, f.Bottom);
        int  p3         = TestAABBToPlane(aabb, f.Top);
        int  p4         = TestAABBToPlane(aabb, f.Near);
        int  p5         = TestAABBToPlane(aabb, f.Far);
        bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

        if (anyOutside)
        {
           return CONTAINMENT_DISJOINT;
        }

        if (allInside)
        {
           return CONTAINMENT_CONTAINS;
        }

        return CONTAINMENT_INTERSECTS;
    }

    ContainmentFlag IsFrustumContainBSphere(const Frustum f, const BSphere& bsphere)
    {
        int  p0         = TestBSphereToPlane(bsphere, f.Left);
        int  p1         = TestBSphereToPlane(bsphere, f.Right);
        int  p2         = TestBSphereToPlane(bsphere, f.Bottom);
        int  p3         = TestBSphereToPlane(bsphere, f.Top);
        int  p4         = TestBSphereToPlane(bsphere, f.Near);
        int  p5         = TestBSphereToPlane(bsphere, f.Far);
        bool anyOutside = p0 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p1 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p2 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p3 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p4 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        anyOutside |= p5 == PLANE_INTERSECTION_NEGATIVE_HALFSPACE;
        bool allInside = p0 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p1 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p2 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p3 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p4 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;
        allInside &= p5 == PLANE_INTERSECTION_POSITIVE_HALFSPACE;

        if (anyOutside)
        {
           return CONTAINMENT_DISJOINT;
        }

        if (allInside)
        {
           return CONTAINMENT_CONTAINS;
        }

        return CONTAINMENT_INTERSECTS;
    }

    // Extract frustum planes from a DirectX-style MVP matrix
    static Frustum ExtractPlanesDX(const glm::float4x4& mvp)
    {
        Frustum frustum;

        // Since GLM uses column-major storage, we transpose to work with row-major convention
        glm::float4x4 m = glm::transpose(mvp);

        // Left clipping plane: column4 + column1
        frustum.Left.normal.x = m[3][0] + m[0][0];
        frustum.Left.normal.y = m[3][1] + m[0][1];
        frustum.Left.normal.z = m[3][2] + m[0][2];
        frustum.Left.offset = m[3][3] + m[0][3];

        // Right clipping plane: column4 - column1
        frustum.Right.normal.x = m[3][0] - m[0][0];
        frustum.Right.normal.y = m[3][1] - m[0][1];
        frustum.Right.normal.z = m[3][2] - m[0][2];
        frustum.Right.offset = m[3][3] - m[0][3];

        // Bottom clipping plane: column4 + column2
        frustum.Bottom.normal.x = m[3][0] + m[1][0];
        frustum.Bottom.normal.y = m[3][1] + m[1][1];
        frustum.Bottom.normal.z = m[3][2] + m[1][2];
        frustum.Bottom.offset = m[3][3] + m[1][3];

        // Top clipping plane: column4 - column2
        frustum.Top.normal.x = m[3][0] - m[1][0];
        frustum.Top.normal.y = m[3][1] - m[1][1];
        frustum.Top.normal.z = m[3][2] - m[1][2];
        frustum.Top.offset = m[3][3] - m[1][3];

        // Near clipping plane (DirectX convention: z_clip = 0) = column3
        frustum.Near.normal.x = m[2][0];
        frustum.Near.normal.y = m[2][1];
        frustum.Near.normal.z = m[2][2];
        frustum.Near.offset = m[2][3];

        // Far clipping plane (DirectX convention: z_clip = 1) = column4 - column3
        frustum.Far.normal.x = m[3][0] - m[2][0];
        frustum.Far.normal.y = m[3][1] - m[2][1];
        frustum.Far.normal.z = m[3][2] - m[2][2];
        frustum.Far.offset = m[3][3] - m[2][3];

        // Normalize all planes (crucial for correct distance calculations)
        auto NormalizePlane = [](Plane& plane) {
            float length = glm::length(plane.normal);
            // Avoid division by zero
            if (length > 0.0f) {
                plane.normal /= length;
                plane.offset /= length;
            }
            };

        NormalizePlane(frustum.Left);
        NormalizePlane(frustum.Right);
        NormalizePlane(frustum.Bottom);
        NormalizePlane(frustum.Top);
        NormalizePlane(frustum.Near);
        NormalizePlane(frustum.Far);

        return frustum;
    }

    void UpdateFrustumCorners(Frustum& frustum)
    {
        // Compute corners from the planes instead of projection matrix. Otherwise you get the same issue with near and far for oblique projection.
        frustum.corners[0] = IntersectFrustumPlanes(frustum.planes[0], frustum.planes[3], frustum.planes[4]);
        frustum.corners[1] = IntersectFrustumPlanes(frustum.planes[1], frustum.planes[3], frustum.planes[4]);
        frustum.corners[2] = IntersectFrustumPlanes(frustum.planes[0], frustum.planes[2], frustum.planes[4]);
        frustum.corners[3] = IntersectFrustumPlanes(frustum.planes[1], frustum.planes[2], frustum.planes[4]);
        frustum.corners[4] = IntersectFrustumPlanes(frustum.planes[0], frustum.planes[3], frustum.planes[5]);
        frustum.corners[5] = IntersectFrustumPlanes(frustum.planes[1], frustum.planes[3], frustum.planes[5]);
        frustum.corners[6] = IntersectFrustumPlanes(frustum.planes[0], frustum.planes[2], frustum.planes[5]);
        frustum.corners[7] = IntersectFrustumPlanes(frustum.planes[1], frustum.planes[2], frustum.planes[5]);
    }

    Plane ComputePlane(glm::float3 a, glm::float3 b, glm::float3 c)
    {
        Plane plane;
        plane.normal = normalize(cross(b - a, c - a));
        plane.offset = dot(plane.normal, a);
        return plane;
    }

    Plane ComputePlane(glm::float3 position, glm::float3 normal)
    {
        Plane plane;
        plane.normal = normal;
        plane.offset = glm::dot(normal, position);
        return plane;
    }

    Plane CameraSpacePlane(glm::float4x4 worldToCamera, glm::float3 positionWS, glm::float3 normalWS, float sideSign, float clipPlaneOffset)
    {
        glm::float3 offsetPosWS = positionWS + normalWS * clipPlaneOffset;
        glm::float3 posCS = worldToCamera * glm::float4(offsetPosWS, 1);
        glm::float3 normalCS = worldToCamera * glm::float4(normalWS, 0) * sideSign;
        Plane plane;
        plane.normal = normalCS;
        plane.offset = glm::dot(posCS, normalCS);
        return plane;
    }
    
    bool BSphere::Intersects(BSphere other)
    {
        // The distance between the sphere centers is computed and compared
        // against the sum of the sphere radii. To avoid an square root operation, the
        // squared distances are compared with squared sum radii instead.
        glm::float3 d         = center - other.center;
        float       dist2     = dot(d, d);
        float       radiusSum = radius + other.radius;
        float       r2        = radiusSum * radiusSum;
        return dist2 <= r2;
    }

    //------------------------------------------------------------------------------------------

    OrientedBBox OrientedBBoxFromRTS(glm::float4x4 trs)
    {
        glm::float3 vecX = glm::column(trs, 0);
        glm::float3 vecY = glm::column(trs, 1);
        glm::float3 vecZ = glm::column(trs, 2);

        OrientedBBox obb;
        
        obb.center = glm::column(trs, 3);
        obb.right = vecX * (1.0f / glm::length(vecX));
        obb.up = vecY * (1.0f / glm::length(vecY));

        obb.extentX = 0.5f * glm::length(vecX);
        obb.extentY = 0.5f * glm::length(vecY);
        obb.extentZ = 0.5f * glm::length(vecZ);

        return obb;
    }

    // https://iquilezles.org/www/articles/distfunctions/distfunctions.htm
    float DistanceToOriginAABB(glm::float3 point, glm::float3 aabbSize)
    {
        glm::float3 q = abs(point) - glm::float3(aabbSize);
        return glm::length(max(q, 0.0f)) + glm::min(glm::max(q.x, glm::max(q.y, q.z)), 0.0f);
    }

    // Optimized version of https://www.sciencedirect.com/topics/computer-science/oriented-bounding-box
    float DistanceToOBB(OrientedBBox obb, glm::float3 point)
    {
        glm::float3 offset = point - obb.center;
        glm::float3 boxForward = normalize(cross(obb.right, obb.up));
        glm::float3 axisAlignedPoint = glm::float3(dot(offset, normalize(obb.right)), dot(offset, normalize(obb.up)), dot(offset, boxForward));

        return DistanceToOriginAABB(axisAlignedPoint, glm::float3(obb.extentX, obb.extentY, obb.extentZ));
    }

    AABB OBBToAABB(glm::float3 right, glm::float3 up, glm::float3 forward, glm::float3 extent, glm::float3 center)
    {
        glm::float3 aabbExtents = abs(right * extent.x) + abs(up * extent.y) + abs(forward * extent.z);
        AABB aabb;
        aabb.update(center, aabbExtents);
        return aabb;
    }
    
    //------------------------------------------------------------------------------------------

    const Color Color::White(1.f, 1.f, 1.f, 1.f);
    const Color Color::Black(0.f, 0.f, 0.f, 0.f);
    const Color Color::Red(1.f, 0.f, 0.f, 1.f);
    const Color Color::Green(0.f, 1.f, 0.f, 1.f);
    const Color Color::Blue(0.f, 0.f, 1.f, 1.f);
    
    Color Color::ToSRGB() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf(c[i], 1.0f / 2.4f) * 1.055f - 0.055f;
            if (c[i] < 0.0031308f)
                _o = c[i] * 12.92f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::FromSRGB() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf((c[i] + 0.055f) / 1.055f, 2.4f);
            if (c[i] < 0.0031308f)
                _o = c[i] / 12.92f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::ToREC709() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf(c[i], 0.45f) * 1.099f - 0.099f;
            if (c[i] < 0.0018f)
                _o = c[i] * 4.5f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    Color Color::FromREC709() const
    {
        float c[3] = {r, g, b};
        float o[3] = {r, g, b};
        for (size_t i = 0; i < 3; i++)
        {
            float _o = powf((c[i] + 0.099f) / 1.099f, 1.0f / 0.45f);
            if (c[i] < 0.0081f)
                _o = c[i] / 4.5f;
            o[i] = _o;
        }
        return Color(o[0], o[1], o[2], a);
    }

    uint32_t Color::R10G10B10A2() const
    {
        return 0;
    }

    uint32_t Color::R8G8B8A8() const
    {
        return 0;
    }

    uint32_t Color::R11G11B10F(bool RoundToEven) const
    {
        const float kMaxVal   = float(1 << 16);
        const float kF32toF16 = (1.0 / (1ull << 56)) * (1.0 / (1ull << 56));

        union
        {
            float    f;
            uint32_t u;
        } R, G, B;

        R.f = glm::clamp(r, 0.0f, kMaxVal) * kF32toF16;
        G.f = glm::clamp(g, 0.0f, kMaxVal) * kF32toF16;
        B.f = glm::clamp(b, 0.0f, kMaxVal) * kF32toF16;

        if (RoundToEven)
        {
            // Bankers rounding:  2.5 -> 2.0  ;  3.5 -> 4.0
            R.u += 0x0FFFF + ((R.u >> 16) & 1);
            G.u += 0x0FFFF + ((G.u >> 16) & 1);
            B.u += 0x1FFFF + ((B.u >> 17) & 1);
        }
        else
        {
            // Default rounding:  2.5 -> 3.0  ;  3.5 -> 4.0
            R.u += 0x00010000;
            G.u += 0x00010000;
            B.u += 0x00020000;
        }

        R.u &= 0x0FFE0000;
        G.u &= 0x0FFE0000;
        B.u &= 0x0FFC0000;

        return R.u >> 17 | G.u >> 6 | B.u << 4;
    }

    uint32_t Color::R9G9B9E5() const
    {
        const float kMaxVal = float(0x1FF << 7);
        const float kMinVal = float(1.f / (1 << 16));

        // Clamp RGB to [0, 1.FF*2^16]
        float _r = glm::clamp(r, 0.0f, kMaxVal);
        float _g = glm::clamp(g, 0.0f, kMaxVal);
        float _b = glm::clamp(b, 0.0f, kMaxVal);

        // Compute the maximum channel, no less than 1.0*2^-15
        float MaxChannel = glm::max(glm::max(_r, _g), glm::max(_b, kMinVal));

        // Take the exponent of the maximum channel (rounding up the 9th bit) and
        // add 15 to it.  When added to the channels, it causes the implicit '1.0'
        // bit and the first 8 mantissa bits to be shifted down to the low 9 bits
        // of the mantissa, rounding the truncated bits.
        union
        {
            float   f;
            int32_t i;
        } R, G, B, E;
        E.f = MaxChannel;
        E.i += 0x07804000; // Add 15 to the exponent and 0x4000 to the mantissa
        E.i &= 0x7F800000; // Zero the mantissa

        // This shifts the 9-bit values we need into the lowest bits, rounding as
        // needed.  Note that if the channel has a smaller exponent than the max
        // channel, it will shift even more.  This is intentional.
        R.f = _r + E.f;
        G.f = _g + E.f;
        B.f = _b + E.f;

        // Convert the Bias to the correct exponent in the upper 5 bits.
        E.i <<= 4;
        E.i += 0x10000000;

        // Combine the fields.  RGB floats have unwanted data in the upper 9
        // bits.  Only red needs to mask them off because green and blue shift
        // it out to the left.
        return E.i | B.i << 18 | G.i << 9 | R.i & 511;
    }

    // == Random ======================================================================================

    void Random::SetSeed(glm::uint32 seed)
    {
        engine.seed(seed);
    }

    void Random::SeedWithRandomValue()
    {
        std::random_device device;
        engine.seed(device());
    }

    glm::uint32 Random::RandomUint()
    {
        return engine();
    }

    float Random::RandomFloat()
    {
        // return distribution(engine);
        return (RandomUint() & 0xFFFFFF) / float(1 << 24);
    }

    glm::float2 Random::RandomFloat2()
    {
        return glm::float2(RandomFloat(), RandomFloat());
    }

} // namespace MoYu