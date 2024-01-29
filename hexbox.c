//
// Made by Leonardo in Jan 29th of 2024
// Have any questions or inquiries? Contact me: mdnssknght at tuta dot io
// Happy hacking!
//

#define TB_IMPL

#include "termbox.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//
// 1 kb = 1024 bytes
// 1 mb = 1024 kb (1024 * 1024 bytes)
//
#define BUFFER_SIZE 1024 * 1024

#define BYTE_ZERO "00"
#define OFFSET_ZERO "0x00000000"

#define LUCORNER 0x250c
#define LDCORNER 0x2514
#define RUCORNER 0x2510
#define RDCORNER 0x2518
#define HLINE 0x2500
#define VLINE 0x2502

#define MIN(A, B) ((A) > (B) ? (B) : (A))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

struct box {
	int x, y;
	int w, h;
};

struct cursor_pos {
	int x, y;
};

struct view {
	struct box draw_area;
};

struct hex_view {
	struct view root;
	struct view offsets;
	struct view hexdump;
	struct view strings;
};

//
// draws a simple box, pretty eh?
//
static void draw_box(void) {
	int w = tb_width();
	int h = tb_height();

	tb_set_cell(0, 0, LUCORNER, 0, 0);
	tb_set_cell(w - 1, 0, RUCORNER, 0, 0);

	tb_set_cell(0, h - 1, LDCORNER, 0, 0);
	tb_set_cell(w - 1, h - 1, RDCORNER, 0, 0);

	for (int i = 0 + 1; i < w - 1; i++) {
		tb_set_cell(i, 0, HLINE, 0, 0);
		tb_set_cell(i, h - 1, HLINE, 0, 0);
	}
	
	for (int i = 0 + 1; i < h - 1; i++) {
		tb_set_cell(0, i, VLINE, 0, 0);
		tb_set_cell(w - 1, i, VLINE, 0, 0);
	}

	tb_present();
}

//
// <offset>: <hex dump at offset>
//
// draws the Hex view containing the offset and hex dump
//
static void hex_view(struct hex_view *hexview, uint8_t *buff, uint8_t start) {
	hexview->hexdump.draw_area = hexview->root.draw_area;
	hexview->hexdump.draw_area.x += strlen(OFFSET_ZERO) + 1;
	hexview->hexdump.draw_area.w -= (hexview->root.draw_area.x + hexview->hexdump.draw_area.x) - 1;

	int nsegments = hexview->hexdump.draw_area.w / strlen(BYTE_ZERO " ");

	for (int i = 0; i < hexview->hexdump.draw_area.h - 1; i++) {
		for (int j = 0; j < nsegments; j++) {
			uint8_t byte = buff[(i + start) * nsegments + j];
			tb_printf(hexview->hexdump.draw_area.x + (j * 3), hexview->hexdump.draw_area.y + i,
					0, 0, "%.2X", byte);
		}
	}

	hexview->offsets.draw_area = hexview->root.draw_area;

	for (int i = 0; i < hexview->offsets.draw_area.h - 1; i++) {
		tb_printf(hexview->offsets.draw_area.x, hexview->offsets.draw_area.y + i,
				0, 0, "0x%08x", (i + start) * nsegments);
	}

	tb_present();
}

static bool cursor_pos_clamp(struct cursor_pos *cpos, struct box limit) {
	// we do cpos->{x,y} - limit.{x,y} to normalize the coords
	// i.e if the cursor origin is located at (4, 3) in the terminal buffer
	// and the cursor position is located at (5, 6) in the terminal buffer
	// we subtract the origin cords from the position to get the new position
	// coords with an origin at (0, 0).
	int x = (cpos->x - limit.x) < 0 ? limit.x : MIN(cpos->x, limit.x + limit.w);
	int y = (cpos->y - limit.y) < 0 ? limit.y : MIN(cpos->y, limit.y + limit.h);

	if (cpos->x != x || cpos->y != y) {
		cpos->x = x;
		cpos->y = y;
		return true;
	}

	return false;
}

static void cursor_pos_update(struct cursor_pos cpos) {
	tb_set_cursor(cpos.x, cpos.y);
	tb_present();
}

int main(int argc, char *argv[]) {
	argv++;
	argc--;

	tb_init();

	draw_box();

	FILE *f = fopen(argv[0], "r");
	uint8_t buff[BUFFER_SIZE];

	fread(buff, sizeof(uint8_t), BUFFER_SIZE, f);
	fclose(f);

	struct hex_view hexview = {0};

	hexview.root.draw_area.x = 1;
	hexview.root.draw_area.y = 1;
	hexview.root.draw_area.w = tb_width() - hexview.root.draw_area.x;
	hexview.root.draw_area.h = tb_height() - hexview.root.draw_area.y;

	int start = 0;

	hex_view(&hexview, buff, start);

	struct box cursor_limits = hexview.hexdump.draw_area;
	cursor_limits.w -= hexview.root.draw_area.x + 1;
	cursor_limits.h -= hexview.hexdump.draw_area.y + 1;

	struct cursor_pos cpos = {0};
	cpos.x = hexview.hexdump.draw_area.x;
	cpos.y = hexview.hexdump.draw_area.y;
	cursor_pos_update(cpos);

	struct tb_event ev;
	bool quit = false;

	while (!quit) {
		tb_poll_event(&ev);

		if (ev.type != TB_EVENT_KEY) {
			continue;
		}

		switch (ev.key) {
			case TB_KEY_ARROW_UP:
				cpos.y--;
				if (cursor_pos_clamp(&cpos, cursor_limits)) {
					start = MAX(0, start - 1);
					hex_view(&hexview, buff, start);
				}
				cursor_pos_update(cpos);
				break;
			case TB_KEY_ARROW_DOWN:
				cpos.y++;
				if (cursor_pos_clamp(&cpos, cursor_limits)) {
					start++;
					hex_view(&hexview, buff, start);
				}
				cursor_pos_update(cpos);
				break;
			case TB_KEY_ARROW_LEFT:
				cpos.x--;
				cursor_pos_clamp(&cpos, cursor_limits);
				cursor_pos_update(cpos);
				break;
			case TB_KEY_ARROW_RIGHT:
				cpos.x++;
				cursor_pos_clamp(&cpos, cursor_limits);
				cursor_pos_update(cpos);
				break;
			case TB_KEY_ESC:
				quit = true;
				break;
			default:
				break;
		}
	}

	tb_shutdown();

	return 0;
}
