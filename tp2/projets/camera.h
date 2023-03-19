#ifndef CAMERA_H
#define CAMERA_H

#include <iostream>
#include <vector>

#include "mat.h"
#include "triangle.h"

struct Camera
{
	Camera(Vector position, float fov, float near, float far) : _position(position), _fovy(fov), _near(near), _far(far) {}
	Camera(Vector position, float fov) : Camera(position, fov, 1, 1000) {}
	Camera(Vector position) : Camera(position, 90, 1, 1000) {}

	void init_perspec_proj_mat(float aspect_ratio);

	Vector _position;

	float _fovy;//Vertical fov
	float _near, _far;//Near and far clipping planes

	Transform _perspective_proj_mat;//Perspective projection matrix from camera space to NDC space
	Transform _perspective_proj_mat_inv;//Inverse of the perspective projection matrix from NDC space to camera space

	friend std::ostream& operator << (std::ostream& os, const Camera& camera);
};

#endif