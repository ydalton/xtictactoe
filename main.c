#include <X11/Xlib.h> 

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <math.h>

#define WINDOW_SIZE		512
#define PADDING 		32
#define NUMBER_OF_BOXES 	3
#define CIRCLE_RADIUS 		64
#define NUMBER_OF_ANGLES	20

#define NAME "xtictactoe"

struct game {
	int tiles[NUMBER_OF_BOXES][NUMBER_OF_BOXES];
	int squares;
};

static void draw_x(Display *dpy, Window win, GC gc, int x, int y, int width, 
		int height)
{
	XDrawLine(dpy, win, gc, x, y, x + width, y + height);
	XDrawLine(dpy, win, gc, x, y + height, x + width, y);
}

static void draw_polygon(Display *dpy, Window win, GC gc, int x, int y, int r, size_t n) 
{
	float f;
	float ratio = (M_PI * 2/n); 

	/* draw the center */
	XDrawPoint(dpy, win, gc, x, y);

	for(f = 0; f < M_PI * 2; f += ratio) {
		XDrawLine(dpy, win, gc, sin(f) * r + x, cos(f) * r + y,
				sin(f+ratio) * r + x, cos(f+ratio) * r + y);
	}
}

/* returns a positive random number, -1 on failure */
static int random_number(int max)
{
	FILE *fp;
	int rand_byte;

	fp = fopen("/dev/urandom", "r");
	if(!fp) {
		fprintf(stderr, "What\n");
		exit(EXIT_FAILURE);
	}

	rand_byte = fgetc(fp);

	fclose(fp);

	if(rand_byte == -1)
		return -1;

	/* TODO: replace with constant */
	rand_byte = (float) rand_byte / ((float) 255 / max);

	return rand_byte;
}

static void ai_make_move(struct game *g)
{
	/* for now, pick a random number and go from there */
	int rand;
	size_t x, y;

	rand = random_number(g->squares);

	assert(rand <= g->squares);

	for(y = 0; y < NUMBER_OF_BOXES; y++) {
		for(x = 0; x < NUMBER_OF_BOXES; x++) {
			if(g->tiles[y][x])
				continue;
			rand--;
			if(!rand) {
				g->tiles[y][x] = 2;
				g->squares--;
				return;
			}
		}
	}
}

static void draw_board(Display *dpy, Window win, GC gc, int width, int height)
{
	XDrawLine(dpy, win, gc, PADDING, height/3, 
			width-PADDING, height/3);
	XDrawLine(dpy, win, gc, PADDING, 2*height/3, 
			width-PADDING, 2*height/3);
	XDrawLine(dpy, win, gc, width/3, PADDING, 
			width/3, height-PADDING);
	XDrawLine(dpy, win, gc, 2*width/3, PADDING, 
			2*width/3, height-PADDING);
}

int main(void)
{
	XWindowAttributes attrs;
	Display *dpy;
	Window win, rootwin;
	XEvent ev;
	GC gc;
	int scr;

	struct game new_game = {0};
	new_game.squares = NUMBER_OF_BOXES * NUMBER_OF_BOXES;

	dpy = XOpenDisplay(NULL);
	if(!dpy) {
		fprintf(stderr, "Failed to open display!\n");
		return EXIT_FAILURE;
	}

	scr = DefaultScreen(dpy);

	/* get the root window */
	rootwin = RootWindow(dpy, scr);

	/* create window */
	win = XCreateSimpleWindow(dpy, rootwin, 0, 0, WINDOW_SIZE, WINDOW_SIZE, 
			0, BlackPixel(dpy, scr), WhitePixel(dpy, scr));

	/* set the window title to whatever we want */
	XStoreName(dpy, win, NAME);

	/* 
	 * select the inputs we want to trap, in this case Expose (see below)
	 * and button (mouse) events
	 */
	XSelectInput(dpy, win, ExposureMask | ButtonPressMask);

	/* show the window */
	XMapWindow(dpy, win);

	gc = DefaultGC(dpy, scr);

	for(;;) {
		size_t y, x;

		/* should always be 9 - 0 */
		assert(new_game.squares >= 0);

		if(new_game.squares == 0) {
			puts("Game finished");
			break;
		}

		XNextEvent(dpy, &ev);

		switch(ev.type) {
		/* When the window is exposed, i.e. shown */
		case Expose: {
			/* query window attributes: width, height, etc... */
			XGetWindowAttributes(dpy, win, &attrs);
			/* draw the board */
			draw_board(dpy, win, gc, attrs.width, attrs.height);

			/* loop over all the squares */
			for(y = 0; y < NUMBER_OF_BOXES; y++) {
				for(x = 0; x < NUMBER_OF_BOXES; x++) {
					switch(new_game.tiles[y][x]) {
					case 0:
						break;
					/* is an X */
					case 1:
						draw_x(dpy, win, gc, 
							x*(attrs.width/3) 
							+ PADDING/2, 
							y*(attrs.height/3) 
							+ PADDING/2, 
							/* magic */
							attrs.width / (3.75), 
							attrs.height / (3.75));
						break;
					/* is an O */
					case 2:
						/* 
						 * i mean, circles are just
						 * polygons with high n, right?
						 */
						draw_polygon(dpy, win, gc, 
								x*(attrs.width/3)
								+ CIRCLE_RADIUS 
								+ PADDING/2, 
								y*(attrs.height/3)
								+ CIRCLE_RADIUS 
								+ PADDING/2, 
								CIRCLE_RADIUS, 
								NUMBER_OF_ANGLES);
						break;
					}
				}
			}
			break;
		}
		case ButtonPress: {
			XExposeEvent new_ev = {0};
			int filled_x, filled_y;
			int *cur_box;
			/* only register left click */
			if(ev.xbutton.button != 1) 
				break;

			filled_x = 3 * ev.xbutton.x / attrs.width;
			filled_y = 3 * ev.xbutton.y / attrs.height;

#if 0
			printf("Filled in box: %i %i\n", filled_x, filled_y);
#endif

			/* Set the selected box to filled */
			cur_box = &new_game.tiles[filled_y][filled_x];
			if(*cur_box == 0) {
				*cur_box = 1;
				new_game.squares--;
			} else {
				printf("Box already filled in\n");
			}

			ai_make_move(&new_game);
			/* 
			 * Force the expose event on our window, so that the 
			 * window is redrawn
			 */
			new_ev.type = Expose;
			new_ev.send_event = True;
			XSendEvent(dpy, win, 1, Expose, (XEvent *) &new_ev);
			break;
		}
			default:
				  break;
		}
	}
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}
