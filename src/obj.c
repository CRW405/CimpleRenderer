#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "obj.h"

/** @brief Largest number of vertices supported on a single "f" line (n-gon). */
enum { MAX_FACE_VERTICES = 64 };

Vertex parse_vertex(const char *line) {
	Vertex v;
	sscanf(line, "v %lf %lf %lf", &v.x, &v.y, &v.z);
	return v;
}

Normal parse_normal(const char *line) {
	Normal n;
	sscanf(line, "vn %lf %lf %lf", &n.x, &n.y, &n.z);
	return n;
}

/**
 * @brief Counts the vertex references on a single "f" line, regardless of
 * face-vertex index format (v, v/vt, v//vn, v/vt/vn) or vertex count (tris,
 * quads, n-gons).
 */
static int count_face_vertices(const char *line) {
	int count = 0;
	const char *p = line + 1; // skip leading 'f'

	while (*p != '\0' && *p != '\n' && *p != '\r') {
		while (*p == ' ' || *p == '\t')
			p++;
		if (*p == '\0' || *p == '\n' || *p == '\r')
			break;
		count++;
		while (*p != '\0' && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r')
			p++;
	}

	return count;
}

/**
 * @brief Parses a single face-vertex token ("v", "v/vt", "v//vn", or
 * "v/vt/vn") into its vertex and (optional) normal index.
 * @param out_normal_index Set to the parsed normal index, or 0 if the token
 * has no normal reference.
 */
static int parse_face_vertex_token(const char *token, int *out_normal_index) {
	int v = 0, vt = 0, vn = 0;

	if (sscanf(token, "%d/%d/%d", &v, &vt, &vn) == 3) {
		*out_normal_index = vn;
	} else if (sscanf(token, "%d//%d", &v, &vn) == 2) {
		*out_normal_index = vn;
	} else if (sscanf(token, "%d/%d", &v, &vt) == 2) {
		*out_normal_index = 0;
	} else {
		sscanf(token, "%d", &v);
		*out_normal_index = 0;
	}

	return v;
}

/**
 * @brief Parses one "f" line and fan-triangulates it into @p faces.
 *
 * @param line Mutable line buffer; tokenized in place (contents are not
 * needed after this call, matching the caller's fgets-then-discard usage).
 * @return The number of triangles written (vertex_count - 2).
 */
static size_t parse_face_line(char *line, Vertex *vertices, Normal *normals,
                               size_t normal_count, Face *faces) {
	int v_index[MAX_FACE_VERTICES];
	int n_index[MAX_FACE_VERTICES];
	int count = 0;

	strtok(line, " \t\r\n"); // discard the leading "f" token
	char *token = strtok(NULL, " \t\r\n");
	while (token && count < MAX_FACE_VERTICES) {
		v_index[count] = parse_face_vertex_token(token, &n_index[count]);
		count++;
		token = strtok(NULL, " \t\r\n");
	}

	if (count < 3)
		return 0;

	Normal *fallback_normal = (normal_count > 0) ? &normals[0] : NULL;

	for (int i = 1; i < count - 1; i++) {
		Face *face = &faces[i - 1];
		face->vertex1 = &vertices[v_index[0] - 1];
		face->vertex2 = &vertices[v_index[i] - 1];
		face->vertex3 = &vertices[v_index[i + 1] - 1];
		face->normal = (n_index[0] > 0) ? &normals[n_index[0] - 1] : fallback_normal;
	}

	return (size_t)(count - 2);
}

int parse_obj(const char *path, Mesh *mesh) {
	FILE *file = fopen(path, "r");
	if (!file)
		return 1;

	char line[256];
	size_t vertex_count = 0;
	size_t normal_count = 0;
	size_t face_count = 0;

	while (fgets(line, sizeof(line), file)) {
		switch (line[0]) {
		case 'v':
			if (line[1] == 'n') {
				normal_count++;
			} else if (line[1] == ' ') {
				vertex_count++;
			}
			break;
		case 'f': {
			int face_vertices = count_face_vertices(line);
			if (face_vertices >= 3)
				face_count += (size_t)(face_vertices - 2);
			break;
		}
		}
	}

	Vertex *vertices = malloc(vertex_count * sizeof(Vertex));
	Normal *normals = malloc(normal_count * sizeof(Normal));
	Face *faces = malloc(face_count * sizeof(Face));

	size_t vertex_index = 0;
	size_t normal_index = 0;
	size_t face_index = 0;

	rewind(file);

	while (fgets(line, sizeof(line), file)) {
		switch (line[0]) {
		case 'v':
			if (line[1] == 'n') {
				normals[normal_index++] = parse_normal(line);
			} else if (line[1] == ' ') {
				vertices[vertex_index++] = parse_vertex(line);
			}
			break;
		case 'f':
			face_index += parse_face_line(line, vertices, normals, normal_count,
			                               &faces[face_index]);
			break;
		}
	}

	mesh->vertices = vertices;
	mesh->vertex_count = vertex_count;
	mesh->normals = normals;
	mesh->normal_count = normal_count;
	mesh->faces = faces;
	mesh->face_count = face_count;

	fclose(file);
	return 0;
}

void free_mesh(Mesh *mesh) {
	if (!mesh)
		return;

	free(mesh->vertices);
	free(mesh->normals);
	free(mesh->faces);

	free(mesh);
}

void center_mesh(Mesh *mesh) {
	if (mesh->vertex_count == 0)
		return;

	double min_x = mesh->vertices[0].x, max_x = mesh->vertices[0].x;
	double min_y = mesh->vertices[0].y, max_y = mesh->vertices[0].y;
	double min_z = mesh->vertices[0].z, max_z = mesh->vertices[0].z;

	for (size_t i = 1; i < mesh->vertex_count; i++) {
		if (mesh->vertices[i].x < min_x)
			min_x = mesh->vertices[i].x;
		if (mesh->vertices[i].x > max_x)
			max_x = mesh->vertices[i].x;
		if (mesh->vertices[i].y < min_y)
			min_y = mesh->vertices[i].y;
		if (mesh->vertices[i].y > max_y)
			max_y = mesh->vertices[i].y;
		if (mesh->vertices[i].z < min_z)
			min_z = mesh->vertices[i].z;
		if (mesh->vertices[i].z > max_z)
			max_z = mesh->vertices[i].z;
	}

	double center_x = (min_x + max_x) / 2.0;
	double center_y = (min_y + max_y) / 2.0;
	double center_z = (min_z + max_z) / 2.0;

	for (size_t i = 0; i < mesh->vertex_count; i++) {
		mesh->vertices[i].x -= center_x;
		mesh->vertices[i].y -= center_y;
		mesh->vertices[i].z -= center_z;
	}
}
