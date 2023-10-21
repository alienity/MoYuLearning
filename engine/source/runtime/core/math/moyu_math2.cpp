﻿#include "moyu_math2.h"

namespace MoYu
{
    //------------------------------------------------------------------------------------------

    /*
     * returns n! / d!
     */
    static constexpr float factorial(size_t n, size_t d) {
       d = std::max(size_t(1), d);
       n = std::max(size_t(1), n);
       float r = 1.0;
       if (n == d) {
           // intentionally left blank
       } else if (n > d) {
           for ( ; n>d ; n--) {
               r *= n;
           }
       } else {
           for ( ; d>n ; d--) {
               r *= d;
           }
           r = 1.0f / r;
       }
       return r;
    }

    //------------------------------------------------------------------------------------------
    


    //------------------------------------------------------------------------------------------

    MFloat2 MYFloat2::Zero(0.f, 0.f);
    MFloat2 MYFloat2::One(1.f, 1.f);
    MFloat2 MYFloat2::UnitX(1.f, 0.f);
    MFloat2 MYFloat2::UnitY(0.f, 1.f);

    MFloat3 MYFloat3::Zero(0.f, 0.f, 0.f);
    MFloat3 MYFloat3::One(1.f, 1.f, 1.f);
    MFloat3 MYFloat3::UnitX(1.f, 0.f, 0.f);
    MFloat3 MYFloat3::UnitY(0.f, 1.f, 0.f);
    MFloat3 MYFloat3::UnitZ(0.f, 0.f, 1.f);
    MFloat3 MYFloat3::Up(0.f, 1.f, 0.f);
    MFloat3 MYFloat3::Down(0.f, -1.f, 0.f);
    MFloat3 MYFloat3::Right(1.f, 0.f, 0.f);
    MFloat3 MYFloat3::Left(-1.f, 0.f, 0.f);
    MFloat3 MYFloat3::Forward(0.f, 0.f, -1.f);
    MFloat3 MYFloat3::Backward(0.f, 0.f, 1.f);

    MFloat4 MYFloat4::Zero(0.f, 0.f, 0.f, 0.f);
    MFloat4 MYFloat4::One(1.f, 1.f, 1.f, 1.f);
    MFloat4 MYFloat4::UnitX(1.f, 0.f, 0.f, 0.f);
    MFloat4 MYFloat4::UnitY(0.f, 1.f, 0.f, 0.f);
    MFloat4 MYFloat4::UnitZ(0.f, 0.f, 1.f, 0.f);
    MFloat4 MYFloat4::UnitW(0.f, 0.f, 0.f, 1.f);

    MMatrix3x3 MYMatrix3x3::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    MMatrix3x3 MYMatrix3x3::Identity(1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 1.f);

    MMatrix4x4 MYMatrix4x4::Zero(0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f);
    MMatrix4x4 MYMatrix4x4::Identity(1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, 0.f, 1.f);

    MQuaternion MYQuaternion::Identity(1.f, 0.f, 0.f, 0.f);

    //------------------------------------------------------------------------------------------

    MMatrix4x4 MYMatrix4x4::createPerspectiveFieldOfView(float fovY, float aspectRatio, float zNearPlane, float zFarPlane)
    {
       MMatrix4x4 m = Zero;

        m[0][0] = 1.0f / (aspectRatio * std::tan(fovY * 0.5f));
        m[1][1] = 1.0f / std::tan(fovY * 0.5f);
        m[2][2] = zNearPlane / (zFarPlane - zNearPlane);
        m[3][2] = zFarPlane * zNearPlane / (zFarPlane - zNearPlane);
        m[2][3] = -1;

        return m;
    }

    MMatrix4x4 MYMatrix4x4::createPerspective(float width, float height, float zNearPlane, float zFarPlane)
    {
        return createPerspectiveOffCenter(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNearPlane, zFarPlane);
    }

    MMatrix4x4 MYMatrix4x4::createPerspectiveOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
    {
        MMatrix4x4 m = Zero;

        m[0][0] = 2 * zNearPlane / (right - left);
        m[2][0] = (right + left) / (right - left);
        m[1][1] = 2 * zNearPlane / (top - bottom);
        m[2][1] = (top + bottom) / (top - bottom);
        m[2][2] = zNearPlane / (zFarPlane - zNearPlane);
        m[3][2] = zFarPlane * zNearPlane / (zFarPlane - zNearPlane);
        m[2][3] = -1;

        return m;
    }

    MMatrix4x4 MYMatrix4x4::createOrthographic(float width, float height, float zNearPlane, float zFarPlane)
    {
        return createOrthographicOffCenter(-width * 0.5f, width * 0.5f, -height * 0.5f, height * 0.5f, zNearPlane, zFarPlane);
    }

    MMatrix4x4 MYMatrix4x4::createOrthographicOffCenter(float left, float right, float bottom, float top, float zNearPlane, float zFarPlane)
    {
        MMatrix4x4 m = Zero;

        m[0][0] = 2 / (right - left);
        m[3][0] = -(right + left) / (right - left);
        m[1][1] = 2 / (top - bottom);
        m[3][1] = -(top + bottom) / (top - bottom);
        m[2][2] = 1 / (zFarPlane - zNearPlane);
        m[3][2] = zFarPlane / (zFarPlane - zNearPlane);
        m[3][3] = 1;

        return m;
    }

    MMatrix4x4 MYMatrix4x4::createLookAtMatrix(const MFloat3& eye, const MFloat3& center, const MFloat3& up)
    {
        glm::mat lookAtMat = glm::lookAtRH(eye, center, up);
        return lookAtMat;
    }

    MMatrix4x4 MYMatrix4x4::createViewMatrix(const MFloat3& position, const MQuaternion& orientation)
    {
        glm::mat r_inverse  = glm::toMat4(glm::inverse(orientation));
        glm::mat t_inverse  = glm::translate(glm::mat4(1.0), -position);
        glm::mat tr_inverse = r_inverse * t_inverse;
        return tr_inverse;
    }

    MMatrix4x4 MYMatrix4x4::createWorldMatrix(const MFloat3& position, const MQuaternion& orientation, const MFloat3& scale)
    {
        glm::mat t = glm::translate(glm::mat4(1.0), position);
        glm::mat r = glm::toMat4(orientation);
        glm::mat s = glm::scale(glm::mat4(1.0), scale);
        glm::mat trs = t * r * s;
        return trs;
    }

    MMatrix4x4 MYMatrix4x4::makeTransform(const MFloat3& position, const MQuaternion& orientation, const MFloat3& scale)
    {
        glm::mat t = glm::translate(glm::mat4(1.0), position);
        glm::mat r = glm::toMat4(orientation);
        glm::mat s = glm::scale(glm::mat4(1.0), scale);
        glm::mat trs = t * r * s;
        return trs;
    }

    MMatrix4x4 MYMatrix4x4::makeInverseTransform(const MFloat3& position, const MQuaternion& orientation, const MFloat3& scale)
    {
        glm::mat t_inverse = glm::translate(glm::mat4(1.0), -position);
        glm::mat r_inverse = glm::toMat4(glm::inverse(orientation));
        glm::mat s_inverse = glm::scale(glm::mat4(1.0), 1.0f / scale);
        glm::mat trs_inverse = s_inverse * r_inverse * t_inverse;
        return trs_inverse;
    }

    //------------------------------------------------------------------------------------------

    AxisAlignedBox::AxisAlignedBox(const MFloat3& center, const MFloat3& half_extent) { update(center, half_extent); }

    void AxisAlignedBox::merge(const AxisAlignedBox& axis_aligned_box)
    { 
        merge(axis_aligned_box.getMinCorner());
        merge(axis_aligned_box.getMaxCorner());
    }

    void AxisAlignedBox::merge(const MFloat3& new_point)
    {
        m_min_corner = glm::min(m_min_corner, new_point);
        m_max_corner = glm::max(m_max_corner, new_point);

        m_center      = 0.5f * (m_min_corner + m_max_corner);
        m_half_extent = m_center - m_min_corner;
    }

    void AxisAlignedBox::update(const MFloat3& center, const MFloat3& half_extent)
    {
        m_center      = center;
        m_half_extent = half_extent;
        m_min_corner  = center - half_extent;
        m_max_corner  = center + half_extent;
    }

    //------------------------------------------------------------------------------------------

    const Color Color::White(1.f, 1.f, 1.f, 1.f);
    const Color Color::Black(0.f, 0.f, 0.f, 0.f);
    const Color Color::Red(1.f, 0.f, 0.f, 1.f);
    const Color Color::Green(0.f, 1.f, 0.f, 1.f);
    const Color Color::Blue(0.f, 1.f, 0.f, 1.f);

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
        static const float kMaxVal   = float(1 << 16);
        static const float kF32toF16 = (1.0 / (1ull << 56)) * (1.0 / (1ull << 56));

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
        static const float kMaxVal = float(0x1FF << 7);
        static const float kMinVal = float(1.f / (1 << 16));

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

} // namespace MoYuMath