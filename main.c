#include <curses.h>
#include <menu.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include "shortcut.h"
#include "logo.h"
#include "common.h"
#include "config.h"

int main(int argc, char **argv)
{
	WINDOW *win;
	settings.shortcut_settings = NULL;
	settings.handlers = NULL;
	if( load_config() == -1 )
		return -1;

	win = initscr();
	start_color();
	cbreak();
	noecho();

	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
	
	/* Setup colors */
	setup_colors();
	/*
	init_pair(1, COLOR_GREEN, COLOR_BLUE);
	assume_default_colors(COLOR_WHITE, COLOR_BLUE);
	*/

	create_window_all();
	shortcut_list(settings.shortcut_settings);

	delwin(__win_list);
	delwin(__win_main);

	delwin(win);
	endwin();
	free_config();

	return 0;
}
