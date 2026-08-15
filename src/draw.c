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

	y2 -= 2.0;
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
