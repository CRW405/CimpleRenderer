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

void fill_triangle(ProjectedVertex p1, ProjectedVertex p2, ProjectedVertex p3, char *pixel_buffer, float *depth_buffer, unsigned char *shade_buffer, int width, int height, char symbol, unsigned char shade) {
	// Sort vertices by Y ascending
	ProjectedVertex tmp;
	if (p1.y > p2.y) {
		tmp = p1;
		p1 = p2;
		p2 = tmp;
	}
	if (p1.y > p3.y) {
		tmp = p1;
		p1 = p3;
		p3 = tmp;
	}
	if (p2.y > p3.y) {
		tmp = p2;
		p2 = p3;
		p3 = tmp;
	}

	int total_height = p3.y - p1.y;
	if (total_height == 0)
		return;

	for (int y = 0; y <= total_height; y++) {
		bool second_half = y > (p2.y - p1.y) || (p2.y == p1.y);
		int segment_height = second_half ? (p3.y - p2.y) : (p2.y - p1.y);
		if (segment_height == 0)
			continue;

		float alpha = (float)y / total_height;
		float beta = (float)(y - (second_half ? (p2.y - p1.y) : 0)) / segment_height;

		int ax = (int)(p1.x + (p3.x - p1.x) * alpha);
		int ay = p1.y + y;
		float az = p1.z + (p3.z - p1.z) * alpha;

		int bx = second_half ? (int)(p2.x + (p3.x - p2.x) * beta) : (int)(p1.x + (p2.x - p1.x) * beta);
		float bz = second_half ? (p2.z + (p3.z - p2.z) * beta) : (p1.z + (p2.z - p1.z) * beta);

		if (ax > bx) {
			int tx = ax;
			ax = bx;
			bx = tx;
			float tz = az;
			az = bz;
			bz = tz;
		}

		int span_width = bx - ax;
		for (int x = ax; x <= bx; x++) {
			if (x >= 0 && x < width && ay >= 0 && ay < height) {
				float t_span = (span_width == 0) ? 0.0f : (float)(x - ax) / span_width;
				float current_z = az + (bz - az) * t_span;

				int idx = ay * width + x;
				if (current_z < depth_buffer[idx]) {
					depth_buffer[idx] = current_z;
					pixel_buffer[idx] = symbol;
					shade_buffer[idx] = shade;
				}
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
