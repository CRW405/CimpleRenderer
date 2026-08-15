#ifndef OBJ_H
#define OBJ_H

/**
 * @file obj.h
 * @brief OBJ file parsing: mesh data structures and load/free helpers.
 */

#include <stddef.h>

/** @brief A single 3D vertex. */
typedef struct {
	double x;
	double y;
	double z;
} Vertex;

/** @brief A vertex normal; shares the Vertex layout. */
typedef Vertex Normal;

/** @brief A triangular face referencing vertices and a normal by pointer. */
typedef struct {
	Vertex *vertex1;
	Vertex *vertex2;
	Vertex *vertex3;
	Normal *normal;
} Face;

/** @brief A loaded mesh: vertex/normal/face arrays plus counts. */
typedef struct {
	Vertex *vertices;
	size_t vertex_count;
	Normal *normals;
	size_t normal_count;
	Face *faces;
	size_t face_count;
} Mesh;

/**
 * @brief Loads an OBJ file (v/vn/f lines) into a new @ref Mesh.
 * @param path Filesystem path to the OBJ file.
 * @param mesh Out-parameter; populated on success.
 * @return 0 on success, non-zero on failure.
 */
int parse_obj(const char *path, Mesh *mesh);

/**
 * @brief Frees arrays owned by @p mesh and the mesh itself.
 * @param mesh Mesh to free; NULL is safe.
 */
void free_mesh(Mesh *mesh);

void center_mesh(Mesh *mesh);

#endif
