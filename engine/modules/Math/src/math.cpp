#include "math/math.hpp"

namespace astre::math
{
    Vec2Serialized serialize(const Vec2& v)
    {
        Vec2Serialized vSerialized;
        vSerialized.set_x(v.x);
        vSerialized.set_y(v.y);
        return vSerialized;
    }

    Vec3Serialized serialize(const Vec3& v)
    {
        Vec3Serialized vSerialized;
        vSerialized.set_x(v.x);
        vSerialized.set_y(v.y);
        vSerialized.set_z(v.z);
        return vSerialized;
    }

    Vec4Serialized serialize(const Vec4& v)
    {
        Vec4Serialized vSerialized;
        vSerialized.set_x(v.x);
        vSerialized.set_y(v.y);
        vSerialized.set_z(v.z);
        vSerialized.set_w(v.w);
        return vSerialized;
    }

    QuatSerialized serialize(const Quat& v)
    {
        QuatSerialized qSerialized;
        qSerialized.set_x(v.x);
        qSerialized.set_y(v.y);
        qSerialized.set_z(v.z);
        qSerialized.set_w(v.w);
        return qSerialized;
    }

    Mat2Serialized serialize(const Mat2& mat2)
    {
        Mat2Serialized mat2Serialized;
        mat2Serialized.add_data(mat2[0][0]);
        mat2Serialized.add_data(mat2[0][1]);

        mat2Serialized.add_data(mat2[1][0]);
        mat2Serialized.add_data(mat2[1][1]);
        return mat2Serialized;
    }

    Mat3Serialized serialize(const Mat3& mat3)
    {
        Mat3Serialized mat3Serialized;
        mat3Serialized.add_data(mat3[0][0]);
        mat3Serialized.add_data(mat3[0][1]);
        mat3Serialized.add_data(mat3[0][2]);

        mat3Serialized.add_data(mat3[1][0]);
        mat3Serialized.add_data(mat3[1][1]);
        mat3Serialized.add_data(mat3[1][2]);

        mat3Serialized.add_data(mat3[2][0]);
        mat3Serialized.add_data(mat3[2][1]);
        mat3Serialized.add_data(mat3[2][2]);
        return mat3Serialized;
    }

    Mat4Serialized serialize(const Mat4& mat4)
    {
        Mat4Serialized mat4Serialized;
        mat4Serialized.add_data(mat4[0][0]);
        mat4Serialized.add_data(mat4[0][1]);
        mat4Serialized.add_data(mat4[0][2]);
        mat4Serialized.add_data(mat4[0][3]);

        mat4Serialized.add_data(mat4[1][0]);
        mat4Serialized.add_data(mat4[1][1]);
        mat4Serialized.add_data(mat4[1][2]);
        mat4Serialized.add_data(mat4[1][3]);

        mat4Serialized.add_data(mat4[2][0]);
        mat4Serialized.add_data(mat4[2][1]);
        mat4Serialized.add_data(mat4[2][2]);
        mat4Serialized.add_data(mat4[2][3]);

        mat4Serialized.add_data(mat4[3][0]);
        mat4Serialized.add_data(mat4[3][1]);
        mat4Serialized.add_data(mat4[3][2]);
        mat4Serialized.add_data(mat4[3][3]);

        return mat4Serialized;
    }


    Vec2 deserialize(const Vec2Serialized& v)
    {
        Vec2 v2;
        v2.x = v.x();
        v2.y = v.y();
        return v2;
    }

    Vec3 deserialize(const Vec3Serialized& v)
    {
        Vec3 v3;
        v3.x = v.x();
        v3.y = v.y();
        v3.z = v.z();
        return v3;
    }

    Vec4 deserialize(const Vec4Serialized& v)
    {
        Vec4 v4;
        v4.x = v.x();
        v4.y = v.y();
        v4.z = v.z();
        v4.w = v.w();
        return v4;
    }

    Quat deserialize(const QuatSerialized& v)
    {
        Quat q;
        q.x = v.x();
        q.y = v.y();
        q.z = v.z();
        q.w = v.w();
        return q;
    }

    Mat2 deserialize(const Mat2Serialized& mat2Serialized)
    {
        Mat2 mat2;
        mat2[0][0] = mat2Serialized.data(0);
        mat2[0][1] = mat2Serialized.data(1);

        mat2[1][0] = mat2Serialized.data(2);
        mat2[1][1] = mat2Serialized.data(3);
        return mat2;
    }

    Mat3 deserialize(const Mat3Serialized& mat3Serialized)
    {
        Mat3 mat3;
        mat3[0][0] = mat3Serialized.data(0);
        mat3[0][1] = mat3Serialized.data(1);
        mat3[0][2] = mat3Serialized.data(2);

        mat3[1][0] = mat3Serialized.data(3);
        mat3[1][1] = mat3Serialized.data(4);
        mat3[1][2] = mat3Serialized.data(5);

        mat3[2][0] = mat3Serialized.data(6);
        mat3[2][1] = mat3Serialized.data(7);
        mat3[2][2] = mat3Serialized.data(8);
        return mat3;
    }

    Mat4 deserialize(const Mat4Serialized& mat4Serialized)
    {
        Mat4 mat4;
        mat4[0][0] = mat4Serialized.data(0);
        mat4[0][1] = mat4Serialized.data(1);
        mat4[0][2] = mat4Serialized.data(2);
        mat4[0][3] = mat4Serialized.data(3);

        mat4[1][0] = mat4Serialized.data(4);
        mat4[1][1] = mat4Serialized.data(5);
        mat4[1][2] = mat4Serialized.data(6);
        mat4[1][3] = mat4Serialized.data(7);

        mat4[2][0] = mat4Serialized.data(8);
        mat4[2][1] = mat4Serialized.data(9);
        mat4[2][2] = mat4Serialized.data(10);
        mat4[2][3] = mat4Serialized.data(11);

        mat4[3][0] = mat4Serialized.data(12);
        mat4[3][1] = mat4Serialized.data(13);
        mat4[3][2] = mat4Serialized.data(14);
        mat4[3][3] = mat4Serialized.data(15);
        return mat4;
    }

}