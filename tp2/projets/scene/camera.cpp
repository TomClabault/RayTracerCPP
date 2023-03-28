#include <iostream>

#include "camera.h"

void Camera::init_perspec_proj_mat(float aspect_ratio)
{
	_perspective_proj_mat = Perspective(_fov, aspect_ratio, _near, _far);
	_perspective_proj_mat_inv = _perspective_proj_mat.inverse();

	_aspect_ratio = aspect_ratio;
}

std::ostream& operator << (std::ostream& os, const Camera& camera)
{
	os << "Camera[" << camera._position << ", [fovY, near, far] = [" << camera._fov << ", " << camera._near << ", " << camera._far << "]\n";

	return os;
}