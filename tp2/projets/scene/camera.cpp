#include <iostream>

#include "camera.h"

void Camera::set_aspect_ratio(float aspect_ratio)
{
    _aspect_ratio = aspect_ratio;

    _perspective_proj_mat = Perspective(_fov, _aspect_ratio, _near, _far);
	_perspective_proj_mat_inv = _perspective_proj_mat.inverse();
}

void Camera::set_fov(float fov)
{
    _fov = fov;

    _perspective_proj_mat = Perspective(_fov, _aspect_ratio, _near, _far);
    _perspective_proj_mat_inv = _perspective_proj_mat.inverse();
}

std::ostream& operator << (std::ostream& os, const Camera& camera)
{
    os << "Camera[" << camera._position << ", [fovY, near, far] = [" << camera._fov << ", " << camera._near << ", " << camera._far << "]\n";

	return os;
}
