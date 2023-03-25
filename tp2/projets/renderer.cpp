#include "renderer.h"

#include <cmath>

#include "mat.h"

Material init_default_material()
{
	Material mat = Material(Color(1.0f, 0.0f, 0.5f));//Pink color
	mat.specular = Color(1.0f, 1.0f, 1.0f);
	mat.ns = 15;

	return mat;
}

Material Renderer::DEFAULT_MATERIAL = init_default_material();
Color Renderer::AMBIENT_COLOR = Color(0.1f, 0.1f, 0.1f);
Color Renderer::BACKGROUND_COLOR = Color(0.0, 0.0, 0.0);

RenderSettings RenderSettings::basic_settings(int width, int height, bool hybrid_raste_trace)
{
	RenderSettings settings;
	settings.image_width = width;
	settings.image_height = height;
	settings.hybrid_rasterization_tracing = hybrid_raste_trace;
	settings.compute_shadows = false;

	return settings;
}

RenderSettings RenderSettings::ssaa_settings(int width, int height, int ssaa_factor, bool hybrid_raste_trace, bool compute_shadows)
{
	RenderSettings settings;
	settings.image_width = width;
	settings.image_height = height;
	settings.enable_ssaa = true;
	settings.ssaa_factor = ssaa_factor;
	settings.hybrid_rasterization_tracing = hybrid_raste_trace;
	settings.compute_shadows = compute_shadows;

	return settings;
}

std::ostream& operator << (std::ostream& os, const RenderSettings& settings)
{
	os << "Render[" << (settings.hybrid_rasterization_tracing ? "Rast" : "RT") << ", " << settings.image_width << "x" << settings.image_height;
	if (settings.enable_ssaa)
		os << ", " << "SSAAx" << settings.ssaa_factor;
	os << "]";

	return os;
}

Renderer::Renderer(Scene scene, std::vector<Triangle>& triangles, RenderSettings render_settings) : _triangles(triangles), 
	_bvh(BVH(triangles, render_settings.bvh_max_depth)), _render_settings(render_settings), _scene(scene)
{
	//Accounting for the SSAA scaling
	_render_width = render_settings.enable_ssaa ? render_settings.image_width * render_settings.ssaa_factor : render_settings.image_width;
	_render_height = render_settings.enable_ssaa ? render_settings.image_height * render_settings.ssaa_factor : render_settings.image_height;

	//Initializing the z-buffer if we're using the rasterization approach
	if (render_settings.hybrid_rasterization_tracing)
	{
		_z_buffer = new float* [_render_height];
		if (_z_buffer == nullptr)
		{
			std::cout << "Not enough memory to allocate the ZBuffer of the ray tracer..." << std::endl;
			std::exit(-1);
		}

		for (int i = 0; i < _render_height; i++)
		{
			_z_buffer[i] = new float[_render_width];
			if (_z_buffer[i] == nullptr)
			{
				std::cout << "Not enough memory to allocate the ZBuffer of the ray tracer..." << std::endl;
				std::exit(-1);
			}

			for (int j = 0; j < _render_width; j++)
				_z_buffer[i][j] = -INFINITY;
		}

		_scene._camera.init_perspec_proj_mat((float)_render_width / _render_height);//Perspective projection matrix from camera space to NDC space
	}

	_image = Image(_render_width, _render_height);
}

Renderer::~Renderer()
{
	if (_render_settings.hybrid_rasterization_tracing)
	{
		for (int i = 0; i < _render_height; i++)
			delete[] _z_buffer[i];

		delete[] _z_buffer;
	}
}

Image* Renderer::getImage()
{
	return &_image;
}

RenderSettings& Renderer::render_settings()
{
	return _render_settings;
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
        return hit_material.specular * std::pow(std::max(0.0f, angle), hit_material.ns);
}

bool Renderer::is_shadowed(const Point& inter_point, const Point& light_position) const
{
	if (_render_settings.compute_shadows)
	{
		Ray ray(inter_point, light_position - inter_point);
		HitInfo hitInfo;

		if (_render_settings.enable_bvh)
		{
			if (_bvh.intersect(ray, hitInfo))
			{
				Point new_inter_point = ray._origin + ray._direction * hitInfo.t;

				//If we found an object that is between the light and the current inter_point: the point is shadowed
				if (distance(Point(inter_point), Point(new_inter_point)) < distance(Point(inter_point), Point(light_position)))
					return true;
			}
		}
		else
		{
			for (const Triangle& triangle : _triangles)
			{
				if (triangle.intersect(ray, hitInfo))
				{
					Point new_inter_point = ray._origin + ray._direction * hitInfo.t;

					if (distance(Point(inter_point), Point(new_inter_point)) < distance(Point(inter_point), Point(light_position)))
					{
						//We found an object that is between the light and the current inter_point: the point is shadowed

						return true;
					}
				}
			}
		}


	}	//We haven't found any object between the light source and the intersection point, the point isn't shadowed

	return false;
}

Color Renderer::trace_triangle(const Ray& ray, const Triangle& triangle) const
{
	HitInfo hit_info;
	Color finalColor = Color(0, 0, 0);

	if (triangle.intersect(ray, hit_info))
	{
		if (_render_settings.use_shading)
		{
			Point inter_point = ray._origin + ray._direction * (hit_info.t + Renderer::EPSILON);
			Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
			Vector normal = normalize(hit_info.normal_at_intersection);

			Material hit_material;
			if (hit_info.mat_index == -1)
				hit_material = Renderer::DEFAULT_MATERIAL;
			else
				hit_material = _scene._materials(hit_info.mat_index);

			finalColor = finalColor + computeDiffuse(hit_material, normal, direction_to_light);
			finalColor = finalColor + computeSpecular(hit_material, ray._direction, normal, direction_to_light);
			if (is_shadowed(inter_point, _scene._point_light._position))
				finalColor = finalColor * Color(Renderer::SHADOW_INTENSITY);
			finalColor = finalColor + hit_material.emission;

			finalColor = finalColor + Renderer::AMBIENT_COLOR * hit_material.ambient_coeff;
		}
		else
		{
			if (_render_settings.color_normal_or_barycentric)
			{
				//Color triangles with normal
				Vector normalized_normal = normalize(hit_info.normal_at_intersection);
				finalColor = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
			}
			else
			{
				//Color triangles with barycentric coordinates
				float u = hit_info.u;
				float v = hit_info.v;

				finalColor = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
			}
		}
	}

	return finalColor;
}

template<int plane_index, int plane_sign>
inline bool is_a_inside(const Triangle4& triangle)
{
	return false;
}

template<int plane_index, int plane_sign>
inline bool is_b_inside(const Triangle4& triangle)
{
	return false;
}

template<int plane_index, int plane_sign>
inline bool is_c_inside(const Triangle4& triangle)
{
	return false;
}

template<>
inline bool is_a_inside<0, 1>(const Triangle4& triangle)
{
	return triangle._a.x < triangle._a.w;
}

template<>
inline bool is_a_inside<0, -1>(const Triangle4& triangle)
{
	return triangle._a.x > -triangle._a.w;
}

template<>
inline bool is_a_inside<1, 1>(const Triangle4& triangle)
{
	return triangle._a.y < triangle._a.w;
}

template<>
inline bool is_a_inside<1, -1>(const Triangle4& triangle)
{
	return triangle._a.y > -triangle._a.w;
}

template<>
inline bool is_a_inside<2, 1>(const Triangle4& triangle)
{
	return triangle._a.z < triangle._a.w;
}

template<>
inline bool is_a_inside<2, -1>(const Triangle4& triangle)
{
	return triangle._a.z > -triangle._a.w;
}




template<>
inline bool is_b_inside<0, 1>(const Triangle4& triangle)
{
	return triangle._b.x < triangle._b.w;
}

template<>
inline bool is_b_inside<0, -1>(const Triangle4& triangle)
{
	return triangle._b.x > -triangle._b.w;
}

template<>
inline bool is_b_inside<1, 1>(const Triangle4& triangle)
{
	return triangle._b.y < triangle._b.w;
}

template<>
inline bool is_b_inside<1, -1>(const Triangle4& triangle)
{
	return triangle._b.y > -triangle._b.w;
}

template<>
inline bool is_b_inside<2, 1>(const Triangle4& triangle)
{
	return triangle._b.z < triangle._b.w;
}

template<>
inline bool is_b_inside<2, -1>(const Triangle4& triangle)
{
	return triangle._b.z > -triangle._b.w;
}



template<>
inline bool is_c_inside<0, 1>(const Triangle4& triangle)
{
	return triangle._c.x < triangle._c.w;
}

template<>
inline bool is_c_inside<0, -1>(const Triangle4& triangle)
{
	return triangle._c.x > -triangle._c.w;
}

template<>
inline bool is_c_inside<1, 1>(const Triangle4& triangle)
{
	return triangle._c.y < triangle._c.w;
}

template<>
inline bool is_c_inside<1, -1>(const Triangle4& triangle)
{
	return triangle._c.y > -triangle._c.w;
}

template<>
inline bool is_c_inside<2, 1>(const Triangle4& triangle)
{
	return triangle._c.z < triangle._c.w;
}

template<>
inline bool is_c_inside<2, -1>(const Triangle4& triangle)
{
	return triangle._c.z > -triangle._c.w;
}

template<int plane_index, int plane_sign>
int Renderer::clip_triangles_to_plane(std::array<Triangle4, 12>& to_clip, int nb_triangles, std::array<Triangle4, 12>& out_clipped) const
{
	const static float CLIPPING_EPSILON = 1.0e-5f;

	int sum_inside;
	bool a_inside, b_inside, c_inside;

	int triangles_added = 0;//Used as an index in the out_clipped array to know where to store clipped triangles

	for (int triangle_index = 0; triangle_index < nb_triangles; triangle_index++)
	{
		Triangle4& triangle_4 = to_clip[triangle_index];

		a_inside = is_a_inside<plane_index, plane_sign>(triangle_4);
		b_inside = is_b_inside<plane_index, plane_sign>(triangle_4);
		c_inside = is_c_inside<plane_index, plane_sign>(triangle_4);

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
			float inside_dist_to_plane = (inside_vertex(plane_index) - inside_vertex.w * plane_sign);
			float outside_1_dist_to_plane = (outside_1(plane_index) - outside_1.w * plane_sign);
			float outside_2_dist_to_plane = (outside_2(plane_index) - outside_2.w * plane_sign);

			float tP1 = outside_1_dist_to_plane / (outside_1_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the first clipping point in the direction of insde_vertex
			float tP2 = outside_2_dist_to_plane / (outside_2_dist_to_plane - inside_dist_to_plane);//distance from inside_vertex to the second clipping point in the direction of insde_vertex

			//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
			vec4 P1 = outside_1 + (tP1 - CLIPPING_EPSILON) * (inside_vertex - outside_1);
			vec4 P2 = outside_2 + (tP2 - CLIPPING_EPSILON) * (inside_vertex - outside_2);

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

			float inside_1_dist_to_plane = (inside_1(plane_index) - inside_1.w * plane_sign);
			float inside_2_dist_to_plane = (inside_2(plane_index) - inside_2.w * plane_sign);
			float outside_dist_to_plane = (outside_vertex(plane_index) - outside_vertex.w * plane_sign);

			//distance from the outside vertex the first clipping point the direction of the inside vertex
			float tP1 = outside_dist_to_plane / (outside_dist_to_plane - inside_1_dist_to_plane);
			//distance from the outside vertex the second clipping point the direction of the inside vertex
			float tP2 = outside_dist_to_plane / (outside_dist_to_plane - inside_2_dist_to_plane);

			//Adding EPSILON to avoid artifacts on the edges of the image where triangles are clipped
			vec4 P1 = outside_vertex + (tP1 - CLIPPING_EPSILON) * (inside_1 - outside_vertex);
			vec4 P2 = outside_vertex + (tP2 - CLIPPING_EPSILON) * (inside_2 - outside_vertex);

			//Creating the 2 new triangles
			out_clipped[triangles_added++] = Triangle4(inside_1, inside_2, P2);
			out_clipped[triangles_added++] = Triangle4(inside_1, P2, P1);
		}
	}

	return triangles_added;
}

int Renderer::clip_triangle(std::array<Triangle4, 12>& to_clip_triangles, std::array<Triangle4, 12>& clipped_triangles) const
{
	int nb_triangles = 1;

	if (_render_settings.enable_clipping)
	{
		//TODO profiler le clipping pour continuer sur ce que je faisais
		//TODO opti ? pour ne pas redéclarer le tableau à chaque appel de la fonction 
		//std::array<Triangle4, 12> temp = { to_clip_triangle };

		nb_triangles = clip_triangles_to_plane<0, 1>(to_clip_triangles, nb_triangles, to_clip_triangles);//right plane
		nb_triangles = clip_triangles_to_plane<0, -1>(to_clip_triangles, nb_triangles, clipped_triangles);//left plane
		nb_triangles = clip_triangles_to_plane<1, 1>(clipped_triangles, nb_triangles, to_clip_triangles);//top plane
		nb_triangles = clip_triangles_to_plane<1, -1>(to_clip_triangles, nb_triangles, clipped_triangles);//bottom plane
		nb_triangles = clip_triangles_to_plane<2, 1>(clipped_triangles, nb_triangles, to_clip_triangles);//far plane
		nb_triangles = clip_triangles_to_plane<2, -1>(to_clip_triangles, nb_triangles, clipped_triangles);//near plane
	}
	else
		clipped_triangles[0] = to_clip_triangles[0];

	return nb_triangles;
}

void Renderer::raster_trace()
{
	Transform perspective_projection = _scene._camera._perspective_proj_mat;
	Transform perspective_projection_inv = _scene._camera._perspective_proj_mat_inv;

	static const float render_height_scaling = 1.0f / _render_height * 2;
	static const float render_width_scaling = 1.0f / _render_width * 2;

	//TODO projection matrix sur la version ray tracée parce que c'est actuellement pas le cas
	std::array<Triangle4, 12> to_clip_triangles;
	std::array<Triangle4, 12> clipped_triangles;
//#pragma omp parallel for private(to_clip_triangles, clipped_triangles)
	for (int triangle_index = 0; triangle_index < _triangles.size(); triangle_index++)
	{
		Triangle& triangle = _triangles[triangle_index];

		vec4 a_image_plane4 = perspective_projection(vec4(triangle._a));
		vec4 b_image_plane4 = perspective_projection(vec4(triangle._b));
		vec4 c_image_plane4 = perspective_projection(vec4(triangle._c));

		to_clip_triangles[0] = Triangle4(a_image_plane4, b_image_plane4, c_image_plane4);
		int nb_clipped = clip_triangle(to_clip_triangles, clipped_triangles);

		for (int clipped_triangle_index = 0; clipped_triangle_index < nb_clipped; clipped_triangle_index++)
		{
			Triangle4 clipped_triangle = clipped_triangles[clipped_triangle_index];
			Triangle clipped_triangle_NDC(clipped_triangle, triangle._materialIndex);

			Point a_image_plane = clipped_triangle_NDC._a;
			Point b_image_plane = clipped_triangle_NDC._b;
			Point c_image_plane = clipped_triangle_NDC._c;

			//We don't care about the z coordinate here
			float invTriangleArea = 1 / ((b_image_plane.x - a_image_plane.x) * (c_image_plane.y - a_image_plane.y) - (b_image_plane.y - a_image_plane.y) * (c_image_plane.x - a_image_plane.x));

			//Computing the bounding box of the triangle so that next, we only test pixels that are in the bounding box of the triangle
			float boundingMinX = std::min(a_image_plane.x, std::min(b_image_plane.x, c_image_plane.x));
			float boundingMinY = std::min(a_image_plane.y, std::min(b_image_plane.y, c_image_plane.y));
			float boundingMaxX = std::max(a_image_plane.x, std::max(b_image_plane.x, c_image_plane.x));
			float boundingMaxY = std::max(a_image_plane.y, std::max(b_image_plane.y, c_image_plane.y));
		
			int minXPixels = (int)((boundingMinX + 1) * 0.5 * _render_width);
			int minYPixels = (int)((boundingMinY + 1) * 0.5 * _render_height);
			int maxXPixels = (int)((boundingMaxX + 1) * 0.5 * _render_width);
			int maxYPixels = (int)((boundingMaxY + 1) * 0.5 * _render_height);

			maxXPixels = std::min(_render_width - 1, maxXPixels);
			maxYPixels = std::min(_render_height - 1, maxYPixels);

			float image_y = minYPixels * render_height_scaling - 1;
			float image_y_increment = render_height_scaling;

			float image_x = minXPixels * render_width_scaling - 1;
			float image_x_increment = render_width_scaling;
			for (int py = minYPixels; py <= maxYPixels; py++, image_y += image_y_increment)
			{
				for (int px = minXPixels; px <= maxXPixels; px++, image_x += image_x_increment)
				{
					//NOTE If there are still issues with the clipping algorithm creating new points just a little over the edge of the view frustum, consider using a simple std::max(0, ...)
					assert(px >= 0 && px < _render_width);
					assert(py >= 0 && py < _render_height);

					Point pixel_point(image_x, image_y, -1);

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

						Color final_color;

						if (_render_settings.use_shading)
							final_color = trace_triangle(Ray(_scene._camera._position, normalize(perspective_projection_inv(pixel_point) - _scene._camera._position)), perspective_projection_inv(clipped_triangle_NDC));
						else if (_render_settings.color_normal_or_barycentric)
						{
							Vector normalized_normal = normalize(cross(triangle._b - triangle._a, triangle._c - triangle._a));
							final_color = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
						}
						else //Color triangles with barycentric coordinates
							final_color = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
						
						_image(px, py) = final_color;
					}
				}
			}
		}
	}
}

void Renderer::ray_trace()
{
#pragma omp parallel for schedule(dynamic)
	for (int py = 0; py < _render_height; py++)
	{
		float y_world = (float)py / _render_height * 2 - 1;

		for (int px = 0; px < _render_width; px++)
		{
			float x_world = (float)px / _render_width * 2 - 1;
			x_world *= (float)_render_width / _render_height;

			Point image_plane_point = Point(x_world, y_world, -1);

			Point camera_position = _scene._camera._position;
			Vector ray_direction = normalize(image_plane_point - camera_position);
			Ray ray(camera_position, ray_direction);

			HitInfo finalHitInfo;
			HitInfo hit_info;

			//TODO mettre un std::cout << dans le copy constructor des Triangle pour voir partout où le copy constructor est appelé
			if (_render_settings.enable_bvh)
			{
				if (_bvh.intersect(ray, hit_info))
					if (hit_info.t < finalHitInfo.t || finalHitInfo.t == -1)
						finalHitInfo = hit_info;
			}
			else
				for (Triangle& triangle : _triangles)
					if (triangle.intersect(ray, hit_info))
						if (hit_info.t < finalHitInfo.t || finalHitInfo.t == -1)
							finalHitInfo = hit_info;

			//TODO modifier le parseur d'OBJ pour ajouter le parsing de l'ambient
			Color finalColor;

			if (finalHitInfo.t > 0)//We found an intersection
			{
				//TODO factoriser ça dans une fonction parce que ce morcaeu de code où on teste (if shading) ... on le fait partout à plein d'endroit donc l'idée serait de faire une fonction qui fait tout ça pour nous
				if (_render_settings.use_shading)
				{
					Point inter_point = ray._origin + ray._direction * (finalHitInfo.t + Renderer::EPSILON);
					Vector direction_to_light = normalize(_scene._point_light._position - inter_point);
					Vector normal = normalize(finalHitInfo.normal_at_intersection);

					Material hit_material;
					if (finalHitInfo.mat_index == -1)
						hit_material = Renderer::DEFAULT_MATERIAL;
					else
						hit_material = _scene._materials(finalHitInfo.mat_index);

					finalColor = finalColor + computeDiffuse(hit_material, normal, direction_to_light);
					finalColor = finalColor + computeSpecular(hit_material, ray._direction, normal, direction_to_light);
					finalColor = finalColor + hit_material.emission;
					if (is_shadowed(inter_point, _scene._point_light._position))
						finalColor = finalColor * Color(Renderer::SHADOW_INTENSITY);

					finalColor = finalColor + Renderer::AMBIENT_COLOR * hit_material.ambient_coeff;
				}
				else if (_render_settings.color_normal_or_barycentric) //Color triangles with normal
				{
					Vector normalized_normal = normalize(finalHitInfo.normal_at_intersection);
					finalColor = Color(std::abs(normalized_normal.x), std::abs(normalized_normal.y), std::abs(normalized_normal.z));
				}
				else //Color triangles with barycentric coordinates
				{
					float u = finalHitInfo.u;
					float v = finalHitInfo.v;

					finalColor = Color(1, 0, 0) * u + Color(0, 1.0, 0) * v + Color(0, 0, 1) * (1 - u - v);
				}
			}
			else
				finalColor = Renderer::BACKGROUND_COLOR;//Black since we didn't found any intersection

			_image(px, py) = finalColor;
		}
	}
}
