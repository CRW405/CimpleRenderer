#include "render.h"

char *pixel_buffer;

static char gui_buffer[256];

static char *gui(void) {
	snprintf(gui_buffer, sizeof(gui_buffer),
	         "FPS: %d/%d | %dx%d | Mouse: %d, %d\033[K",
	         fps, target_fps, screen_width, screen_height, mouse_x, mouse_y);
	return gui_buffer;
}

void init_framebuffer(int width, int height) {
	screen_width = width;
	screen_height = height;
	int pixel_count = screen_width * screen_height;
	pixel_buffer = malloc(pixel_count);
	frame_buffer_size = pixel_count + (screen_height * 2) + 256;
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

void render() {
	int frame_buffer_offset = 0;

	memcpy(frame_buffer + frame_buffer_offset, CUR_TO_TOP, sizeof(CUR_TO_TOP) - 1);
	frame_buffer_offset += sizeof(CUR_TO_TOP) - 1;

	for (int y = 0; y < screen_height; y++) {
		memcpy(frame_buffer + frame_buffer_offset,
		       pixel_buffer + (y * screen_width), screen_width);
		frame_buffer_offset += screen_width;
		frame_buffer[frame_buffer_offset++] = '\n';
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