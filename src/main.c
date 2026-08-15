#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "draw.h"
#include "input.h"
#include "obj.h"
#include "render.h"
#include "term_ops.h"

bool running = true;
int screen_width;
int screen_height;
char *frame_buffer;
int frame_buffer_size;
int fps;
int target_fps = TARGET_FPS;
char last_input = ' ';
int mouse_x = 0;
int mouse_y = 0;
int scroll_delta = 0;
double scale = 1.0;
double fov = 1.5;

static const double ZOOM_SPEED = 0.5;
static const double MIN_DISTANCE = 1.0;
static const double MAX_DISTANCE = 50.0;

void handle_sigint(int sig) {
	(void)sig;
	running = false;
}

void render_scene(Mesh *scene) {
	static double distance = 5.0;
	static double angle_x = 0.0;
	static double angle_y = 0.0;

	if (last_input == 'w')
		angle_x -= 0.05;
	if (last_input == 's')
		angle_x += 0.05;
	if (last_input == 'a')
		angle_y -= 0.05;
	if (last_input == 'd')
		angle_y += 0.05;
	last_input = ' ';

	distance += scroll_delta * ZOOM_SPEED;
	if (distance < MIN_DISTANCE)
		distance = MIN_DISTANCE;
	if (distance > MAX_DISTANCE)
		distance = MAX_DISTANCE;
	scroll_delta = 0;

	memset(pixel_buffer, ' ', screen_width * screen_height);
	for (size_t i = 0; i < scene->face_count; i++) {
		Face *face = &scene->faces[i];
		Point2D p1 = project(*face->vertex1, screen_width, screen_height, angle_x, angle_y, distance);
		Point2D p2 = project(*face->vertex2, screen_width, screen_height, angle_x, angle_y, distance);
		Point2D p3 = project(*face->vertex3, screen_width, screen_height, angle_x, angle_y, distance);

		draw_line(p1.x, p1.y, p2.x, p2.y, pixel_buffer, screen_width, screen_height, '#');
		draw_line(p2.x, p2.y, p3.x, p3.y, pixel_buffer, screen_width, screen_height, '#');
		draw_line(p3.x, p3.y, p1.x, p1.y, pixel_buffer, screen_width, screen_height, '#');
	}
}

int main(int argc, char *argv[]) {
	int opt;
	int set_width = 1;
	int set_height = 1;
	bool width_set = false;
	bool height_set = false;
	int term_width = 0;
	int term_height = 0;
	const char *filepath = "../obj/cube.obj";

	static const struct option long_options[] = {
		{ "fps",    required_argument, NULL, 'f' },
		{ "width",  required_argument, NULL, 'w' },
		{ "height", required_argument, NULL, 'h' },
		{ "obj",    required_argument, NULL, 'o' },
		{ "scale",  required_argument, NULL, 's' },
		{ "fov",    required_argument, NULL, 'v' },
		{ NULL,     0,                 NULL, 0   }
	};

	while ((opt = getopt_long(argc, argv, "w:h:f:o:s:v:", long_options, NULL)) != -1) {
		switch (opt) {
		case 'w':
			set_width = atoi(optarg) > 1 ? atoi(optarg) : set_width;
			width_set = true;
			break;
		case 'h':
			set_height = atoi(optarg) > 1 ? atoi(optarg) : set_height;
			height_set = true;
			break;
		case 'f':
			target_fps = atoi(optarg) != 0 ? atoi(optarg) : TARGET_FPS;
			break;
		case 'o':
			filepath = optarg;
			break;
		case 's':
			scale = atof(optarg) > 0 ? atof(optarg) : scale;
			break;
		case 'v':
			fov = atof(optarg) > 0 ? atof(optarg) : fov;
			break;
		}
	}

	if (get_terminal_bounds(&term_width, &term_height)) {
		if (!width_set)
			set_width = term_width;
		if (!height_set)
			set_height = term_height;
		if (set_width > term_width)
			set_width = term_width;
		if (set_height > term_height)
			set_height = term_height;
	}

	Mesh *scene = malloc(sizeof(Mesh));
	if (!scene)
		return EXIT_FAILURE;

	if (parse_obj(filepath, scene) != 0) {
		fprintf(stderr, "Failed to load OBJ file: %s\n", filepath);
		free_mesh(scene);
		return EXIT_FAILURE;
	}

	center_mesh(scene);

	signal(SIGINT, handle_sigint);
	init_framebuffer(set_width, set_height);
	init_screen();

	Termios orig_term = enable_raw_term();
	Timespec start_time, end_time;
	long elapsed_time, total_frame_time, sleep_time;

	// fps is limited by sleeping until target fps is reached
	while (running) {
		clock_gettime(CLOCK_MONOTONIC, &start_time);

		render_scene(scene);
		render();
		handle_input();

		clock_gettime(CLOCK_MONOTONIC, &end_time);
		elapsed_time = (end_time.tv_sec - start_time.tv_sec) * 1000000 +
		               (end_time.tv_nsec - start_time.tv_nsec) / 1000;

		total_frame_time = elapsed_time;
		int frame_time = 1000000 / target_fps;
		if (elapsed_time < frame_time) {
			sleep_time = frame_time - elapsed_time;
			usleep(sleep_time);
			total_frame_time += sleep_time;
		}

		if (total_frame_time > 0) {
			fps = 1000000 / total_frame_time;
		}
	}

	free_mesh(scene);
	free_framebuffer();
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
	reset_term();
	return EXIT_SUCCESS;
}
