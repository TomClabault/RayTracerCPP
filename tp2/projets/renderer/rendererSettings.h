#ifndef RENDERER_SETTINGS_H
#define RENDERER_SETTINGS_H

#include <iostream>

struct RenderSettings
{
    RenderSettings() {}
    RenderSettings(int width, int height) : image_width(width), image_height(height) {}

    static RenderSettings basic_settings(int width, int height, bool hybrid_raster_trace = true);
    static RenderSettings ssaa_settings(int width, int height, int ssaa_factor, bool hybrid_raster_trace = true, bool compute_shadows = false);

    int image_width = 1024;
    int image_height = 1024;

    bool enable_ssaa = false;
    //Super sampling factor. How many times larger will the image be rendered
    int ssaa_factor = 2;

    //Enable triangle clipping when rasterizing.
    //This can be safely disabled to save performance if your objects fit in
    //the view frustum
    bool enable_clipping = true;

    //Whether or not to use rasterization to first determine the visibility of the
    //triangles and then ray tracing for the rest of computations (shadows, reflections, ...)
    //0 for full ray-tracing
    //1 for rasterization/ray-tracing
    bool hybrid_rasterization_tracing = true;

    //1 to use shading, false to use 'color_normal_or_barycentric' for the shading.
    //If this is true, 'color_normal_or_barycentric' is ignored
    bool use_shading = true;
    //true to compute shadows, false not to
    bool compute_shadows = true;

    //true to color the triangles with the normal
    //false to color the triangles with the barycentric coordinates
    //This parameter is only used if the SHADING define is set to false
    bool color_normal_or_barycentric = true;

    //Whether or not to use a BVH to intersect the scene
    bool enable_bvh = true;
    //Maximum depth of the BVH tree
    int bvh_max_depth = 13;
    //Maximum number of objects per leaf of the BVH tree if the maximum recursion depth
    //defined by bvh_max_depth hasn't been reached
    int bvh_leaf_object_count = 60;

    //Whether or not to enable post-processing-screen-space ambient occlusion
    bool enable_ssao = true;
    //Number of sample for the SSAO. The higher the sample count, the more precise
    //and less noisy the SSAO but this also means higher computation times
    int ssao_sample_count = 64;
    //Number of samples used to randomly rotate the ssao samples
    int ssao_noise_size = 16;//TODO remove
    //Radius within which to look for occlusion
    float ssao_radius = 0.5;
    //Direct multiplier on the SSAO occlusion strength
    float ssao_amount = 1.0;

    friend std::ostream& operator << (std::ostream& os, const RenderSettings& settings);
};

#endif
