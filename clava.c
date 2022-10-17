#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "ncurses.h"

#define PIXEL "#"

#define MIN(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

// Coordinate positions X,Y
// Radius
// Velocity VX,VY
struct metaball {
	int y, x, radius;
	float vx, vy;
};

struct metaball *new_metaball(int, int, int);
void free_metaball(struct metaball*);

int main() {
	WINDOW *wnd = initscr();
	start_color();				/* Enable colour in window */
	raw();						/* Line buffering disabled	*/
	cbreak();
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();					/* Don't echo() while we do getch */
	curs_set(0);
	srand(time(NULL));
	use_default_colors();

	int row, col;
	getmaxyx(wnd, row, col);	/* Get window dimensions */

	short wax_color = 1;
	init_pair(wax_color, COLOR_RED, -1);	/* Foreground & background */

	wattron(wnd, COLOR_PAIR(wax_color));
	char msg[] = "Totally a lava lamp";
	mvwprintw(wnd, 1, (col-strlen(msg))/2, "%s", msg); /* move to x,y, print to w */

	size_t size = 16;
	float threshold = 1.8f;
	int factor = 1;
	struct metaball *vec[] = {
		// Floating blobs
		new_metaball(15, 15, 10),
		new_metaball(15, 45, 5),
		// Init Floor
		new_metaball(row, col / 2, row / 3.0f),
		new_metaball(row, col / 2 + 10, row / 3.0f),
		new_metaball(row, col / 2 + 20, row / 3.0f),
		new_metaball(row + 1, col / 2 - 10, row / 3.0f),
		new_metaball(row + 1, col / 2 - 20, row / 3.0f),
		new_metaball(row + 1, col / 2 + 33, row / 3.0f),
		new_metaball(row + 1, col / 2 - 33, row / 3.0f),
		// Init Ceiling
		new_metaball(0 - 1, col / 2, row / 3.5f),
		new_metaball(0 - 1, col / 2 + 10, row / 3.5f),
		new_metaball(0 - 1, col / 2 - 10, row / 3.5f),
		new_metaball(0 - 2, col / 2 + 20, row / 4.0f),
		new_metaball(0 - 2, col / 2 - 20, row / 4.0f),
		new_metaball(0 - 2, col / 2 + 25, row / 4.0f),
		new_metaball(0 - 2, col / 2 - 25, row / 4.0f),
	};

	int frameCount = 0;
	int frameTime = 180; // Time in milliseconds
	int isQuit = 0;
	while(!isQuit) {
		// Clear window for next run
		wclear(wnd);
		// Get user input

		int counter = 0;

		// Update every set interval
		struct timespec ts;
		int res;
		ts.tv_sec = frameTime / 1000;
		ts.tv_nsec = (frameTime % 1000) * 1000000;
		do {
			res = nanosleep(&ts, &ts);
		} while (res && errno == EINTR);

		// Draw to screen
		for(int x = 0; x <= col; x++) {
			for(int y = 0; y <= row; y++) {
				float falloff_sum = 0;
				int counter = 0;

				for(int i = 0; i < size; i++) {
					struct metaball *v = vec[i];
					float dx = (float)x - v->x;
					float dy = ((float)y - v->y) * 2; // Font is taller than it is wide
					float dr = ((float)v->radius * v->radius);
					falloff_sum += dr / (dx*dx + dy*dy);

					counter++;
				}

				// printf("%f; ", falloff_sum);
				// printf("counter: %d;", counter); 

				if(falloff_sum > threshold) {
					mvwprintw(wnd, y, x, "%s", PIXEL);
				}
			}
		}


		// Move balls
		// Random slow horizontal movement
		// Bouyant from bottom to top, with pauses in between
		for(int i = 0; i < size; i++) {
			struct metaball *v = vec[i];

			// Normalize X,Y
			float norm_x = (float)v->x / col;
			float norm_y = (float)v->y / row;
			// Between negative and positive value
			float norm_signed_x = norm_x - 0.5f;
			float norm_signed_y = norm_y * 3 - 1.5f;

			// Seed rand and get random scaling factor for displacement
			float factor_x = (float)rand() / RAND_MAX * 0.2;
			float factor_y = (float)rand() / RAND_MAX;

			// Horizontal movespeed = Min(inverse radius, X)
			// Vertical movespeed = inverse radius * distance from center
			float moveSpeed_x = MIN(1.0f, 1/(float)v->radius);
			float moveSpeed_y = (((float)v->y / (float)row) / 2) / (float)v->radius;
			
			/*
			counter++;
			mvwprintw(wnd, row - counter, 0
				, "Move X: %f, Move Y: %f"
				, moveSpeed_x, moveSpeed_y);
			*/

			v->vx += -1 * norm_signed_x * factor_x * moveSpeed_x;
			v->vy += -1 * norm_signed_y * factor_y * moveSpeed_y;

			v->x += v->vx;
			v->y += v->vy;

			if(v->x > col)
				v->x = col;
			else if(v->x < 0)
				v->x = 0;
			if(v->y > row)
				v->y = row;
			else if(v->y < 0)
				v->y = 0;
			/*
			mvwprintw(wnd, row - counter, 0, "VX: %f, VY: %f, X: %d, Y: %d"
					, v->vx, v->vy, v->x, v->y);
			*/
		}

		// mvwprintw(wnd, row-5, 1, "Frame: %d", frameCount);
		frameCount++;
		wrefresh(wnd);
	}

	wattroff(wnd, COLOR_PAIR(wax_color));
	
	for(int i = 0; i < size; i++) {
		free_metaball(vec[i]);
	}

	endwin();

	return 0;
}

struct metaball *new_metaball(int y, int x, int radius) {
	struct metaball *ball = malloc(sizeof(struct metaball));
	ball->y = y;
	ball->x = x;
	ball->radius = radius;

	ball->vx = 0.0f;
	ball->vy = 0.0f;

	return ball;
}

void free_metaball(struct metaball *ball) {
	if(ball != NULL) {
		free(ball);
	}
}
