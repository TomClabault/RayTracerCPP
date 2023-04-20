#ifndef SKYBOX_H
#define SKYBOX_H

#include "image.h"
#include "vec.h"

class Skybox
{
public:
    Skybox();

    /**
     * @brief Loads the skybox from the given 6 faces
     * @param faces Faces order is right, left, top, bottom, back, front
     */
    Skybox(const Image faces[6]);

    Color sample(const Vector& direction) const;

private:
    Image _faces[6];
};

#endif
