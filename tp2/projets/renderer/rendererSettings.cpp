#include "rendererSettings.h"

std::ostream& operator << (std::ostream& os, const RenderSettings& settings)
{
    os << "Render[" << (settings.hybrid_rasterization_tracing ? "Rast" : "RT") << ", " << settings.image_width << "x" << settings.image_height;
    if (settings.enable_ssaa)
        os << ", " << "SSAAx" << settings.ssaa_factor;
    if (settings.enable_bvh)
        os << ", " << "BVH[LObjC=" << settings.bvh_leaf_object_count << ", maxDepth=" << settings.bvh_max_depth;

    os << "]";

    return os;
}
