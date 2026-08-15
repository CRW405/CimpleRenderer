#include "term_ops.h"

void term_op(int op_count, bool flush, ...) {
	va_list args;
	va_start(args, flush);

	for (int i = 0; i < op_count; i++) {
		char *code = va_arg(args, char *);
		printf("%s", code);
	}
	va_end(args);

	if (flush)
		fflush(stdout);
}

void init_screen() {
	term_op(6, true, CLEAR, HIDE_CUR, ALT_SCREEN, ENABLE_MOUSE_SGR,
	        ENABLE_MOUSE, ENABLE_FOCUS);
}

void reset_term() {
	term_op(5, true, SHOW_CUR, MAIN_SCREEN, DISABLE_MOUSE_SGR, DISABLE_MOUSE,
	        DISABLE_FOCUS);
}

Termios enable_raw_term() {
	Termios orig, raw;
	tcgetattr(STDIN_FILENO, &orig);
	raw = orig;
	raw.c_lflag &= ~(ECHO | ICANON);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	return orig;
}

bool get_terminal_bounds(int *width, int *height) {
	struct winsize ws;
	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0 || ws.ws_col == 0 ||
	    ws.ws_row <= 1) {
		return false;
	}

	*width = ws.ws_col;
	*height = (ws.ws_row - 1) * 2;
	return true;
}
