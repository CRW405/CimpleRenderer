#ifndef RENDER_H
#define RENDER_H

/**
 * @file render.h
 * @brief Character buffer management and frame rendering for terminal output.
 */

#include "common.h"

/**
 * @brief Allocates the character buffer and terminal frame buffer for the
 * given dimensions, and clears every cell to a space.
 * @param width Render width in cells.
 * @param height Render height in cells.
 */
void init_framebuffer(int width, int height);

/** @brief Frees buffers allocated by @ref init_framebuffer. */
void free_framebuffer(void);

/**
 * @brief Renders the current frame into the terminal.
 *
 * Builds the frame in @ref frame_buffer from @ref pixel_buffer and writes it
 * to stdout, including both the rendered view and the status line.
 */
void render(void);

#endif