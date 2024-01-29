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

/* struct hex_view { */
/* 	struct box drawing_area; */
/* 	struct box offsets_drawing_area; */
/* 	struct box hexdump_drawing_area; */
/* 	struct box strings_drawing_area; */
/* }; */

/* struct hex_view hexview_drawing_area; */

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
static void hex_view(uint8_t *buff, uint8_t start, struct box hexview_drawing_area) {
	struct box hexdump_drawing_area = hexview_drawing_area;
	hexdump_drawing_area.x += strlen(OFFSET_ZERO) + 1;
	hexdump_drawing_area.w -= (hexview_drawing_area.x + hexdump_drawing_area.x) - 1;

	int nsegments = hexdump_drawing_area.w / 3;

	for (int i = 0; i < hexdump_drawing_area.h - 1; i++) {
		for (int j = 0; j < nsegments; j++) {
			uint8_t byte = buff[(i + start) * nsegments + j];
			tb_printf(hexdump_drawing_area.x + (j * 3), hexdump_drawing_area.y + i, 0, 0, "%.2X", byte);
		}
	}

	struct box offset_drawing_area = hexview_drawing_area;

	for (int i = 0; i < offset_drawing_area.h - 1; i++) {
		tb_printf(offset_drawing_area.x, offset_drawing_area.y + i, 0, 0, "0x%08x", (i + start) * nsegments);
	}

	tb_present();
}

static bool cursor_pos_clamp(struct cursor_pos *cpos, struct box box) {
	int x = cpos->x < box.x ? box.x : MIN(cpos->x, box.x + box.w);
	int y = cpos->y < box.y ? box.y : MIN(cpos->y, box.y + box.h);

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

	struct box hexview_drawing_area = {0};
	hexview_drawing_area.x = 1;
	hexview_drawing_area.y = 1;
	hexview_drawing_area.w = tb_width() - hexview_drawing_area.x;
	hexview_drawing_area.h = tb_height() - hexview_drawing_area.y;

	int start = 0;

	hex_view(buff, start, hexview_drawing_area);

	struct box hexdump_drawing_area = hexview_drawing_area;
	hexdump_drawing_area.x += strlen(OFFSET_ZERO) + 1;
	hexdump_drawing_area.w -= (hexview_drawing_area.x + hexdump_drawing_area.x) + 1;
	hexdump_drawing_area.h -= hexdump_drawing_area.y + 1;

	struct cursor_pos cpos = {0};
	cpos.x = hexdump_drawing_area.x;
	cpos.y = hexdump_drawing_area.y;
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
				if (cursor_pos_clamp(&cpos, hexdump_drawing_area)) {
					start = MAX(0, start - 1);
					hex_view(buff, start, hexview_drawing_area);
				}
				cursor_pos_update(cpos);
				break;
			case TB_KEY_ARROW_DOWN:
				cpos.y++;
				if (cursor_pos_clamp(&cpos, hexdump_drawing_area)) {
					start++;
					hex_view(buff, start, hexview_drawing_area);
				}
				cursor_pos_update(cpos);
				break;
			case TB_KEY_ARROW_LEFT:
				cpos.x--;
				cursor_pos_clamp(&cpos, hexdump_drawing_area);
				cursor_pos_update(cpos);
				break;
			case TB_KEY_ARROW_RIGHT:
				cpos.x++;
				cursor_pos_clamp(&cpos, hexdump_drawing_area);
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
