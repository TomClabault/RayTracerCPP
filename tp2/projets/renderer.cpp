#include "renderer.h"

Material init_default_material()
{
	Material mat = Material(Color(1.0f, 0.0f, 0.5f));//Pink color
	mat.specular = Color(1.0f, 1.0f, 1.0f);
	mat.ns = 15;

	return mat;
}

Material Renderer::DEFAULT_MATERIAL = init_default_material();
Color Renderer::AMBIENT_COLOR = Color(0.1f, 0.1f, 0.1f);

Renderer::Renderer(int width, int height, Scene scene) : _width(width), _height(height),
	_image(Image(width, height)), _scene(scene) 
{
#if HYBRID_RASTERIZATION_TRACING
	_z_buffer = (float**)std::malloc(sizeof(float*) * height);
	if (_z_buffer == NULL)
	{
		std::cout << "Not enough memory to allocate the ZBuffer of the ray tracer..." << std::endl;
		std::exit(-1);
	}

	for (int i = 0; i < height; i++)
	{
		_z_buffer[i] = (float*)std::malloc(sizeof(float) * width);
		if (_z_buffer[i] == NULL)
		{
			std::cout << "Not enough memory to allocate the ZBuffer of the ray tracer..." << std::endl;
			std::exit(-1);
		}

		std::memset(_z_buffer[i], std::numeric_limits<float>::max(), width * sizeof(float));
	}
#endif
}

Image* Renderer::getImage()
{
	return &_image;
}

Color Renderer::computeDiffuse(const Material& hitMaterial, const Vector& normal, const Vector& direction_to_light) const
{
	return hitMaterial.diffuse * std::max(0.0f, dot(normal, direction_to_light));
}

Color Renderer::computeSpecular(const Material& hit_material, const Vector& ray_direction, const Vector& normal, const Vector& direction_to_light) const
{
	Vector incident_direction = -direction_to_light;
	Vector reflection_ray = incident_direction - 2 * dot(normal, incident_direction) * normal;

	return hit_material.specular * std::max(0.0f, std::pow(dot(reflection_ray, -ray_direction), hit_material.ns));
}

bool Renderer::isShadowed(const Vector& inter_point, const Vector& light_position) const
{
	Ray ray(inter_point, light_position - inter_point);
	HitInfo hitInfo;

	for (const Triangle& triangle : _scene._triangles)
	{
		if (triangle.intersect(ray, hitInfo))
		{
			Vector new_inter_point = ray._origin + ray._direction * hitInfo.t;

			if (distance(Point(inter_point), Point(new_inter_point)) < distance(Point(inter_point), Point(light_position)))
			{
				//We found an object that is between the light and the current inter_point: the point is shadowed

				return true;
			}
		}
	}

	//We haven't found any object between the light source and the intersection point, the point isn't shadowed
	return false;
}

Color Renderer::traceTriangle(const Ray& ray, const Triangle& triangle) const
{
	HitInfo hit_info;
	Color finalColor = Color(0, 0, 0);

	if (triangle.intersect(ray, hit_info))
	{
#if SHADING
		Vector inter_point = ray._origin + ray._direction * (hit_info.t + Renderer::EPSILON);
		Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
		Vector normal = normalize(hit_info.normal_at_intersection);

		Material hit_material;
		if (hit_info.mat_index == -1)
			hit_material = Renderer::DEFAULT_MATERIAL;
		else
			hit_material = _scene._materials(hit_info.mat_index);

		finalColor = finalColor + computeDiffuse(hit_material, normal, direction_to_light);
		finalColor = finalColor + computeSpecular(hit_material, ray._direction, normal, direction_to_light);
		if (isShadowed(inter_point, _scene._point_light._position))
			finalColor = finalColor * Color(Renderer::SHADOW_INTENSITY);

		finalColor = finalColor + Renderer::AMBIENT_COLOR;
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

	return finalColor;
}

void Renderer::rasterTrace()
{
	for (const Triangle& triangle : _scene._triangles)
	{
#pragma omp parallel for
		for (int py = 0; py < _height; py++)
		{
			float image_y = py / (float)_height * 2 - 1;
			for (int px = 0; px < _width; px++)
			{
				float image_x = px / (float)_width * 2 - 1;

				Triangle NDC_triangle = Triangle(triangle._a / -triangle._a.z, triangle._b / -triangle._b.z, triangle._c / -triangle._c.z);
				if (NDC_triangle.inside_outside_2D(Vector(image_x, image_y, -1)))
					_image(px, py) = traceTriangle(Ray(_scene._camera._position, normalize(Vector(image_x, image_y, -1) - _scene._camera._position)), triangle);
			}
		}
	}
}

void Renderer::rayTrace()
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

			Color finalColor = Color(0, 0, 0);

			if (finalHitInfo.t > 0)//We found an intersection
			{
#if SHADING
				Vector inter_point = ray._origin + ray._direction * (finalHitInfo.t + Renderer::EPSILON);
				Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
				Vector normal = normalize(finalHitInfo.normal_at_intersection);

				Material hit_material;
				if (finalHitInfo.mat_index == -1)
					hit_material = Renderer::DEFAULT_MATERIAL;
				else
					hit_material = _scene._materials(finalHitInfo.mat_index);

				finalColor = finalColor + computeDiffuse(hit_material, normal, direction_to_light);
				finalColor = finalColor + computeSpecular(hit_material, ray._direction, normal, direction_to_light);
				if (isShadowed(inter_point, _scene._point_light._position))
					finalColor = finalColor * Color(Renderer::SHADOW_INTENSITY);

				finalColor = finalColor + Renderer::AMBIENT_COLOR;
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
				finalColor = Color(0.0, 0.0, 0);//Black since we didn't found any intersection

			_image(px, py) = finalColor;
		}
	}
}