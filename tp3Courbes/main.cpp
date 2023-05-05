#include <stdio.h>
#include <vector>

struct Point3D
{
    float x;
    float y;
    float z;

    friend Point3D operator * (float k, const Point3D& point)
    {
        return { point.x * k, point.y * k, point.z * k };
    }

    friend Point3D operator + (const Point3D& point_a, const Point3D& point_b)
    {
        return { point_a.x + point_b.x, point_a.y + point_b.y, point_a.z + point_b.z };
    }
};

void casteljau(const std::vector<Point3D>& control_points, std::vector<Point3D>& out_points, float t)
{
    float current_t = 0;

    for (int t_iter = 0; t_iter < (int)(1.0f / t); t_iter++)
    {
        std::vector<Point3D> current_iteration_points = control_points;

        for (int iter = 0; iter < current_iteration_points.size(); iter++)
        {   
            std::vector<Point3D> new_points;

            for (int i = 0; i < current_iteration_points.size() - iter - 1; i++)
                new_points.push_back((1 - current_t) * current_iteration_points[i] + current_t * current_iteration_points[i + 1]);

            current_iteration_points = new_points;
        }

        out_points.push_back(current_iteration_points[0]);

        current_t += t;
    }
}

int main(void)
{
    std::vector<Point3D> control_points;
    control_points.push_back({ 0, 0, 0 });
    control_points.push_back({ 0, 0, 1 });
    control_points.push_back({ 0, 0, 2 });
    control_points.push_back({ 0, -2, 2 });
    control_points.push_back({ 0, -3, 2 });

    std::vector<Point3D> out_points;

    casteljau(control_points, out_points, 0.01f);

    FILE *f = fopen("line.obj", "w");

    for (int i = 0; i < out_points.size(); i++)
        fprintf(f, "v %f %f %f\n", out_points[i].x, out_points[i].y, out_points[i].z);

    fprintf(f, "\nl");
    for (int i = 1; i <= out_points.size(); i++)
        fprintf(f, " %d", i);

    fclose(f);

    return 0;
}
