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
std::array<ClippingPlane, 5> Renderer::CLIPPING_PLANES = { ClippingPlane{Vector(0, 0, -1), 1} };//, //Near clipping plane
														   //ClippingPlane{Vector(1 / std::sqrt(2), 0, -1 / std::sqrt(2)), 0}, //Left
														   //ClippingPlane{Vector(-1 / std::sqrt(2), 0, -1 / std::sqrt(2)), 0},//Right
														   //ClippingPlane{Vector(0, 1 / std::sqrt(2), -1 / std::sqrt(2)), 0},//Bottom
														   //ClippingPlane{Vector(0, -1 / std::sqrt(2), -1 / std::sqrt(2)), 0} };//Top

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

std::vector<Triangle> Renderer::clip_triangle_against_plane(const Triangle& triangle, const ClippingPlane& plane, bool a_inside, bool b_inside, bool c_inside)
{
	std::vector<Triangle> new_triangles;

	int sum = a_inside + b_inside + c_inside;

	if (sum == 1)//Only one vertex inside, we're going to clip the triangle and create a new one
	{
		Vector inside_vertex, outside_1, outside_2;
		if (a_inside)
		{
			inside_vertex = triangle._a;
			outside_1 = triangle._b;
			outside_2 = triangle._c;
		}
		else if (b_inside)
		{
			inside_vertex = triangle._b;
			outside_1 = triangle._a;
			outside_2 = triangle._c;
		}
		else
		{
			inside_vertex = triangle._c;
			outside_1 = triangle._a;
			outside_2 = triangle._b;
		}

		float NIn = dot(plane._normal, inside_vertex);

		//Intersection between the segment [inside_vertex, outside_1] and the plane
		Vector segment_1 = outside_1 - inside_vertex;
		float tP1 = (-plane._d - NIn) / dot(plane._normal, segment_1);
		Vector P1 = inside_vertex + tP1 * segment_1;

		//Intersection between the segment [inside_vertex, outside_2] and the plane
		Vector segment_2 = outside_2 - inside_vertex;
		float tP2 = (-plane._d - NIn) / dot(plane._normal, segment_2);
		Vector P2 = inside_vertex + tP2 * segment_2;

		new_triangles.push_back(Triangle(inside_vertex, P1, P2));
	}
	else if (sum == 2)//Two vertices are inside, we're going to clip the triangle and create two new ones
	{
		std::vector<Triangle> clippedNewTriangles;

		Vector outside_vertex, inside_1, inside_2;

		if (!a_inside)
		{
			outside_vertex = triangle._a;
			inside_1 = triangle._b;
			inside_2 = triangle._c;
		}
		else if (!b_inside)
		{
			outside_vertex = triangle._b;
			inside_1 = triangle._a;
			inside_2 = triangle._c;
		}
		else if (!c_inside)
		{
			outside_vertex = triangle._c;
			inside_1 = triangle._a;
			inside_2 = triangle._b;//TODO maybe swap les deux inside ici pour avoir un triangle bien orienté ?
		}

		//Now computing the points that are going to be used to make 2 new triangles

		float NOut = dot(plane._normal, outside_vertex);

		Vector segment_1 = outside_vertex - inside_1;
		float tP1 = (-plane._d - NOut) / dot(plane._normal, segment_1);
		Vector P1 = inside_1 + tP1 * segment_1;

		Vector segment_2 = outside_vertex - inside_2;
		float tP2 = (-plane._d - NOut) / dot(plane._normal, segment_2);
		Vector P2 = inside_2 + tP2 * segment_2;

		new_triangles.push_back(Triangle(inside_1, P1, inside_2));
		new_triangles.push_back(Triangle(inside_2, P1, P2));
	}

	return new_triangles;
}

void Renderer::clip()
{
	_scene._triangles = clip(_scene._triangles);
}

std::vector<Triangle> Renderer::clip(const std::vector<Triangle>& triangles)
{
	std::vector<Triangle> visible_triangles;
	visible_triangles.reserve(triangles.size());

	std::cout << sizeof(std::vector<Triangle>::iterator) << std::endl;

	int kept = 0;
	int clipped = 0;
	int removed = 0;

	std::vector<Triangle> clipped_triangles;//Triangles that have been clipped and that need to be checked against the planes again
	for (const Triangle& triangle : triangles)
	{

		for (const ClippingPlane& plane : Renderer::CLIPPING_PLANES)
		{
			bool a_inside = dot(plane._normal, triangle._a) - plane._d > 0;
			bool b_inside = dot(plane._normal, triangle._b) - plane._d > 0;
			bool c_inside = dot(plane._normal, triangle._c) - plane._d > 0;
			int sum = a_inside + b_inside + c_inside;

			if (sum == 3)
				visible_triangles.push_back(triangle);
			else if (sum != 0)//One or two vertices are outside the volume defined by the plane
			{
				std::vector<Triangle> new_triangles = clip_triangle_against_plane(triangle, plane, a_inside, b_inside, c_inside);
				for (const Triangle& triangle : new_triangles)
					clipped_triangles.push_back(triangle + plane._normal * Triangle::EPSILON);//TODO modifier ça pour éviter les copies dans tous les sens
			}
		}
	}

	//Clipping the triangles further
	while (clipped_triangles.size() > 0)
	{
		std::vector<Triangle> new_clipped_triangles;

		for (const Triangle& triangle : clipped_triangles)
		{
			for (const ClippingPlane& plane : Renderer::CLIPPING_PLANES)
			{
				bool a_inside = dot(plane._normal, triangle._a) - plane._d > 0;
				bool b_inside = dot(plane._normal, triangle._b) - plane._d > 0;
				bool c_inside = dot(plane._normal, triangle._c) - plane._d > 0;
				int sum = a_inside + b_inside + c_inside;

				if (sum == 3)
					visible_triangles.push_back(triangle);
				else if (sum != 0)//One or two vertices are outside the volume defined by the plane
				{
					std::vector<Triangle> new_triangles = clip_triangle_against_plane(triangle, plane, a_inside, b_inside, c_inside);
					for (const Triangle& triangle : new_triangles)
						new_clipped_triangles.push_back(triangle);//TODO modifier ça pour éviter les copies dans tous les sens
				}
			}
		}

		clipped_triangles = new_clipped_triangles;
	}

	std::cout << removed << " triangles completely clipped\n";
	std::cout << clipped << " triangles partially clipped\n";
	std::cout << kept << " triangles kept\n";

	visible_triangles.shrink_to_fit();
	return visible_triangles;
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

//TODO utiliser les proper Point et Vector là où il faut plutôt que Vector partout
void Renderer::rasterTrace()
{
#pragma omp parallel for
	for (int triangle_index = 0; triangle_index < _scene._triangles.size(); triangle_index++)
	{
		Triangle& triangle = _scene._triangles[triangle_index];

		//Projection of the triangle on the image plane
		float invAZ = 1 / -triangle._a.z;
		float invBZ = 1 / -triangle._b.z;
		float invCZ = 1 / -triangle._c.z;

		Transform perspective_projection = Perspective(30, (float)_width / _height, 1, 1000);//TODO mettre ça dans la classe Camera
		Vector a_image_plane = Vector(perspective_projection(Point(triangle._a)));
		Vector b_image_plane = Vector(perspective_projection(Point(triangle._b)));
		Vector c_image_plane = Vector(perspective_projection(Point(triangle._c)));

		//Computing the bounding box of the triangle so that next, we only test pixels that are in the bounding box of the triangle
		float boundingMinX = std::min(a_image_plane.x, std::min(b_image_plane.x, c_image_plane.x));
		float boundingMinY = std::min(a_image_plane.y, std::min(b_image_plane.y, c_image_plane.y));
		float boundingMaxX = std::max(a_image_plane.x, std::max(b_image_plane.x, c_image_plane.x));
		float boundingMaxY = std::max(a_image_plane.y, std::max(b_image_plane.y, c_image_plane.y));

		//if (boundingMinX < -1 || boundingMinY < -1 || boundingMaxX > 1 || boundingMaxY > 1)
			//continue;//Completely clipping the triangle //TODO Should be replaced by a proper polygon clipping algorithm !!!!
		
		int minXPixels = (boundingMinX + 1) * 0.5 * IMAGE_WIDTH;
		int minYPixels = (boundingMinY + 1) * 0.5 * IMAGE_HEIGHT;
		int maxXPixels = (boundingMaxX + 1) * 0.5 * IMAGE_WIDTH;
		int maxYPixels = (boundingMaxY + 1) * 0.5 * IMAGE_HEIGHT;

		//TODO remplacer ces ifs là par un std::min dans le calcul de min/maxXPixels au desuss
		/*if (maxXPixels == IMAGE_WIDTH)
			maxXPixels--;
		if (maxYPixels == IMAGE_HEIGHT)
			maxYPixels--;*/


		//TODO remplace l'utilisatrlion de IMAGE_WIDTH  et height par _width rey _height
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
					_image(px, py) = traceTriangle(Ray(_scene._camera._position, normalize(normalize(Vector(perspective_projection.inverse()(Point(pixel_point)))) - _scene._camera._position)), triangle);
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