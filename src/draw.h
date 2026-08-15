#ifndef DRAW_H
#define DRAW_H

/**
 * @file draw.h
 * @brief 3D-to-2D projection and 2D line rasterization.
 */

#include "obj.h"

/** @brief A 2D screen-space point. */
typedef struct {
	int x;
	int y;
} Point2D;

/**
 * @brief Projects a 3D vertex into screen space using a rotating camera angle.
 * @param v Vertex in object space.
 * @param width Render width in cells.
 * @param height Render height in cells.
 * @param angle Camera rotation angle in radians.
 * @param angle_x Camera rotation angle around the X-axis in radians.
 * @param angle_y Camera rotation angle around the Y-axis in radians.
 * @return The projected 2D point.
 */
Point2D project(Vertex v, int width, int height, double angle_x, double angle_y, double distance);

/**
 * @brief Rasterizes a line (Bresenham) into a character buffer.
 * @param x0,y0 Start point.
 * @param x1,y1 End point.
 * @param pixel_buffer Flat character buffer, width * height.
 * @param width Buffer width in cells.
 * @param height Buffer height in cells.
 * @param symbol Character to write along the line.
 */
void draw_line(int x0, int y0, int x1, int y1, char *pixel_buffer, int width, int height, char symbol);

#endif
