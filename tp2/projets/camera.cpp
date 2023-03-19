#include <iostream>

#include "camera.h"

std::ostream& operator << (std::ostream& os, const Camera& camera)
{
	os << "Camera[" << camera._position << ", [fovY, near, far] = [" << camera._fovy << ", " << camera._near << ", " << camera._far << "]\n";

	return os;
}