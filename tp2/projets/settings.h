#define IMAGE_WIDTH 3840*4
#define IMAGE_HEIGHT 2160*4

#define BACKFACE_CULLING 1

#define HYBRID_RASTERIZATION_TRACING 1
#define BLOCK_SIZE 32

#define SHADING 1 //1 to use shading, 0 to use COLOR_NORMAL_OR_BARYCENTRIC for the shading. If this is 1, COLOR_NORMAL_OR_BARYCENTRIC is ignored
#define COLOR_NORMAL_OR_BARYCENTRIC 1 //1 for normal, 0 for barycentric
#define MOLLER_TRUMBORE 1
