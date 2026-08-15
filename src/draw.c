#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "common.h"
#include "draw.h"

ProjectedVertex project(Vertex v, int width, int height, double angle_x, double angle_y, double distance) {
	double x1 = v.x * scale * cos(angle_y) - v.z * scale * sin(angle_y);
	double z1 = v.x * scale * sin(angle_y) + v.z * scale * cos(angle_y);
	double y1 = v.y * scale;

	double y2 = y1 * cos(angle_x) - z1 * sin(angle_x);
	double z2 = y1 * sin(angle_x) + z1 * cos(angle_x);

	y2 -= 0.5;
	z2 += distance;

	double x_proj = (x1 / z2) * fov;
	double y_proj = (y2 / z2) * fov;

	ProjectedVertex p;
	p.x = (int)((x_proj + 1.0) * 0.5 * width);
	p.y = (int)((1.0 - y_proj) * 0.5 * height);
	p.z = z2;

	return p;
}

void draw_line(int x0, int y0, int x1, int y1, char *pixel_buffer, unsigned char *shade_buffer, int width, int height, char symbol, unsigned char shade) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true) {
		if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
			int idx = y0 * width + x0;
			pixel_buffer[idx] = symbol;
			shade_buffer[idx] = shade;
		}

		if (x0 == x1 && y0 == y1)
			break;

		int err2 = 2 * err;
		if (err2 > -dy) {
			err -= dy;
			x0 += sx;
		}
		if (err2 < dx) {
			err += dx;
			y0 += sy;
		}
	}
}

bool is_backface(Face *face, double angle_x, double angle_y) {
	if (!face->normal)
		return false;

	double nx = face->normal->x;
	double ny = face->normal->y;
	double nz = face->normal->z;

	double z1 = nx * sin(angle_y) + nz * cos(angle_y);
	double y1 = ny;

	double z2 = y1 * sin(angle_x) + z1 * cos(angle_x);

	// If z2 >= 0, the face normal is pointing away from the camera
	return z2 >= 0;
}

/** @brief Twice the signed area of triangle ABP; sign indicates which side of
 *  edge AB the point P falls on. Integer math keeps this exact (no float
 *  rounding), which is what makes shared edges between adjacent triangles
 *  resolve identically on both sides. */
static long edge_function(ProjectedVertex a, ProjectedVertex b, int px, int py) {
	return (long)(b.x - a.x) * (py - a.y) - (long)(b.y - a.y) * (px - a.x);
}

/**
 * @brief Classifies a directed edge as "top" or "left" per the standard
 * rasterization fill rule.
 *
 * For any non-degenerate edge, exactly one of its two directions (A->B vs
 * B->A) is top-left. Biasing that direction's edge test to be inclusive
 * (and the other exclusive) means a pixel that lands exactly on a shared
 * edge is claimed by exactly one of the two triangles that share it - never
 * both (an overlap, which reads as z-fighting where two nearly-coplanar
 * triangles flicker between each other) and never neither (a 1px gap).
 */
static bool is_top_left_edge(int dx, int dy) {
	return (dy < 0) || (dy == 0 && dx > 0);
}

void fill_triangle(ProjectedVertex p1, ProjectedVertex p2, ProjectedVertex p3, char *pixel_buffer, float *depth_buffer, unsigned char *shade_buffer, int width, int height, char symbol, unsigned char shade) {
	long area = edge_function(p1, p2, p3.x, p3.y);
	if (area == 0)
		return; // degenerate: zero screen-space area

	if (area < 0) {
		// Normalize winding so `area` and every edge weight below are positive.
		ProjectedVertex tmp = p2;
		p2 = p3;
		p3 = tmp;
		area = -area;
	}

	int min_x = p1.x < p2.x ? (p1.x < p3.x ? p1.x : p3.x) : (p2.x < p3.x ? p2.x : p3.x);
	int max_x = p1.x > p2.x ? (p1.x > p3.x ? p1.x : p3.x) : (p2.x > p3.x ? p2.x : p3.x);
	int min_y = p1.y < p2.y ? (p1.y < p3.y ? p1.y : p3.y) : (p2.y < p3.y ? p2.y : p3.y);
	int max_y = p1.y > p2.y ? (p1.y > p3.y ? p1.y : p3.y) : (p2.y > p3.y ? p2.y : p3.y);

	if (min_x < 0)
		min_x = 0;
	if (min_y < 0)
		min_y = 0;
	if (max_x > width - 1)
		max_x = width - 1;
	if (max_y > height - 1)
		max_y = height - 1;

	int bias0 = is_top_left_edge(p3.x - p2.x, p3.y - p2.y) ? 0 : -1;
	int bias1 = is_top_left_edge(p1.x - p3.x, p1.y - p3.y) ? 0 : -1;
	int bias2 = is_top_left_edge(p2.x - p1.x, p2.y - p1.y) ? 0 : -1;

	for (int y = min_y; y <= max_y; y++) {
		for (int x = min_x; x <= max_x; x++) {
			long w0 = edge_function(p2, p3, x, y) + bias0;
			long w1 = edge_function(p3, p1, x, y) + bias1;
			long w2 = edge_function(p1, p2, x, y) + bias2;

			if (w0 < 0 || w1 < 0 || w2 < 0)
				continue;

			// Barycentric weights (undo the fill-rule bias first) interpolate
			// full-precision depth across the triangle, instead of chaining
			// per-scanline float lerps that lose precision along the way.
			double b0 = (double)(w0 - bias0) / (double)area;
			double b1 = (double)(w1 - bias1) / (double)area;
			double b2 = (double)(w2 - bias2) / (double)area;
			double current_z = b0 * p1.z + b1 * p2.z + b2 * p3.z;

			int idx = y * width + x;
			if (current_z < depth_buffer[idx]) {
				depth_buffer[idx] = (float)current_z;
				pixel_buffer[idx] = symbol;
				shade_buffer[idx] = shade;
			}
		}
	}
}

double get_shade_intensity(Face *face, double angle_x, double angle_y) {
	if (!face->normal)
		return 1.0;

	double nx = face->normal->x;
	double ny = face->normal->y;
	double nz = face->normal->z;

	double x1 = nx * cos(angle_y) - nz * sin(angle_y);
	double z1 = nx * sin(angle_y) + nz * cos(angle_y);
	double y1 = ny;

	double y2 = y1 * cos(angle_x) - z1 * sin(angle_x);
	double z2 = y1 * sin(angle_x) + z1 * cos(angle_x);

	double light_dir_x = 0.0;
	double light_dir_y = -1.0;
	double light_dir_z = -1.0;

	double light_dot = x1 * light_dir_x + y2 * light_dir_y + z2 * light_dir_z;
	if (light_dot < 0.0)
		light_dot = 0.0;
	if (light_dot > 1.0)
		light_dot = 1.0;

	return light_dot;
}

int get_shade_level(Face *face, double angle_x, double angle_y, int ramp_size) {
	double intensity = get_shade_intensity(face, angle_x, angle_y);
	return (int)(intensity * (ramp_size - 1));
}
