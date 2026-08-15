#include "render.h"

/** @brief Colors used to paint a pixel-mode half-block sample "lit". */
#define PIXEL_ON_BG BG_WHITE
#define PIXEL_ON_FG WHITE

/** @brief Colors used to paint a pixel-mode half-block sample "unlit". */
#define PIXEL_OFF_BG BG_BLACK
#define PIXEL_OFF_FG BLACK

char *pixel_buffer;

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

	if (pixel_mode) {
		int output_rows = (screen_height + 1) / 2;
		frame_buffer_size = (screen_width * output_rows * 25) + 256;
	} else {
		frame_buffer_size = pixel_count + (screen_height * 2) + 256;
	}
	frame_buffer = malloc(frame_buffer_size);

	if (!pixel_buffer || !frame_buffer) {
		free(pixel_buffer);
		free(frame_buffer);
		fprintf(stderr, "Failed to allocate renderer buffers.\n");
		exit(EXIT_FAILURE);
	}

	memset(pixel_buffer, ' ', pixel_count);
}

void free_framebuffer(void) {
	free(pixel_buffer);
	free(frame_buffer);
	pixel_buffer = NULL;
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

/**
 * @brief Packs two vertical samples per terminal row using the "▄" half-block
 * glyph, doubling effective vertical resolution (top sample -> background
 * color, bottom sample -> foreground color).
 */
static int render_pixel_rows(int frame_buffer_offset) {
	const char *last_top_color = NULL;
	const char *last_bottom_color = NULL;

	for (int y = 0; y < screen_height; y += 2) {
		int top_row_index = y * screen_width;
		int bot_row_index = (y + 1) * screen_width;
		bool has_bottom = (y + 1) < screen_height;

		for (int x = 0; x < screen_width; x++) {
			bool top_on = pixel_buffer[top_row_index + x] != ' ';
			bool bottom_on = has_bottom && pixel_buffer[bot_row_index + x] != ' ';

			const char *top_color = top_on ? PIXEL_ON_BG : PIXEL_OFF_BG;
			const char *bottom_color = bottom_on ? PIXEL_ON_FG : PIXEL_OFF_FG;

			if (top_color != last_top_color) {
				size_t len = strlen(top_color);
				memcpy(frame_buffer + frame_buffer_offset, top_color, len);
				frame_buffer_offset += (int)len;
				last_top_color = top_color;
			}

			if (bottom_color != last_bottom_color) {
				size_t len = strlen(bottom_color);
				memcpy(frame_buffer + frame_buffer_offset, bottom_color, len);
				frame_buffer_offset += (int)len;
				last_bottom_color = bottom_color;
			}

			memcpy(frame_buffer + frame_buffer_offset, "▄", 3);
			frame_buffer_offset += 3;
		}

		size_t r_len = sizeof(RESET_STYLE) - 1;
		memcpy(frame_buffer + frame_buffer_offset, RESET_STYLE, r_len);
		frame_buffer_offset += (int)r_len;
		frame_buffer[frame_buffer_offset++] = '\n';

		last_top_color = NULL;
		last_bottom_color = NULL;
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