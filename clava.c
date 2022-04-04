#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>

#include "ncurses.h"

#define PIXEL "#"

// Coordinate positions X,Y
// Radius
// Velocity VX,VY
struct metaball {
	int y, x, radius;
	float vx, vy;
};

struct metaball *new_metaball(int, int, short);
void free_metaball(struct metaball*);
float Q_rsqrt(float);

int main() {
	WINDOW *wnd = initscr();
	start_color();				/* Enable colour in window */
	raw();						/* Line buffering disabled	*/
	cbreak();
	keypad(stdscr, TRUE);		/* We get F1, F2 etc..		*/
	noecho();					/* Don't echo() while we do getch */
	curs_set(0);

	int row, col;
	getmaxyx(wnd, row, col);	/* Get window dimensions */

	short wax_color = 1;
	init_pair(wax_color, COLOR_RED, COLOR_BLACK);	/* Foreground & background */

	wattron(wnd, COLOR_PAIR(wax_color));
	char msg[] = "Totally a lava lamp";
	mvwprintw(wnd, 1, (col-strlen(msg))/2, "%s", msg); /* move to x,y, print to w */

	struct metaball *ball = new_metaball(15, 15, 5);
	struct metaball *ball1 = new_metaball(15, 30, 5);

	size_t size = 2;
	float threshold = 1.5f;
	int factor = 1;
	struct metaball *vec = malloc(size * sizeof(struct metaball));
	*vec = *ball;
	*(vec + sizeof(vec)) = *ball1;  

	int frameCount = 0;
	int frameTime = 60; // Time in milliseconds
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
		for(int x = 0; x < col; x++) {
			for(int y = 0; y < row; y++) {
				float falloff_sum = 0;
				int counter = 0;

				for(struct metaball *v = vec; v < vec + size*sizeof(vec); v += sizeof(vec)) {
					float dx = (float)x - v->x;
					float dy = ((float)y - v->y) * 2; // Font is taller than it is wide
					float dr = ((float)v->radius * v->radius);
					falloff_sum += dr / (dx*dx + dy*dy);

					counter++;
				}

				// printf("%f; ", falloff_sum);
				// printf("counter: %d;", counter); 

				if(falloff_sum > threshold) {
					// Check if within bounds
					if(y < row && y > 0 &&
							x < col && x > 0) {
						// Draw point
						mvwprintw(wnd, y, x, "%s", PIXEL);
					}
				}
			}
		}


		// Move balls
		// Random slow horizontal movement
		// Bouyant from bottom to top, with pauses in between
		for(struct metaball *v = vec; v < vec + size*sizeof(vec); v += sizeof(vec)) {	
			// Only move X points per frame
			
			// Normalize X,Y
			float norm_x = (float)v->x / col;
			float norm_y = (float)v->y / row;
			// Between -0.5 and 0.5
			float norm_signed_x = norm_x - 0.5f;
			float norm_signed_y = norm_y - 0.5f;

			// Seed rand and get random scaling factor for displacement
			srand(time(NULL));
			float factor_x = (float)rand() / RAND_MAX * 0.5;
			float factor_y = (float)rand() / RAND_MAX * 0.5;

			v->vx += -1 * norm_signed_x * factor_x;
			v->vy += -1 * norm_signed_y * factor_y;

			v->x += v->vx;
			v->y += v->vy;
			
			/*
			if(v->x > col)
				v->x = col;
			else if(v->x < 0)
				v->x = 0;
			if(v->y > row)
				v->y = row;
			else if(v->y < 0)
				v->y = 0;
			*/

			counter++;
			mvwprintw(wnd, row - counter, 0, "VX: %f, VY: %f, X: %d, Y: %d"
					, v->vx, v->vy, v->x, v->y); 
		}

		mvwprintw(wnd, row-5, 1, "Frame: %d", frameCount);
		frameCount++;
		wrefresh(wnd);
	}

	wattroff(wnd, COLOR_PAIR(wax_color));

	free_metaball(ball);
	free_metaball(ball1);
	free(vec);

	endwin();

	return 0;
}

struct metaball *new_metaball(int y, int x, short radius) {
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

// https://en.wikipedia.org/wiki/Fast_inverse_square_root 
float Q_rsqrt(float number)
{
	union {
		float    f;
		uint32_t i;
	} conv = { .f = number };
	conv.i  = 0x5f3759df - (conv.i >> 1);
	conv.f *= 1.5F - (number * 0.5F * conv.f * conv.f);
	return conv.f;
}
