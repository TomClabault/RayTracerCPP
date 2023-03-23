#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080

#define ENABLE_SSAA 1
//Super sampling factor. How many times larger will the image be rendered
#define SSAA_FACTOR 6

//Enable triangle clipping. 
//This can be safely disabled to save performance if your objects fit in
//the view frustum
#define CLIPPING 1
//Whether or not to render triangles that are facing away from the camera
#define BACKFACE_CULLING 1

//Whether or not to use 
#define HYBRID_RASTERIZATION_TRACING 1

//1 to use shading, 0 to use COLOR_NORMAL_OR_BARYCENTRIC for the shading. 
//If this is 1, COLOR_NORMAL_OR_BARYCENTRIC is ignored
#define SHADING 0
//1 to compute shadows, 0 not to
#define SHADOWS 0

//1 to color the triangles with the normal
//0 to color the triangles with the barycentric coordinates
//This parameter is only used if the SHADING define is set to 0
#define COLOR_NORMAL_OR_BARYCENTRIC 1
//1 to use the Moller Trumbore ray-triangle intersection algorithm. 
//0 to use the naive (barycentric coordinates) ray-triangle intersection algorithm
#define MOLLER_TRUMBORE 1
