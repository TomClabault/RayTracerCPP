#ifndef  MAIN_UTILS_H
#define MAIN_UTILS_H

#include "meshIOUtils.h"
#include "renderer.h"

float loadOBJ(MeshIOData& meshData, std::vector<Triangle>& triangles);

/*
 * Does some pre-computations on the materials. Notably the specular visiblity threshold
 */
void precompute_materials(Materials& materials);

float render(Renderer& renderer);
float writeImage(Renderer& renderer, const char* filepath);

#endif // ! MAIN_UTILS_H
