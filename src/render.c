#include "render.h"

char *pixel_buffer;
unsigned char *shade_buffer;

static char gui_buffer[256];

static char *gui(void) {
	snprintf(gui_buffer, sizeof(gui_buffer),
	         "FPS: %d/%d | %dx%d | Mouse: %d, %d%s\033[K",
	         fps, target_fps, screen_width, screen_height, mouse_x, mouse_y,
	         pixel_mode ? " | Pixel" : "");
	return gui_buffer;
}

void init_framebuffer(int width, int height) {
	screen_width = width;
	screen_height = height;
	int pixel_count = screen_width * screen_height;
	pixel_buffer = malloc(pixel_count);
	shade_buffer = malloc(pixel_count);
	depth_buffer = malloc(sizeof(float) * pixel_count);

	if (pixel_mode) {
		// Each cell may emit two 24-bit truecolor escapes (bg + fg, up to
		// ~19 bytes each) plus the 3-byte half-block glyph.
		int output_rows = (screen_height + 1) / 2;
		frame_buffer_size = (screen_width * output_rows * 48) + 256;
	} else {
		frame_buffer_size = pixel_count + (screen_height * 2) + 256;
	}
	frame_buffer = malloc(frame_buffer_size);

	if (!pixel_buffer || !shade_buffer || !frame_buffer) {
		free(pixel_buffer);
		free(shade_buffer);
		free(frame_buffer);
		fprintf(stderr, "Failed to allocate renderer buffers.\n");
		exit(EXIT_FAILURE);
	}

	memset(pixel_buffer, ' ', pixel_count);
	memset(shade_buffer, 0, pixel_count);
}

void free_framebuffer(void) {
	free(pixel_buffer);
	free(shade_buffer);
	free(frame_buffer);
	free(depth_buffer);
	pixel_buffer = NULL;
	shade_buffer = NULL;
	frame_buffer = NULL;
}

static int render_ascii_rows(int frame_buffer_offset) {
	for (int y = 0; y < screen_height; y++) {
		memcpy(frame_buffer + frame_buffer_offset,
		       pixel_buffer + (y * screen_width), screen_width);
		frame_buffer_offset += screen_width;
		frame_buffer[frame_buffer_offset++] = '\n';
	}

	return frame_buffer_offset;
}

/** @brief Darkest gray a lit half-block sample can render as, so even a
 *  fully unlit face stays visually distinct from the background. */
#define SHADE_FLOOR 40

/**
 * @brief Maps a light intensity byte to a grayscale RGB triplet.
 * @param shade Light intensity in [0, 255], from @ref shade_buffer.
 * @param on Whether this sample is part of a filled/drawn cell at all.
 * @param r,g,b Output color channels.
 */
static void shade_to_rgb(unsigned char shade, bool on, unsigned char *r, unsigned char *g, unsigned char *b) {
	if (!on) {
		*r = *g = *b = 0;
		return;
	}

	int v = SHADE_FLOOR + (int)((255 - SHADE_FLOOR) * (shade / 255.0));
	*r = *g = *b = (unsigned char)v;
}

/**
 * @brief Packs two vertical samples per terminal row using the "▄" half-block
 * glyph, doubling effective vertical resolution (top sample -> background
 * color, bottom sample -> foreground color). Each sample's color is a
 * grayscale shade driven by @ref shade_buffer, giving real per-pixel lighting
 * instead of a flat on/off color.
 */
static int render_pixel_rows(int frame_buffer_offset) {
	int last_top_r = -1, last_top_g = -1, last_top_b = -1;
	int last_bot_r = -1, last_bot_g = -1, last_bot_b = -1;

	for (int y = 0; y < screen_height; y += 2) {
		int top_row_index = y * screen_width;
		int bot_row_index = (y + 1) * screen_width;
		bool has_bottom = (y + 1) < screen_height;

		for (int x = 0; x < screen_width; x++) {
			int top_idx = top_row_index + x;
			bool top_on = pixel_buffer[top_idx] != ' ';
			unsigned char top_r, top_g, top_b;
			shade_to_rgb(shade_buffer[top_idx], top_on, &top_r, &top_g, &top_b);

			unsigned char bot_r = 0, bot_g = 0, bot_b = 0;
			if (has_bottom) {
				int bot_idx = bot_row_index + x;
				bool bottom_on = pixel_buffer[bot_idx] != ' ';
				shade_to_rgb(shade_buffer[bot_idx], bottom_on, &bot_r, &bot_g, &bot_b);
			}

			if (top_r != last_top_r || top_g != last_top_g || top_b != last_top_b) {
				frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
				                                 frame_buffer_size - frame_buffer_offset,
				                                 "\033[48;2;%d;%d;%dm", top_r, top_g, top_b);
				last_top_r = top_r;
				last_top_g = top_g;
				last_top_b = top_b;
			}

			if (bot_r != last_bot_r || bot_g != last_bot_g || bot_b != last_bot_b) {
				frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
				                                 frame_buffer_size - frame_buffer_offset,
				                                 "\033[38;2;%d;%d;%dm", bot_r, bot_g, bot_b);
				last_bot_r = bot_r;
				last_bot_g = bot_g;
				last_bot_b = bot_b;
			}

			memcpy(frame_buffer + frame_buffer_offset, "▄", 3);
			frame_buffer_offset += 3;
		}

		size_t r_len = sizeof(RESET_STYLE) - 1;
		memcpy(frame_buffer + frame_buffer_offset, RESET_STYLE, r_len);
		frame_buffer_offset += (int)r_len;
		frame_buffer[frame_buffer_offset++] = '\n';

		last_top_r = last_top_g = last_top_b = -1;
		last_bot_r = last_bot_g = last_bot_b = -1;
	}

	return frame_buffer_offset;
}

void render() {
	int frame_buffer_offset = 0;

	memcpy(frame_buffer + frame_buffer_offset, CUR_TO_TOP, sizeof(CUR_TO_TOP) - 1);
	frame_buffer_offset += sizeof(CUR_TO_TOP) - 1;

	if (pixel_mode) {
		memcpy(frame_buffer + frame_buffer_offset, RESET_STYLE, sizeof(RESET_STYLE) - 1);
		frame_buffer_offset += sizeof(RESET_STYLE) - 1;
		frame_buffer_offset = render_pixel_rows(frame_buffer_offset);
	} else {
		frame_buffer_offset = render_ascii_rows(frame_buffer_offset);
	}

	frame_buffer_offset += snprintf(frame_buffer + frame_buffer_offset,
	                                frame_buffer_size - frame_buffer_offset,
	                                "%s", gui());

	if (frame_buffer_offset >= frame_buffer_size) {
		frame_buffer[frame_buffer_size - 1] = '\0';
	} else {
		frame_buffer[frame_buffer_offset] = '\0';
	}

	write(STDOUT_FILENO, frame_buffer, frame_buffer_offset);
}
