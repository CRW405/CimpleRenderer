#ifndef TERM_OPS_H
#define TERM_OPS_H

/**
 * @file term_ops.h
 * @brief Terminal mode/control helpers used by startup, frame loop, and exit.
 */

#include "common.h"

/**
 * @brief Writes a variadic list of terminal control codes to stdout.
 * @param op_count Number of string codes that follow.
 * @param flush Whether to flush stdout after writing.
 * @param ... Terminal control code strings (const char *).
 */
void term_op(int op_count, bool flush, ...);

/**
 * @brief Initializes terminal presentation for interactive rendering.
 *
 * Clears screen, hides cursor, switches to alternate screen, and enables
 * mouse/focus tracking modes.
 */
void init_screen(void);

/**
 * @brief Restores terminal presentation settings changed by @ref init_screen.
 */
void reset_term(void);

/**
 * @brief Enables raw terminal input mode.
 * @return Original terminal settings to be restored on shutdown.
 */
Termios enable_raw_term(void);

/**
 * @brief Reads current terminal bounds and maps them to render dimensions.
 *
 * @p height is the literal number of terminal lines available for the
 * rendered view (one row is reserved for the status line). Callers that need
 * a doubled sample count for pixel mode's half-block packing are responsible
 * for applying that doubling themselves.
 *
 * @param width Output pointer for render width in cells.
 * @param height Output pointer for render height in terminal lines.
 * @return true when bounds were read successfully; otherwise false.
 */
bool get_terminal_bounds(int *width, int *height);

#endif
