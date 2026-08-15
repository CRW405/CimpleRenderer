#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#include "common.h"
#include "draw.h"

Point2D project(Vertex v, int width, int height, double angle_x, double angle_y, double distance) {
	double x1 = v.x * scale * cos(angle_y) - v.z * scale * sin(angle_y);
	double z1 = v.x * scale * sin(angle_y) + v.z * scale * cos(angle_y);
	double y1 = v.y * scale;

	double y2 = y1 * cos(angle_x) - z1 * sin(angle_x);
	double z2 = y1 * sin(angle_x) + z1 * cos(angle_x);

	y2 -= 0.5;
	z2 += distance;

	double x_proj = (x1 / z2) * fov;
	double y_proj = (y2 / z2) * fov;

	Point2D p;
	p.x = (int)((x_proj + 1.0) * 0.5 * width);
	p.y = (int)((1.0 - y_proj) * 0.5 * height);

	return p;
}

void draw_line(int x0, int y0, int x1, int y1, char *pixel_buffer, int width, int height, char symbol) {
	int dx = abs(x1 - x0);
	int dy = abs(y1 - y0);
	int sx = (x0 < x1) ? 1 : -1;
	int sy = (y0 < y1) ? 1 : -1;
	int err = dx - dy;

	while (true) {
		if (x0 >= 0 && x0 < width && y0 >= 0 && y0 < height) {
			pixel_buffer[y0 * width + x0] = symbol;
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

	// double nx = face->normal->x * cos(angle_y) - face->normal->z * sin(angle_y);
	double nz = face->normal->x * sin(angle_y) + face->normal->z * cos(angle_y);
	double ny = face->normal->y;

	// double ny_rotated = ny * cos(angle_x) - nz * sin(angle_x);
	double nz_rotated = ny * sin(angle_x) + nz * cos(angle_x);

	return nz_rotated >= 0;
}

void fill_triangle(Point2D p1, Point2D p2, Point2D p3, char *pixel_buffer, int width, int height, char symbol) {
	Point2D tmp;
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
	for (int y = 0; y < total_height; y++) {
		bool second_half = y > (p2.y - p1.y) || (p2.y == p1.y);
		int segment_height = second_half ? (p3.y - p2.y) : (p2.y - p1.y);
		float alpha = (float)y / total_height;
		float beta = (float)(y - (second_half ? (p2.y - p1.y) : 0)) / segment_height;

		Point2D A = { .x = (int)(p1.x + (p3.x - p1.x) * alpha), .y = p1.y + y };
		Point2D B = second_half ? (Point2D){ .x = (int)(p2.x + (p3.x - p2.x) * beta), .y = p2.y + (y - (p2.y - p1.y)) } : (Point2D){ .x = (int)(p1.x + (p2.x - p1.x) * beta), .y = p1.y + y };

		if (A.x > B.x) {
			Point2D tmp = A;
			A = B;
			B = tmp;
		}

		for (int x = A.x; x <= B.x; x++) {
			if (x >= 0 && x < width && A.y >= 0 && A.y < height) {
				pixel_buffer[A.y * width + x] = symbol;
			}
		}
	}
}
