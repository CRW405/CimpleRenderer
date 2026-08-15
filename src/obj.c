#include <stdio.h>
#include <stdlib.h>

#include "obj.h"

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

Face parse_face(const char *line, Vertex *vertices, Normal *normals) {
	Face face;
	int v1, v2, v3, n1, n2, n3;

	sscanf(line, "f %d//%d %d//%d %d//%d", &v1, &n1, &v2, &n2, &v3, &n3);

	face.vertex1 = &vertices[v1 - 1];
	face.vertex2 = &vertices[v2 - 1];
	face.vertex3 = &vertices[v3 - 1];
	face.normal = &normals[n1 - 1];

	return face;
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
		case 'f':
			face_count++;
			break;
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
			faces[face_index++] = parse_face(line, vertices, normals);
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
