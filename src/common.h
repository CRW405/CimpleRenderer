#ifndef COMMON_H
#define COMMON_H

/**
 * @file common.h
 * @brief Shared constants, types, and global runtime state for the renderer.
 */

#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define TARGET_FPS 60

#define CLEAR "\033[2J"
#define CUR_TO_TOP "\033[H"
#define HIDE_CUR "\033[?25l"
#define SHOW_CUR "\033[?25h"
#define ALT_SCREEN "\033[?1049h"
#define MAIN_SCREEN "\033[?1049l"
#define ENABLE_MOUSE "\033[?1003h"
#define DISABLE_MOUSE "\033[?1003l"
#define ENABLE_MOUSE_SGR "\033[?1006h"
#define DISABLE_MOUSE_SGR "\033[?1006l"
#define ENABLE_FOCUS "\033[?1004h"
#define DISABLE_FOCUS "\033[?1004l"

#define BG_BLACK "\033[40m"
#define BG_RED "\033[41m"
#define BG_GREEN "\033[42m"
#define BG_YELLOW "\033[43m"
#define BG_BLUE "\033[44m"
#define BG_MAGENTA "\033[45m"
#define BG_CYAN "\033[46m"
#define BG_WHITE "\033[47m"
#define BG_GRAY "\033[48;5;240m"
#define BG_PURPLE "\033[48;5;129m"
#define BG_BROWN "\033[48;5;94m"
#define BG_LGRAY "\033[48;5;250m"
#define BG_ORANGE "\033[48;5;208m"

#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"
#define GRAY "\033[38;5;240m"
#define PURPLE "\033[38;5;129m"
#define BROWN "\033[38;5;94m"
#define LGRAY "\033[38;5;250m"
#define ORANGE "\033[38;5;208m"
#define BG_DKOLIVE "\033[48;5;58m"
#define DKOLIVE "\033[38;5;58m"
#define LIME "\033[38;5;46m"
#define BG_LIME "\033[48;5;46m"

#define RESET_STYLE "\033[0m"

/** @brief Alias for POSIX terminal settings type. */
typedef struct termios Termios;

/** @brief Alias for POSIX timeval type. */
typedef struct timeval Timeval;

/** @brief Alias for POSIX timespec type. */
typedef struct timespec Timespec;

/** @brief Main loop control flag; false exits the app. */
extern bool running;

/** @brief Render target width in cells. */
extern int screen_width;

/** @brief Render target height in cells. */
extern int screen_height;

/** @brief Character buffer for the current frame, sized screen_width * screen_height. */
extern char *pixel_buffer;

/** @brief Render output buffer written to stdout each frame. */
extern char *frame_buffer;

/** @brief Capacity of @ref frame_buffer in bytes. */
extern int frame_buffer_size;

/** @brief Last measured frames-per-second value. */
extern int fps;

/** @brief User target frames-per-second cap. */
extern int target_fps;

/** @brief Last processed input key/event marker. */
extern char last_input;

/** @brief Last terminal mouse X coordinate (0-based). */
extern int mouse_x;

/** @brief Last terminal mouse Y coordinate (0-based). */
extern int mouse_y;

/** @brief Accumulated scroll wheel ticks since last consumed (positive = scroll down, negative = scroll up). */
extern int scroll_delta;

/** @brief Object scale factor. */
extern double scale;

/** @brief Field of view factor. */
extern double fov;

/**
 * @brief Enables pixel rendering mode (-x/--pixel).
 *
 * When set, the renderer packs two vertical samples into each terminal row
 * using the "▄" half-block glyph (background = top sample, foreground =
 * bottom sample), doubling effective vertical resolution instead of printing
 * one plain ASCII character per row.
 */
extern bool pixel_mode;

#endif
