#include "mat.h"
#include "triangle.h"
#include "vec.h"

#include <vector>

class ObjUtils
{
public:
    static std::vector<Triangle> readObj(const char* filePath, const Transform transform, Vector& pMin, Vector& pMax);
    static std::vector<Triangle> readObj(const char* filePath, const Transform transform);
    static std::vector<Triangle> readObj(const char* filePath);
};
