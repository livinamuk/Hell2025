#pragma once
#include "Hell/Render/API/OpenGL/Types/GL_mesh_buffer_old.h"

#define GRASS_SIZE 40

struct GrassMesh {

    void Create();

    glm::vec3 m_bladePoints[GRASS_SIZE][GRASS_SIZE];

    OpenGLMeshBufferOLD glMesh;
};