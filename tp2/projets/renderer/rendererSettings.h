#ifndef RENDERER_SETTINGS_H
#define RENDERER_SETTINGS_H

#include <iostream>

struct RenderSettings
{
    enum ShadingMethod
    {
        //Shades by ray tracing the material of the triangle
        RT_SHADING,

        //Shades with the std::abs(normal) of the triangle
        ABS_NORMALS_SHADING,

        //Shades with the (normal + 1) * 0.5 of the triangle
        PASTEL_NORMALS_SHADING,

        //Shades with the barycentric coordinates of the triangle
        BARYCENTRIC_COORDINATES_SHADING,

        //Shades with a white material that helps visualize where
        //ambient occlusion has been applied
        VISUALIZE_AO,
    };

    RenderSettings() {}
    RenderSettings(int width, int height) : image_width(width), image_height(height) {}

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
    bool hybrid_rasterization_tracing = false;

    //Shading method used to shade the triangles
    ShadingMethod shading_method = RT_SHADING;

    //true to compute shadows, false not to
    bool compute_shadows = false;

    //Maximum recursion depth allowed for reflections / refractions / ...
    int max_recursion_depth = 5;

    //Whether or not to use a BVH to intersect the scene
    bool enable_bvh = true;
    //Maximum depth of the BVH tree
    int bvh_max_depth = 13;
    //Maximum number of objects per leaf of the BVH tree if the maximum recursion depth
    //defined by bvh_max_depth hasn't been reached
    int bvh_leaf_object_count = 60;

    //Whether or not to enable post-processing-screen-space ambient occlusion
    bool enable_ssao = false;
    //Number of sample for the SSAO. The higher the sample count, the more precise
    //and less noisy the SSAO but this also means higher computation times
    int ssao_sample_count = 64;
    //Radius within which to look for occlusion
    float ssao_radius = 0.5;
    //Direct multiplier on the SSAO occlusion strength
    float ssao_amount = 1.0;

    //Whether or not to compute the ambient component of 'RT_SHADING'
    bool enable_ambient = true;
    //Whether or not to compute the diffuse component of 'RT_SHADING'
    bool enable_diffuse = true;
    //Whether or not to compute the specular component of 'RT_SHADING'
    bool enable_specular = true;
    //Whether or not to compute the emissive component of 'RT_SHADING'
    bool enable_emissive = true;
    //Number of rays to trace to compute the average color of a rough reflection
    int rough_reflections_sample_count = 8;

    //Whether or not to use a texture to compute the ambient occlusion
    bool enable_ao_mapping = false;
    //Whether or not to use a texture to compute the diffuse color
    bool enable_diffuse_mapping = false;
    //Whether or not to get the normals from a texture
    bool enable_normal_mapping = false;
    //Whether or not to use displacement mapping (steep occlusion mapping)
    bool enable_displacement_mapping = false;
    float displacement_mapping_strength = 0.02f;
    int parallax_mapping_steps = 32;

    //Whether or not to sample the background color from a texture or not
    //If false, the background color of the scene will be a plain color
    bool enable_skysphere = false;//Mapping from equirectangular image to skysphere
    bool enable_skybox = false;//Mapping from cube map to skybox

    friend std::ostream& operator << (std::ostream& os, const RenderSettings& settings);
};

#endif
