#include <iostream>

#include "camera.h"

void Camera::init_perspec_proj_mat(float aspect_ratio)
{
	_perspective_proj_mat = Perspective(_fovy, aspect_ratio, _near, _far);
	_perspective_proj_mat_inv = _perspective_proj_mat.inverse();
}

std::ostream& operator << (std::ostream& os, const Camera& camera)
{
	os << "Camera[" << camera._position << ", [fovY, near, far] = [" << camera._fovy << ", " << camera._near << ", " << camera._far << "]\n";

	return os;
}