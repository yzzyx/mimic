#include <ncursesw/ncurses.h>

static char *logo[] = {
	"    ___                ____  ",
	"   /   \\  O        O  /  __| ",
	"  /     \\ _  /\\/\\  _ |  |    ",
	" /       \\ ||    || ||  |__  ",
	"/__/\\_/\\__\\||_||_||_| \\____| ",
	0
};

WINDOW *logo_create_win( WINDOW *win )
{
	WINDOW *logo_win;
	int i;

	logo_win = derwin( win, 6, 30, 1, 1);

	for(i=0;logo[i] != 0;i++){
		mvwprintw(logo_win, i, 0, "%s", logo[i]);
	}

	wnoutrefresh(logo_win);
	return logo_win;
}
