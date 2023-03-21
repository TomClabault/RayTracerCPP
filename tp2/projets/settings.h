#define IMAGE_WIDTH 3840*2
#define IMAGE_HEIGHT 2160*2

#define CLIPPING 1
#define BACKFACE_CULLING 1

#define HYBRID_RASTERIZATION_TRACING 1
#define BLOCK_SIZE 32

#define SHADING 1 //1 to use shading, 0 to use COLOR_NORMAL_OR_BARYCENTRIC for the shading. If this is 1, COLOR_NORMAL_OR_BARYCENTRIC is ignored
#define SHADOWS 0 //1 to compute shadows, 0 not to

#define COLOR_NORMAL_OR_BARYCENTRIC 1 //1 for normal, 0 for barycentric
#define MOLLER_TRUMBORE 1
