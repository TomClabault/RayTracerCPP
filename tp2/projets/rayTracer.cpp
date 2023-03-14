#include "rayTracer.h"

RayTracer::RayTracer(unsigned int width, unsigned int height, Scene scene) : _width(width), _height(height),
	_scene(scene), _image(Image(width, height)) {}

Image* RayTracer::getImage()
{
	return &_image;
}

void RayTracer::trace()
{
#pragma omp parallel for
	for (int py = 0; py < _height; py++)
	{
		float y_world = (float)py / _height * 2 - 1;

		for (int px = 0; px < _width; px++)
		{
			float x_world = (float)px / _width * 2 - 1;
			x_world *= (float)_width / _height;

			Vector image_plane_point = Vector(x_world, y_world, -1);

			Vector camera_position = _scene._camera._position;
			Vector ray_direction = normalize(Vector(image_plane_point - camera_position));
			Ray ray(camera_position, ray_direction);

			HitInfo finalHitInfo;
			HitInfo hit_info;
			for (Triangle& triangle : _scene._triangles)
				if (triangle.intersect(ray, hit_info))
					if (hit_info.t < finalHitInfo.t || finalHitInfo.t == -1)
						finalHitInfo = hit_info;

			Color finalColor;

			if (finalHitInfo.t > 0)//We found an intersection
			{
#if SHADING
				Vector inter_point = ray._origin + ray._direction * hit_info.t;
				Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
				Vector normal = normalize(hit_info.normal_at_intersection);

				if (px == 186 && py == 1024 - 577)
				std::cout << normal << std::endl;

				Material hit_material = _scene._materials(finalHitInfo.mat_index);
				finalColor = hit_material.diffuse * dot(normal, direction_to_light);

				if (px == 186 && py == 1024 - 577)
					std::cout << hit_material.diffuse << ", " << dot(normal, direction_to_light) << std::endl;
#else
#if COLOR_NORMAL_OR_BARYCENTRIC //Color triangles with normal
				Vector normalized_normal = normalize(finalHitInfo.normal_at_intersection);
				finalColor = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
#else //Color triangles with barycentric coordinates
				float u = finalHitInfo.u;
				float v = finalHitInfo.v;

				finalColor = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
#endif
#endif
			}
			else
				finalColor = Color(0, 0, 0);//Black since we didn't found any intersection

			_image(px, py) = finalColor;
		}
	}
}