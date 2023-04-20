#include "skybox.h"

Skybox::Skybox() {}

Skybox::Skybox(const Image faces[6])
{
    for (int i = 0; i < 6; i++)
        _faces[i] = faces[i];
}

//Adadpted from https://www.gamedev.net/forums/topic/687535-implementing-a-cube-map-lookup-function/5337472/
Color Skybox::sample(const Vector& direction) const
{
    Vector direction2 = Vector(direction.x, direction.y, -direction.z);
    Vector dir_abs = Vector(std::abs(direction2.x), std::abs(direction2.y), std::abs(direction2.z));

    int face_index;
    float norm_factor;
    float u, v;
    if(dir_abs.z >= dir_abs.x && dir_abs.z >= dir_abs.y)
    {
        face_index = direction2.z < 0.0 ? 4.0 : 5.0;

        norm_factor = 0.5 / dir_abs.z;

        u = direction2.z < 0.0 ? -direction2 .x : direction2.x;
        v = -direction2.y;
    }
    else if(dir_abs.y >= dir_abs.x)
    {
        face_index = direction2.y < 0.0 ? 3.0 : 2.0;

        norm_factor = 0.5 / dir_abs.y;

        u = direction2.x;
        v = direction2.y < 0.0 ? -direction2.z : direction2.z;
    }
    else
    {
        face_index = direction2.x < 0.0 ? 1.0 : 0.0;

        norm_factor = 0.5 / dir_abs.x;

        u = direction2.x < 0.0 ? direction2.z : -direction2.z;
        v = -direction2.y;
    }

    u = u * norm_factor + 0.5;
    v = v * norm_factor + 0.5;

    return _faces[face_index].texture(u, v);

//    //Determines which face of the cube map the direction vector intersects
//    int face_index;
//    if(vAbs.z >= vAbs.x && vAbs.z >= vAbs.y)
//        face_index = v.z < 0.0 ? 5.0 : 4.0;
//    else if(vAbs.y >= vAbs.x)
//        face_index = v.y < 0.0 ? 3.0 : 2.0;
//    else
//        face_index = v.x < 0.0 ? 1.0 : 0.0;

//    Vector mapped_vector;
//    //Map the direction vector onto the face of the cube map
//    if (face_index == 0)
//        mapped_vector = v / std::abs(v.x);
//    else if (face_index == 1)
//        mapped_vector = v / abs(v.y);
//    else if (face_index == 2)
//        mapped_vector = v / abs(v.z);

//    //Convert the mapped direction vector to texture coordinates
//    float u = 0.5 * mapped_vector.x + 0.5;
//    float vv = 0.5 * mapped_vector.y + 0.5;

//    //Sample the cube map using the texture coordinates
//    return _faces[face_index].texture(u, vv);
}
