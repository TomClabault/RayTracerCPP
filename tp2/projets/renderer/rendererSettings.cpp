#include "rendererSettings.h"

RenderSettings RenderSettings::basic_settings(int width, int height, bool hybrid_raster_trace)
{
    RenderSettings settings;
    settings.image_width = width;
    settings.image_height = height;
    settings.hybrid_rasterization_tracing = hybrid_raster_trace;
    settings.compute_shadows = false;

    return settings;
}

RenderSettings RenderSettings::ssaa_settings(int width, int height, int ssaa_factor, bool hybrid_raster_trace, bool compute_shadows)
{
    RenderSettings settings;
    settings.image_width = width;
    settings.image_height = height;
    settings.enable_ssaa = true;
    settings.ssaa_factor = ssaa_factor;
    settings.hybrid_rasterization_tracing = hybrid_raster_trace;
    settings.compute_shadows = compute_shadows;

    return settings;
}

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
