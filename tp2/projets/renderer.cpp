#include "renderer.h"

#include <mat.h>

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
	_z_buffer = new float*[height];
	if (_z_buffer == nullptr)
	{
		std::cout << "Not enough memory to allocate the ZBuffer of the ray tracer..." << std::endl;
		std::exit(-1);
	}

	for (int i = 0; i < height; i++)
	{
		_z_buffer[i] = new float[width];
		if (_z_buffer[i] == nullptr)
		{
			std::cout << "Not enough memory to allocate the ZBuffer of the ray tracer..." << std::endl;
			std::exit(-1);
		}

		for (int j = 0; j < width; j++)
			_z_buffer[i][j] = -INFINITY;
	}

	_scene._camera.init_perspec_proj_mat((float)_width / _height);//Perspective projection matrix from camera space to NDC space
#endif
}

Renderer::~Renderer()
{
#if HYBRID_RASTERIZATION_TRACING
	for (int i = 0; i < _height; i++)
		delete[] _z_buffer[i];

	delete[] _z_buffer;
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
	float angle = dot(reflection_ray, -ray_direction);

	//Specular optimization to avoid computing the exponentiation when not necessary (i.e. when it corresponds to a negligeable visual impact)
	if (angle <= hit_material.specular_threshold)//We're below the visibility threshold so we're not going to notice the specular anyway, returning no specular
		return Color(0, 0, 0);
	else
		return hit_material.specular * std::powf(std::max(0.0f, angle), hit_material.ns);
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
		//if (isShadowed(inter_point, _scene._point_light._position))
			//finalColor = finalColor * Color(Renderer::SHADOW_INTENSITY);

		finalColor = finalColor + Renderer::AMBIENT_COLOR;
#else
#if COLOR_NORMAL_OR_BARYCENTRIC //Color triangles with normal
		Vector normalized_normal = normalize(hit_info.normal_at_intersection);
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

int clip_triangle(const vec4& a, const vec4& b, const vec4& c, std::array<ClippingPlane, 6>& clipping_planes, std::array<Triangle4, 12>& clipped_triangles)
{
	for (int i = 0; i < 1; i++)
	{
		bool a_inside = 
	}
}

struct ClippingPlane
{
	ClippingPlane() {}
	ClippingPlane(Vector normal, float d) : _normal(normal), _d(d) {}
	ClippingPlane(vec4 ABCD) : _normal(Vector(ABCD.x, ABCD.y, ABCD.z)), _d(ABCD.w) {}

	Vector _normal;
	float _d;
};

//TODO utiliser les proper Point et Vector là où il faut plutôt que Vector partout
void Renderer::rasterTrace()
{
	Transform perspective_projection = _scene._camera._perspective_proj_mat;
	Transform perspective_projection_inv = _scene._camera._perspective_proj_mat_inv;

	vec4 left = vec4(vec3(-perspective_projection[0] + perspective_projection[3]), perspective_projection.m[3][0] + perspective_projection.m[3][3]);
	vec4 right = vec4(vec3(-perspective_projection[0] + perspective_projection[3]), -perspective_projection.m[3][0] + perspective_projection.m[3][3]);
	vec4 bottom = vec4(vec3(perspective_projection[1] + perspective_projection[3]), perspective_projection.m[3][1] + perspective_projection.m[3][3]);
	vec4 top = vec4(vec3(-perspective_projection[1] + perspective_projection[3]), -perspective_projection.m[3][1] + perspective_projection.m[3][3]);
	vec4 near = vec4(vec3(perspective_projection[2] + perspective_projection[3]), perspective_projection.m[3][2] + perspective_projection.m[3][3]);
	vec4 far = vec4(vec3(-perspective_projection[2] + perspective_projection[3]), -perspective_projection.m[3][2] + perspective_projection.m[3][3]);
	vec4 minus05X = vec4(vec3(1, 0, 0), -0.5);

	std::array<struct ClippingPlane, 6> clipping_planes = {
		ClippingPlane(minus05X),//TODO to remove
		ClippingPlane(left),//Left plane
		ClippingPlane(right),//Right plane
		ClippingPlane(bottom),//Bottom plane
		ClippingPlane(top),//Top plane
		ClippingPlane(near),//Near plane
		//ClippingPlane(far)//Far plane
	};

#pragma omp parallel for
	for (int triangle_index = 0; triangle_index < _scene._triangles.size(); triangle_index++)
	{
		Triangle& triangle = _scene._triangles[triangle_index];

		//Projection of the triangle on the image plane
		float invAZ = 1 / -triangle._a.z;
		float invBZ = 1 / -triangle._b.z;
		float invCZ = 1 / -triangle._c.z;

		vec4 a_image_plane4 = perspective_projection(vec4(Point(triangle._a)));//TODO remove unecessary conversion
		vec4 b_image_plane4 = perspective_projection(vec4(Point(triangle._b)));
		vec4 c_image_plane4 = perspective_projection(vec4(Point(triangle._c)));

		std::array<Triangle4, 12> clipped_triangles;
		int nb_clipped = clip_triangle(a_image_plane4, b_image_plane4, c_image_plane4, clipping_planes, clipped_triangles);

		//TODO proper clipping algorithm
		//minXPixels = std::max(0, minXPixels);
		//minYPixels = std::max(0, minYPixels);
		//maxXPixels = std::min(_width - 1, maxXPixels);
		//maxYPixels = std::min(_height - 1, maxYPixels);

		for (const Triangle4& clipped_triangle : clipped_triangles)
		{
			Triangle clipped_triangle_NDC(clipped_triangle);

			Vector a_image_plane = clipped_triangle_NDC._a;
			Vector b_image_plane = clipped_triangle_NDC._b;
			Vector c_image_plane = clipped_triangle_NDC._c;

			//Computing the bounding box of the triangle so that next, we only test pixels that are in the bounding box of the triangle
			float boundingMinX = std::min(a_image_plane.x, std::min(b_image_plane.x, c_image_plane.x));
			float boundingMinY = std::min(a_image_plane.y, std::min(b_image_plane.y, c_image_plane.y));
			float boundingMaxX = std::max(a_image_plane.x, std::max(b_image_plane.x, c_image_plane.x));
			float boundingMaxY = std::max(a_image_plane.y, std::max(b_image_plane.y, c_image_plane.y));
		
			int minXPixels = (boundingMinX + 1) * 0.5 * _width;
			int minYPixels = (boundingMinY + 1) * 0.5 * _height;
			int maxXPixels = (boundingMaxX + 1) * 0.5 * _width;
			int maxYPixels = (boundingMaxY + 1) * 0.5 * _height;

			for (int py = minYPixels; py <= maxYPixels; py++)
			{
				float image_y = py / (float)_height * 2 - 1;

				for (int px = minXPixels; px <= maxXPixels; px++)
				{
					float image_x = px / (float)_width * 2 - 1;

					Vector pixel_point(image_x, image_y, -1);

					//We don't care about the z coordinate here
					float invTriangleArea = 1 / ((b_image_plane.x - a_image_plane.x) * (c_image_plane.y - a_image_plane.y) - (b_image_plane.y - a_image_plane.y) * (c_image_plane.x - a_image_plane.x));

					float u = Triangle::edge_function(pixel_point, c_image_plane, a_image_plane);
					if (u < 0)
						continue;

					float v = Triangle::edge_function(pixel_point, a_image_plane, b_image_plane);
					if (v < 0)
						continue;

					float w = Triangle::edge_function(pixel_point, b_image_plane, c_image_plane);
					if (w < 0)
						continue;

					u *= invTriangleArea;
					v *= invTriangleArea;
					w *= invTriangleArea;

					//Depth of the point on the "real" triangle in 3D camera space
					float inv = (w * -invAZ + u * -invBZ + v * -invCZ);
					float zCameraSpace = 1 / inv;

					if (zCameraSpace > _z_buffer[py][px])
					{
						_z_buffer[py][px] = zCameraSpace;

#if SHADING
						//TODO remove unecessary point/vector conversion
						_image(px, py) = traceTriangle(Ray(_scene._camera._position, normalize(Vector(perspective_projection_inv(Point(pixel_point))) - _scene._camera._position)), perspective_projection_inv(clipped_triangle_NDC));
#elif COLOR_NORMAL_OR_BARYCENTRIC
						Vector normalized_normal = normalize(cross(triangle._b - triangle._a, triangle._c - triangle._a));
						_image(px, py) = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
#else //Color triangles with barycentric coordinates
						float u = finalHitInfo.u;
						float v = finalHitInfo.v;

						finalColor = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
#endif
					}
				}
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
			Vector ray_direction = normalize(Vector(image_plane_point - camera_position));//TODO remove unecessary conversion
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