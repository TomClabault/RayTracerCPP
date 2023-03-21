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

//TODO doc
//TODO compress this function's code
int clip_triangle_to_plane(int plane_index, int plane_sign, std::array<Triangle4, 12>& to_clip, int nb_triangles, std::array<Triangle4, 12>& out_clipped)
{
	int sum_inside;
	bool a_inside, b_inside, c_inside;

	int triangles_added = 0;//Used as an index in the out_clipped array to know where to store clipped triangles

	for (int triangle_index = 0; triangle_index < nb_triangles; triangle_index++)
	{
		Triangle4 triangle_4 = to_clip[triangle_index];
		if (plane_index == 0)//Left and right planes
		{
			a_inside = (plane_sign == 1 && (triangle_4._a.x < triangle_4._a.w)) || (plane_sign == -1 && (triangle_4._a.x > -triangle_4._a.w));
			b_inside = (plane_sign == 1 && (triangle_4._b.x < triangle_4._b.w)) || (plane_sign == -1 && (triangle_4._b.x > -triangle_4._b.w));
			c_inside = (plane_sign == 1 && (triangle_4._c.x < triangle_4._c.w)) || (plane_sign == -1 && (triangle_4._c.x > -triangle_4._c.w));
			sum_inside = a_inside + b_inside + c_inside;


			if (sum_inside == 3)//All vertices inside, nothing to clip. Keeping the triangle as is
				out_clipped[triangles_added++] = triangle_4;
			else if (sum_inside == 1)
			{
				vec4 inside_vertex, outside_1, outside_2;
				if (a_inside)
				{
					inside_vertex = triangle_4._a;
					outside_1 = triangle_4._b;
					outside_2 = triangle_4._c;
				}
				else if (b_inside)
				{
					inside_vertex = triangle_4._b;
					outside_1 = triangle_4._c;
					outside_2 = triangle_4._a;
				}
				else if (c_inside)
				{
					inside_vertex = triangle_4._c;
					outside_1 = triangle_4._a;
					outside_2 = triangle_4._b;
				}

				//We're going to create one new triangle
				float inside_dist_to_plane = (inside_vertex.x - inside_vertex.w * plane_sign);
				float outside_1_dist_to_plane = (outside_1.x - outside_1.w * plane_sign);
				float outside_2_dist_to_plane = (outside_2.x - outside_2.w * plane_sign);

				/*
				x = -1:
				w = 2
				outside = -3
				inside = 1.5

				x = -1
				inside_dist = > 0
				dist_outside = < 0

				x = 1
				inside_dist = < 0
				dist_outside = > 0
				*/

				float tP1 = outside_1_dist_to_plane / (outside_1_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the first clipping point in the direction of insde_vertex
				float tP2 = outside_2_dist_to_plane / (outside_2_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the second clipping point in the direction of insde_vertex

				//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
				vec4 P1 = outside_1 + (tP1 - Triangle::EPSILON) * (inside_vertex - outside_1);
				vec4 P2 = outside_2 + (tP2 - Triangle::EPSILON) * (inside_vertex - outside_2);

				//Creating the new triangle
				out_clipped[triangles_added++] = Triangle4(inside_vertex, P1, P2);
			}
			else if (sum_inside == 2)
			{
				vec4 inside_1, inside_2, outside_vertex;
				if (!a_inside)
				{
					outside_vertex = triangle_4._a;
					inside_1 = triangle_4._b;
					inside_2 = triangle_4._c;
				}
				else if (!b_inside)
				{
					outside_vertex = triangle_4._b;
					inside_1 = triangle_4._c;
					inside_2 = triangle_4._a;
				}
				else if (!c_inside)
				{
					outside_vertex = triangle_4._c;
					inside_1 = triangle_4._a;
					inside_2 = triangle_4._b;
				}

				float inside_1_dist_to_plane = (inside_1.x - inside_1.w * plane_sign);
				float inside_2_dist_to_plane = (inside_2.x - inside_2.w * plane_sign);
				float outside_dist_to_plane = (outside_vertex.x - outside_vertex.w * plane_sign);

				//distance from the outside vertex the first clipping point the direction of the inside vertex
				float tP1 = outside_dist_to_plane / (outside_dist_to_plane - inside_1_dist_to_plane);
				//distance from the outside vertex the second clipping point the direction of the inside vertex
				float tP2 = outside_dist_to_plane / (outside_dist_to_plane - inside_2_dist_to_plane);

				//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
				vec4 P1 = outside_vertex + (tP1 - Triangle::EPSILON) * (inside_1 - outside_vertex);
				vec4 P2 = outside_vertex + (tP2 - Triangle::EPSILON) * (inside_2 - outside_vertex);

				//Creating the 2 new triangles
				out_clipped[triangles_added++] = Triangle4(inside_1, inside_2, P2);
				out_clipped[triangles_added++] = Triangle4(inside_1, P2, P1);
			}
		}
		else if (plane_index == 1)//Top and bottom planes
		{
			a_inside = (plane_sign == 1 && (triangle_4._a.y < triangle_4._a.w)) || (plane_sign == -1 && (triangle_4._a.y > -triangle_4._a.w));
			b_inside = (plane_sign == 1 && (triangle_4._b.y < triangle_4._b.w)) || (plane_sign == -1 && (triangle_4._b.y > -triangle_4._b.w));
			c_inside = (plane_sign == 1 && (triangle_4._c.y < triangle_4._c.w)) || (plane_sign == -1 && (triangle_4._c.y > -triangle_4._c.w));
			sum_inside = a_inside + b_inside + c_inside;


			if (sum_inside == 3)//All vertices inside, nothing to clip. Keeping the triangle as is
				out_clipped[triangles_added++] = triangle_4;
			else if (sum_inside == 1)
			{
				vec4 inside_vertex, outside_1, outside_2;
				if (a_inside)
				{
					inside_vertex = triangle_4._a;
					outside_1 = triangle_4._b;
					outside_2 = triangle_4._c;
				}
				else if (b_inside)
				{
					inside_vertex = triangle_4._b;
					outside_1 = triangle_4._c;
					outside_2 = triangle_4._a;
				}
				else if (c_inside)
				{
					inside_vertex = triangle_4._c;
					outside_1 = triangle_4._a;
					outside_2 = triangle_4._b;
				}

				//We're going to create one new triangle
				float inside_dist_to_plane = (inside_vertex.y - inside_vertex.w * plane_sign);
				float outside_1_dist_to_plane = (outside_1.y - outside_1.w * plane_sign);
				float outside_2_dist_to_plane = (outside_2.y - outside_2.w * plane_sign);

				/*
				x = -1:
				w = 2
				outside = -3
				inside = 1.5

				x = -1
				inside_dist = > 0
				dist_outside = < 0

				x = 1
				inside_dist = < 0
				dist_outside = > 0
				*/

				float tP1 = outside_1_dist_to_plane / (outside_1_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the first clipping point in the direction of insde_vertex
				float tP2 = outside_2_dist_to_plane / (outside_2_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the second clipping point in the direction of insde_vertex

				//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
				vec4 P1 = outside_1 + (tP1 - Triangle::EPSILON) * (inside_vertex - outside_1);
				vec4 P2 = outside_2 + (tP2 - Triangle::EPSILON) * (inside_vertex - outside_2);

				//Creating the new triangle
				out_clipped[triangles_added++] = Triangle4(inside_vertex, P1, P2);
			}
			else if (sum_inside == 2)
			{
				vec4 inside_1, inside_2, outside_vertex;
				if (!a_inside)
				{
					outside_vertex = triangle_4._a;
					inside_1 = triangle_4._b;
					inside_2 = triangle_4._c;
				}
				else if (!b_inside)
				{
					outside_vertex = triangle_4._b;
					inside_1 = triangle_4._c;
					inside_2 = triangle_4._a;
				}
				else if (!c_inside)
				{
					outside_vertex = triangle_4._c;
					inside_1 = triangle_4._a;
					inside_2 = triangle_4._b;
				}

				float inside_1_dist_to_plane = (inside_1.y - inside_1.w * plane_sign);
				float inside_2_dist_to_plane = (inside_2.y - inside_2.w * plane_sign);
				float outside_dist_to_plane = (outside_vertex.y - outside_vertex.w * plane_sign);

				//distance from the outside vertex the first clipping point the direction of the inside vertex
				float tP1 = outside_dist_to_plane / (outside_dist_to_plane - inside_1_dist_to_plane);
				//distance from the outside vertex the second clipping point the direction of the inside vertex
				float tP2 = outside_dist_to_plane / (outside_dist_to_plane - inside_2_dist_to_plane);

				//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
				vec4 P1 = outside_vertex + (tP1 - Triangle::EPSILON) * (inside_1 - outside_vertex);
				vec4 P2 = outside_vertex + (tP2 - Triangle::EPSILON) * (inside_2 - outside_vertex);

				//Creating the 2 new triangles
				out_clipped[triangles_added++] = Triangle4(inside_1, inside_2, P2);
				out_clipped[triangles_added++] = Triangle4(inside_1, P2, P1);
			}
		}
		else if (plane_index == 2)//Near and far planes
		{
			a_inside = (plane_sign == 1 && (triangle_4._a.z < triangle_4._a.w)) || (plane_sign == -1 && (triangle_4._a.z > -triangle_4._a.w));
			b_inside = (plane_sign == 1 && (triangle_4._b.z < triangle_4._b.w)) || (plane_sign == -1 && (triangle_4._b.z > -triangle_4._b.w));
			c_inside = (plane_sign == 1 && (triangle_4._c.z < triangle_4._c.w)) || (plane_sign == -1 && (triangle_4._c.z > -triangle_4._c.w));
			sum_inside = a_inside + b_inside + c_inside;


			if (sum_inside == 3)//All vertices inside, nothing to clip. Keeping the triangle as is
				out_clipped[triangles_added++] = triangle_4;
			else if (sum_inside == 1)
			{
				vec4 inside_vertex, outside_1, outside_2;
				if (a_inside)
				{
					inside_vertex = triangle_4._a;
					outside_1 = triangle_4._b;
					outside_2 = triangle_4._c;
				}
				else if (b_inside)
				{
					inside_vertex = triangle_4._b;
					outside_1 = triangle_4._c;
					outside_2 = triangle_4._a;
				}
				else if (c_inside)
				{
					inside_vertex = triangle_4._c;
					outside_1 = triangle_4._a;
					outside_2 = triangle_4._b;
				}

				//We're going to create one new triangle
				float inside_dist_to_plane = (inside_vertex.z - inside_vertex.w * plane_sign);
				float outside_1_dist_to_plane = (outside_1.z - outside_1.w * plane_sign);
				float outside_2_dist_to_plane = (outside_2.z - outside_2.w * plane_sign);

				/*
				x = -1:
				w = 2
				outside = -3
				inside = 1.5

				x = -1
				inside_dist = > 0
				dist_outside = < 0

				x = 1
				inside_dist = < 0
				dist_outside = > 0
				*/

				float tP1 = outside_1_dist_to_plane / (outside_1_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the first clipping point in the direction of insde_vertex
				float tP2 = outside_2_dist_to_plane / (outside_2_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the second clipping point in the direction of insde_vertex

				//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
				vec4 P1 = outside_1 + (tP1 - Triangle::EPSILON) * (inside_vertex - outside_1);
				vec4 P2 = outside_2 + (tP2 - Triangle::EPSILON) * (inside_vertex - outside_2);

				//Creating the new triangle
				out_clipped[triangles_added++] = Triangle4(inside_vertex, P1, P2);
			}
			else if (sum_inside == 2)
			{
				vec4 inside_1, inside_2, outside_vertex;
				if (!a_inside)
				{
					outside_vertex = triangle_4._a;
					inside_1 = triangle_4._b;
					inside_2 = triangle_4._c;
				}
				else if (!b_inside)
				{
					outside_vertex = triangle_4._b;
					inside_1 = triangle_4._c;
					inside_2 = triangle_4._a;
				}
				else if (!c_inside)
				{
					outside_vertex = triangle_4._c;
					inside_1 = triangle_4._a;
					inside_2 = triangle_4._b;
				}

				float inside_1_dist_to_plane = (inside_1.z - inside_1.w * plane_sign);
				float inside_2_dist_to_plane = (inside_2.z - inside_2.w * plane_sign);
				float outside_dist_to_plane = (outside_vertex.z - outside_vertex.w * plane_sign);

				//distance from the outside vertex the first clipping point the direction of the inside vertex
				float tP1 = outside_dist_to_plane / (outside_dist_to_plane - inside_1_dist_to_plane);
				//distance from the outside vertex the second clipping point the direction of the inside vertex
				float tP2 = outside_dist_to_plane / (outside_dist_to_plane - inside_2_dist_to_plane);

				//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
				vec4 P1 = outside_vertex + (tP1 - Triangle::EPSILON) * (inside_1 - outside_vertex);
				vec4 P2 = outside_vertex + (tP2 - Triangle::EPSILON) * (inside_2 - outside_vertex);

				//Creating the 2 new triangles
				out_clipped[triangles_added++] = Triangle4(inside_1, inside_2, P2);
				out_clipped[triangles_added++] = Triangle4(inside_1, P2, P1);
			}
			}
	}

	return triangles_added;
}

int clip_triangle(const Triangle4& to_clip_triangle, std::array<Triangle4, 12>& clipped_triangles)
{
	int nb_triangles = 1;

	std::array<Triangle4, 12> temp = { to_clip_triangle };//TODO size 12 ? calculer le worst case scenario
	
	nb_triangles = clip_triangle_to_plane(0, -1, temp, nb_triangles, clipped_triangles);
	nb_triangles = clip_triangle_to_plane(0, 1, clipped_triangles, nb_triangles, temp);
	nb_triangles = clip_triangle_to_plane(1, 1, temp, nb_triangles, clipped_triangles);
	nb_triangles = clip_triangle_to_plane(1, -1, clipped_triangles, nb_triangles, temp);
	nb_triangles = clip_triangle_to_plane(2, 1, temp, nb_triangles, clipped_triangles);
	nb_triangles = clip_triangle_to_plane(2, -1, clipped_triangles, nb_triangles, temp);

	clipped_triangles = temp;

	return nb_triangles;
}

//TODO utiliser les proper Point et Vector là où il faut plutôt que Vector partout
void Renderer::rasterTrace()
{
	Transform perspective_projection = _scene._camera._perspective_proj_mat;
	Transform perspective_projection_inv = _scene._camera._perspective_proj_mat_inv;

	//TODO remove
	/*vec4 left = vec4(normalize(vec3(perspective_projection[0] + perspective_projection[3])), perspective_projection.m[3][0] + perspective_projection.m[3][3]);
	vec4 right = vec4(normalize(vec3(-perspective_projection[0] + perspective_projection[3])), -perspective_projection.m[3][0] + perspective_projection.m[3][3]);
	vec4 bottom = vec4(normalize(vec3(perspective_projection[1] + perspective_projection[3])), perspective_projection.m[3][1] + perspective_projection.m[3][3]);
	vec4 top = vec4(normalize(vec3(-perspective_projection[1] + perspective_projection[3])), -perspective_projection.m[3][1] + perspective_projection.m[3][3]);
	vec4 near = vec4(normalize(vec3(perspective_projection[2] + perspective_projection[3])), perspective_projection.m[3][2] + perspective_projection.m[3][3]);
	vec4 far = vec4(normalize(vec3(-perspective_projection[2] + perspective_projection[3])), -perspective_projection.m[3][2] + perspective_projection.m[3][3]);*/

//#pragma omp parallel for
	for (int triangle_index = 0; triangle_index < _scene._triangles.size(); triangle_index++)
	{
		Triangle& triangle = _scene._triangles[triangle_index];

		vec4 a_image_plane4 = perspective_projection(vec4(Point(triangle._a)));//TODO remove unecessary conversion Point(...)
		vec4 b_image_plane4 = perspective_projection(vec4(Point(triangle._b)));
		vec4 c_image_plane4 = perspective_projection(vec4(Point(triangle._c)));

		std::array<Triangle4, 12> clipped_triangles;
		int nb_clipped = clip_triangle(Triangle4(a_image_plane4, b_image_plane4, c_image_plane4), clipped_triangles);
		//int nb_clipped = 1;
		//clipped_triangles[0] = Triangle4(a_image_plane4, b_image_plane4, c_image_plane4);//No clipping

		//TODO proper clipping algorithm
		//minXPixels = std::max(0, minXPixels);
		//minYPixels = std::max(0, minYPixels);
		//maxXPixels = std::min(_width - 1, maxXPixels);
		//maxYPixels = std::min(_height - 1, maxYPixels);

		for (int clipped_triangle_index = 0; clipped_triangle_index < nb_clipped; clipped_triangle_index++)
		{
			Triangle4 clipped_triangle = clipped_triangles[clipped_triangle_index];
			Triangle clipped_triangle_NDC(clipped_triangle, triangle._materialIndex);

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

			minXPixels = std::max(0, minXPixels);
			minYPixels = std::max(0, minYPixels);
			maxXPixels = std::min(_width - 1, maxXPixels);
			maxYPixels = std::min(_height - 1, maxYPixels);

			for (int py = minYPixels; py <= maxYPixels; py++)
			{
				float image_y = py / (float)_height * 2 - 1;

				for (int px = minXPixels; px <= maxXPixels; px++)
				{
					float image_x = px / (float)_width * 2 - 1;

					Vector pixel_point(image_x, image_y, -1);

					//We don't care about the z coordinate here
					float invTriangleArea = 1 / ((b_image_plane.x - a_image_plane.x) * (c_image_plane.y - a_image_plane.y) - (b_image_plane.y - a_image_plane.y) * (c_image_plane.x - a_image_plane.x));

					//TODO optimiser la edge function commee dit sur scratchapixel en ne recalculant pas la partie qui utilise la position du pixel en y pour chaque pixel en X puisque la position du pixel en Y ne change pas pour chaque position en X
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

					//Z coordinate of the point on the triangle by interpolating the z coordinates of the 3 vertices
					float zCameraSpace = (w * -clipped_triangle_NDC._a.z + u * -clipped_triangle_NDC._b.z + v * -clipped_triangle_NDC._c.z);
					if (zCameraSpace > _z_buffer[py][px])
					{
						_z_buffer[py][px] = zCameraSpace;

#if SHADING
						//TODO remove unecessary point/vector conversion
						Color trace_color = traceTriangle(Ray(_scene._camera._position, normalize(Vector(perspective_projection_inv(Point(pixel_point))) - _scene._camera._position)), perspective_projection_inv(clipped_triangle_NDC));
						_image(px, py) = trace_color;

						if (px == 198 && py == (_height - 428))
						{
							std::cout << triangle_index << ": " << triangle << ", " << trace_color << std::endl;
						}

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