#include "input.h"

static bool left_mouse_held = false;
static bool right_mouse_held = false;

enum {
	SCROLL_WHEEL_BIT = 0x40,
	SCROLL_DIRECTION_BIT = 0x01
};

enum {
	ESCAPE_SEQUENCE_CAPACITY = 64,
	MOUSE_SEQUENCE_OFFSET = 2,
	MOUSE_X_ORIGIN_OFFSET = 1,
	MOUSE_Y_ORIGIN_OFFSET = 1
};

typedef void (*KeyAction)(void);

typedef struct {
	char key;
	KeyAction action;
} KeyBinding;

typedef struct {
	char escape_final;
	char mapped_key;
} EscapeBinding;

typedef struct {
	char event_type;
	int button_id;
	bool *target;
	bool value;
} MouseStateBinding;

static void quit_renderer(void) { running = false; }

static const KeyBinding key_registry[] = {
	{ 'q', quit_renderer }
};

static const EscapeBinding arrow_registry[] = {
	{ 'A', '^' },
	{ 'B', 'V' },
	{ 'C', '>' },
	{ 'D', '<' }
};

static const MouseStateBinding mouse_state_registry[] = {
	{ 'M', 0, &left_mouse_held,  true  },
	{ 'M', 2, &right_mouse_held, true  },
	{ 'm', 0, &left_mouse_held,  false },
	{ 'm', 2, &right_mouse_held, false }
};

static int read_char(char *out) { return read(STDIN_FILENO, out, 1); }

static bool is_escape_terminator(char c) {
	return c == 'A' || c == 'B' || c == 'C' || c == 'D' || c == 'M' || c == 'm';
}

static int read_escape_sequence(char *seq, size_t seq_size) {
	size_t i = 0;
	while (isInput() && i < seq_size - 1) {
		if (read_char(&seq[i]) != 1) {
			break;
		}
		if (is_escape_terminator(seq[i])) {
			i++;
			break;
		}
		i++;
	}
	seq[i] = '\0';
	return (int)i;
}

static void apply_key_binding(char key) {
	for (size_t i = 0; i < sizeof(key_registry) / sizeof(key_registry[0]); i++) {
		if (key_registry[i].key == key) {
			key_registry[i].action();
			return;
		}
	}
}

static void apply_arrow_binding(const char *seq) {
	for (size_t i = 0; i < sizeof(arrow_registry) / sizeof(arrow_registry[0]); i++) {
		if (arrow_registry[i].escape_final == seq[1]) {
			last_input = arrow_registry[i].mapped_key;
			return;
		}
	}
}

static void update_mouse_button_state(char event_type, int mouse_button) {
	int button_id = mouse_button & 0b11;
	for (size_t i = 0;
	     i < sizeof(mouse_state_registry) / sizeof(mouse_state_registry[0]); i++) {
		const MouseStateBinding *binding = &mouse_state_registry[i];
		if (binding->event_type == event_type && binding->button_id == button_id) {
			*binding->target = binding->value;
		}
	}
}

static void set_mouse_position(int x, int y) {
	mouse_x = x - MOUSE_X_ORIGIN_OFFSET;
	mouse_y = y - MOUSE_Y_ORIGIN_OFFSET;
}

static void handle_mouse_event(const char *seq, int seq_len) {
	char event_type = seq[seq_len - 1];
	int parsed_button = 0;
	int parsed_x = 0;
	int parsed_y = 0;

	if (sscanf(seq + MOUSE_SEQUENCE_OFFSET, "%d;%d;%d",
	           &parsed_button, &parsed_x, &parsed_y) == 3) {
		set_mouse_position(parsed_x, parsed_y);

		// Wheel events (button bit 0x40 set) aren't tied to a click state.
		if ((parsed_button & SCROLL_WHEEL_BIT) == 0) {
			update_mouse_button_state(event_type, parsed_button);
		} else {
			scroll_delta += (parsed_button & SCROLL_DIRECTION_BIT) ? 1 : -1;
		}
	}

	last_input = event_type;
}

static void handle_escape_input(void) {
	char seq[ESCAPE_SEQUENCE_CAPACITY];
	int seq_len = read_escape_sequence(seq, sizeof(seq));
	if (seq_len < 2 || seq[0] != '[') {
		return;
	}

	if (seq[1] == '<') {
		handle_mouse_event(seq, seq_len);
		return;
	}

	apply_arrow_binding(seq);
}

int isInput(void) {
	Timeval timeout = { 0, 0 };
	fd_set readfds;
	FD_ZERO(&readfds);
	FD_SET(STDIN_FILENO, &readfds);
	return select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);
}

void handle_input() {
	while (isInput()) {
		char input_char = '\0';
		if (read_char(&input_char) != 1) {
			continue;
		}

		if (input_char == (unsigned char)27) {
			handle_escape_input();
			continue;
		}

		apply_key_binding(input_char);
		last_input = input_char;
	}
}
