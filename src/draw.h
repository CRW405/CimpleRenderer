#ifndef DRAW_H
#define DRAW_H

/**
 * @file draw.h
 * @brief 3D-to-2D projection and 2D line rasterization.
 */

#include "obj.h"

/** @brief A 2D screen-space point. old implementation. */
typedef struct {
	int x;
	int y;
} Point2D;

/** @brief A 3D vertex projected into screen space. new implementation*/
typedef struct {
	int x;
	int y;
	double z; /**< Camera-space depth; kept sub-integer precision for z-buffering. */
} ProjectedVertex;

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
ProjectedVertex project(Vertex v, int width, int height, double angle_x, double angle_y, double distance);

/**
 * @brief Rasterizes a line (Bresenham) into a character buffer.
 * @param x0,y0 Start point.
 * @param x1,y1 End point.
 * @param pixel_buffer Flat character buffer, width * height.
 * @param width Buffer width in cells.
 * @param height Buffer height in cells.
 * @param symbol Character to write along the line.
 */
void draw_line(int x0, int y0, int x1, int y1, char *pixel_buffer, unsigned char *shade_buffer, int width, int height, char symbol, unsigned char shade);

/**
 * @brief Determines if a face is back-facing relative to the camera.
 * @param face Face to test.
 * @param angle_x Camera rotation angle around the X-axis in radians.
 * @param angle_y Camera rotation angle around the Y-axis in radians.
 * @return true if the face is back-facing, false otherwise.
 */
bool is_backface(Face *face, double angle_x, double angle_y);

void fill_triangle(ProjectedVertex p1, ProjectedVertex p2, ProjectedVertex p3, char *pixel_buffer, float *depth_buffer, unsigned char *shade_buffer, int width, int height, char symbol, unsigned char shade);

/**
 * @brief Computes diffuse light intensity for a face against a fixed light direction.
 * @param face Face to test.
 * @param angle_x Camera rotation angle around the X-axis in radians.
 * @param angle_y Camera rotation angle around the Y-axis in radians.
 * @return Light intensity clamped to [0.0, 1.0].
 */
double get_shade_intensity(Face *face, double angle_x, double angle_y);

/**
 * @brief Buckets a face's shade intensity into a ramp index.
 * @param face Face to test.
 * @param angle_x Camera rotation angle around the X-axis in radians.
 * @param angle_y Camera rotation angle around the Y-axis in radians.
 * @param ramp_size Number of discrete shade levels available.
 * @return An index in [0, ramp_size - 1].
 */
int get_shade_level(Face *face, double angle_x, double angle_y, int ramp_size);

#endif
