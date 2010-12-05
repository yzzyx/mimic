#include <curses.h>
#include <menu.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "visual-list.h"
#include "common.h"
#include "config.h"
#include "directory.h"
	
SHORTCUT_SETTINGS *dir_settings;
int dir_settings_n_entries;

vl_list *shortcut_create_vl_list(dl_list *shortcut_list)
{
	dl_list *node;
	dl_list *dir_list, *dir_list_node;
	SHORTCUT_SETTINGS *ss_ptr;
	__dir_entry *de;
	vl_list *visual_list;

	int i;

	visual_list = NULL;
	dir_list = NULL;

	/* Create menu */
	node = shortcut_list;
	while( node && node->prev )
		node = node->prev;

	while( node ){
		ss_ptr = node->data;
		if( ss_ptr->type == SHORTCUT_TYPE_LABEL )
			visual_list = vl_list_add(visual_list, ss_ptr->name, COLOR_PAIR(COLOR_PAIR_LABEL) | settings.attr_label, ss_ptr);
		else if( ss_ptr->type == SHORTCUT_TYPE_DIRECTORY_LIST ){
			/* Load list of directories */
			if( (dir_settings_n_entries = dir_get_list( &dir_list, ss_ptr->path, NULL, 0)) > 0 ){
				/* Turn directory-pointers into SHORTCUT_SETTINGS */
				dir_settings_n_entries = 0;
				dir_list_node = dir_list;
				while( dir_list_node ){
					de = dir_list_node->data;
					if( de->type & DT_DIR ) dir_settings_n_entries++;
					dir_list_node = dir_list_node->next;
				}

				/* No directories to list */
				if( dir_settings_n_entries == 0 ){
					if( dir_list ) dl_list_free_data(dir_list);
					dir_list = NULL;
				}else{
					dir_settings = realloc(dir_settings, dir_settings_n_entries * sizeof(SHORTCUT_SETTINGS));

					/* Add them to list */
					i = 0;
					dir_list_node = dir_list;
					while( dir_list_node ){
						de = dir_list->data;
						if( de->type & DT_DIR ){
							dir_settings[i].type = SHORTCUT_TYPE_BROWSER;
							dir_settings[i].path = malloc(strlen(de->name) + strlen(ss_ptr->path) + 2);
							sprintf(dir_settings[i].path, "%s/%s",
								ss_ptr->path, de->name);
							dir_settings[i].name = strdup(de->name);
							dir_settings[i].filter = NULL;

							visual_list = vl_list_add(visual_list, dir_settings[i].name,
									COLOR_PAIR(COLOR_PAIR_DIR) | settings.attr_dir, &dir_settings[i]);
							i++;
						}else{
							/* We're only interested in directories,
							 * so, we free this data here.
							 * The information about directories will
							 * be freed when shortcut_list returns
							 */
							free(de);
						}

						dir_list_node = dir_list_node->next;
					}
				}
			}
		}else
			visual_list = vl_list_add(visual_list, ss_ptr->name, COLOR_PAIR(COLOR_PAIR_TEXT) | settings.attr_text, ss_ptr);
		node = node->next;
	}

	if( dir_list ) dl_list_free(dir_list);

	return visual_list;
}

int shortcut_list(dl_list *list)
{
	vl_list *visual_list;
	SHORTCUT_SETTINGS *ss_ptr;
	WINDOW *list_win;

	int i, ch;
	int win_h, win_w;

	visual_list = NULL;
	dir_settings = NULL;
	dir_settings_n_entries = 0;

	visual_list = shortcut_create_vl_list( list );

	getmaxyx( __win_list, win_h, win_w );
	list_win = derwin(__win_list, win_h - 2, win_w - 2, 1, 1);
	keypad(list_win, TRUE);

	while( (ch = vl_draw_list(list_win, visual_list, (void **)&ss_ptr)) != 'q'){
		switch(ch){
			case KEY_ENTER:
				/* Enter directory ? */
				if( ss_ptr->type == SHORTCUT_TYPE_BROWSER ){
					werase(list_win);
					directory_list(ss_ptr);
					werase(list_win);
				/* Launch program ? */
				}else if( ss_ptr->type == SHORTCUT_TYPE_EXEC  ){	
					run_program(ss_ptr->exec, "");
					werase(list_win);
				}else if( ss_ptr->type == SHORTCUT_TYPE_EXEC_SHOW ){
					run_program_output(ss_ptr->exec, "");
					werase(list_win);
				}
				/* Redraw border */
				box(__win_list, 0, 0);
				wrefresh(__win_list);
			break;
			case CTRL('l'):
			case KEY_RESIZE:
				create_window_all();
				getmaxyx( __win_list, win_h, win_w );
				list_win = derwin(__win_list, win_h - 2, win_w - 2, 1, 1);
				keypad(list_win, TRUE);

				/* Recreate shortcut-list */

			break;

		}
		wrefresh(list_win);
		doupdate();
	}

	vl_free_list(visual_list);

	for(i=0;i<dir_settings_n_entries;i++){
		free(dir_settings[i].name);
		free(dir_settings[i].path);
	}

	if( dir_settings ) free(dir_settings);

	return 0;
}
