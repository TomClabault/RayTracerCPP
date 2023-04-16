#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>
#include <vector>

#include "mat.h"

struct Camera
{
    Camera() : _position(Point(0, 0, 0)), _fov(45), _near(0.1), _far(1000) {}
    Camera(Point position, float fov, float near, float far) : _position(position), _fov(fov), _near(near), _far(far) {}
    Camera(Point position, float fov) : Camera(position, fov, 0.1, 1000) {}
    Camera(Point position) : Camera(position, 90, 0.1, 1000) {}

    void set_aspect_ratio(float aspect_ratio);
    void set_fov(float fov);

    //Position that accounts for the transformations done to the camera
    Point _position;

	float _aspect_ratio;
	float _fov;
	float _near, _far;//Near and far clipping planes

	Transform _perspective_proj_mat;//Perspective projection matrix from camera space to NDC space
	Transform _perspective_proj_mat_inv;//Inverse of the perspective projection matrix from NDC space to camera space

    Transform _camera_to_world_mat = Identity();
    Transform _world_to_camera_mat = Identity();

	friend std::ostream& operator << (std::ostream& os, const Camera& camera);
};

#endif
