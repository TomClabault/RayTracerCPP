#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080

#define ENABLE_SSAA 0
//Super sampling factor. How many times larger will the image be rendered
#define SSAA_FACTOR 6

//Enable triangle clipping when rasterizing. 
//This can be safely disabled to save performance if your objects fit in
//the view frustum
#define CLIPPING 1
//Whether or not to render triangles that are facing away from the camera
#define BACKFACE_CULLING 1

//Whether or not to use rasterization to first determine the visibility of the 
//triangles and then ray tracing for the rest of computations (shadows, reflections, ...)
//0 for full ray-tracing
//1 for rasterization/ray-tracing
#define HYBRID_RASTERIZATION_TRACING 0

//1 to use shading, 0 to use COLOR_NORMAL_OR_BARYCENTRIC for the shading. 
//If this is 1, COLOR_NORMAL_OR_BARYCENTRIC is ignored
#define SHADING 1
//1 to compute shadows, 0 not to
#define SHADOWS 1

//1 to color the triangles with the normal
//0 to color the triangles with the barycentric coordinates
//This parameter is only used if the SHADING define is set to 0
#define COLOR_NORMAL_OR_BARYCENTRIC 1
//1 to use the Moller Trumbore ray-triangle intersection algorithm. 
//0 to use the naive (barycentric coordinates) ray-triangle intersection algorithm
#define MOLLER_TRUMBORE 1

//The maximum number of triangles that a leaf of the BVH can contain
#define ENABLE_BVH 1
#define BVH_MAX_DEPTH 13
#define BVH_LEAF_TRIANGLE_MAX_COUNT 8
