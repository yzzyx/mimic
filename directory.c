#include <ncursesw/ncurses.h>
#include <menu.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "list.h"
#include "visual-list.h"
#include "common.h"
#include "config.h"

typedef struct{
	int type;
	char *name;
}__dir_entry;

/*
 * int
 * dir_check_filter( file, filter )
 *
 * Check if file is among the allowed types in filter
 *
 */
int dir_check_filter(char *file, char *filter)
{
	char *ext;
	char *next;

	/* Empty filter always matches */
	if( filter == NULL || strlen(filter) == 0 )
		return 1;

	/* Otherwise, fileextensions are listed separated by ; */

	ext = strrchr(file, '.');
	if( ext == NULL ) /* No extension, do not list */
		return 0;

	do{
		next = strchr(filter, ';');
		if( next == NULL ){
			if( strcasecmp(ext, filter) == 0 )
				return 1;
		}else{
			if( strlen(ext) == next - filter && 
			    strncmp(ext, filter, next - filter) == 0 )
				return 1;
			next ++;
		}

		filter = next;
	}while(next);

	return 0;
}


int dir_get_list(dl_list **list, char *path, char *filter, int include_parent)
{
	DIR *dir;
	struct dirent *dent;
	int n_entries;
	__dir_entry *dir_entry;

	*list = NULL;
	n_entries = 0;

	dir = opendir(path);
	if( dir == NULL )
		return -1;

	while( (dent = readdir(dir)) != NULL ){
		if( (dent->d_name[0] == '.' && dent->d_name[1] != '.' ) || 
			(!(dent->d_type & DT_DIR) &&
			dir_check_filter(dent->d_name, filter) != 1 ) )
			continue;

		if( include_parent == 0 &&
			dent->d_type & DT_DIR &&
			strlen(dent->d_name) == 2 && 
			dent->d_name[0] == '.' && dent->d_name[1] == '.' &&
			dent->d_name[2] == '\0' )
			continue;

		if( (dir_entry = malloc(sizeof(__dir_entry))) == NULL ){
			perror("malloc()");
			return -1;
		}

		dir_entry->type = dent->d_type;

		if( (dir_entry->name = strdup(dent->d_name)) == NULL ){
			perror("malloc()");
			return -1;
		}

		*list = dl_list_add(*list, dir_entry);
		n_entries ++;
	}
	closedir(dir);

	return n_entries;
}

int dir_sort( void *ptr1, void *ptr2 )
{
	__dir_entry *d1, *d2;

	d1 = (__dir_entry *)ptr1;
	d2 = (__dir_entry *)ptr2;

	if( d1->type & DT_DIR &&
		!(d2->type  & DT_DIR) ){
		return -1;
	}
	if( d2->type & DT_DIR &&
		!(d1->type  & DT_DIR) ){
		return 1;
	}	

	return strcasecmp(d1->name, d2->name);
}

int directory_create_visual_list(vl_list **ret_list, dl_list *list, const char const*filter)
{
	dl_list *node;
	vl_list *visual_list;
	__dir_entry *de;
	char *filter_uppercase, *name_cpy, *ptr;
	int n_entries;

	visual_list = NULL;
	n_entries = 0; node = list;

	if( filter ){
		filter_uppercase = strdup(filter);
		if( filter_uppercase == NULL ) return 0;
		for(ptr=filter_uppercase;*ptr != '\0';ptr++) *ptr = toupper(*ptr);
	}

	while( node && node->prev )
		node = node->prev;

	while( node ){
		de = node->data;

		if( filter && !( strlen(de->name) == 2 && 
				de->name[0] == '.' &&
				de->name[1] == '.' &&
				de->name[2] == '\0' ) ){
			/* Create an uppercase copy of the name */
			if( (name_cpy = strdup(de->name)) == NULL )
				return 0;

			for(ptr=name_cpy;*ptr != '\0';ptr++) *ptr = toupper(*ptr);
		
			if( ! strstr(name_cpy, filter_uppercase) ){
				node = node->next;
				free(name_cpy);
				continue;
			}
			free(name_cpy);
		}

		if( de->type & DT_DIR ){
			visual_list = vl_list_add(visual_list, de->name, COLOR_PAIR(COLOR_PAIR_DIR) | settings.attr_dir, de);
		}else
			visual_list = vl_list_add(visual_list, de->name, COLOR_PAIR(COLOR_PAIR_TEXT) | settings.attr_text, de);
		node = node->next;
		n_entries ++;
	}

	if( filter ) free(filter_uppercase);

	*ret_list = visual_list;

	return n_entries;
}


int directory_list(SHORTCUT_SETTINGS *dir_settings)
{
	dl_list *list, *node;
	vl_list *visual_list;
	__dir_entry *de;
	int exit_code;
	int n_entries, prev_n_entries;
	char *path, *ptr;
	char *filename_path;
	char *current_filter;
	int filter_len, max_filter_len;
	WINDOW *path_win, *list_win;

	int i, ch;
	int win_h, win_w;

	exit_code = 0;

	/* Create sub-windows */
	getmaxyx(__win_list, win_h, win_w);
	path_win = derwin(__win_list, 1, win_w - 2, 0, 1);
	list_win = derwin(__win_list, win_h - 2, win_w - 2, 1, 1);
	keypad(list_win, TRUE);

	/* Allocate memory for filter */
	max_filter_len = 120;
	if( (current_filter = malloc(max_filter_len * sizeof(char))) == NULL ){
		dl_list_free(list);
		delwin(path_win);
		delwin(list_win);
		return -1;
	}

	filter_len = 0;

	path = NULL;
	n_entries = dir_get_list(&list, dir_settings->path, dir_settings->filter, 1);

	if (n_entries == -1)
		return -1;

	visual_list = NULL;

	/* Order list */
	list = dl_list_qsort(list, dir_sort);

	/* Create menu */
	prev_n_entries = directory_create_visual_list( &visual_list, list, NULL);

	/* print path (from base_path) */
	wmove(path_win, 0, 0);
	whline(path_win, ACS_HLINE, win_w - 2);
	mvwprintw(path_win, 0, 0, "%s/", dir_settings->name);
	wrefresh(path_win);
	wclear(list_win);

	while( (ch = vl_draw_list(list_win, visual_list, (void **)&de)) != 'q'){
		switch(ch){
			case KEY_ENTER:
				/* Enter directory ? */
				if( de->type & DT_DIR ){

					/* Clear filter */
					filter_len = 0;
					current_filter[0] = 0x00;

					if ( de->name[0] == '.' && de->name[1] == '.' && de->name[2] == '\0' ){

						/* We've reached the top node */
						if( !path || !strlen(path) ){
							ch = 'q';
							break;
						}

						/* Step up one directory */
						if( path[strlen(path)-1] == '/' && path[1] != '\0' )
							path[strlen(path)-1] = '\0';

						ptr = strrchr(path, '/' );
						if( ptr != NULL ){
							if( ptr == path )
								path[1] = '\0';
							else
								*ptr = '\0';
						}else{
							path[0] = '\0';
						}
					}else{
						if( path == NULL )
							path = strdup(de->name);
						else{
							path = realloc(path, strlen(path) + strlen(de->name) + 2);
							if( strlen(path) )
								sprintf(path+strlen(path), "/%s", de->name);
							else
								strcpy(path, de->name);
						}
					}

					wmove(path_win, 0, 0);
					whline(path_win, ACS_HLINE, win_w - 2);
					mvwprintw(path_win, 0, 0, "%s/%s", dir_settings->name, path);
					wrefresh(path_win);

					vl_free_list(visual_list);

					visual_list = NULL;

					node = list;
					/* Free node data */
					while( node && node->prev)
						node = node->prev;
					while( node && node->next ){
						free(((__dir_entry*)node->data)->name);
						free(node->data);
						node = node->next;
					}
					/* Free last entry */
					if( node ){
						free(((__dir_entry*)node->data)->name);
						free(node->data);
					}

					dl_list_free(list);

					list = NULL;

					ptr = malloc(strlen(path) + strlen(dir_settings->path) + 2);
					/* FIXME! check return */ 
					sprintf(ptr, "%s/%s", dir_settings->path, path );

					n_entries = dir_get_list(&list, ptr, dir_settings->filter, 1);

					if (n_entries == -1) {
						exit_code = -1;
						goto end_function;
					}

					free(ptr);
					list = dl_list_qsort(list, dir_sort);

					n_entries = directory_create_visual_list( &visual_list, list, NULL);

					if( n_entries < prev_n_entries ){
						for(i=n_entries;i<prev_n_entries;i++)
							mvwhline(list_win, i, 0, ' ', win_w - 2);
					}
					prev_n_entries = n_entries;
				}else{	/* Launch program ? */
					if( path == NULL ){

						if( (filename_path = malloc( strlen(dir_settings->path) + strlen(de->name) + 3)) == NULL ){
							perror("malloc()");
							return -1;
						}

						sprintf(filename_path, "%s/%s", dir_settings->path, de->name);
					}else{
						if( (filename_path = malloc( strlen(dir_settings->path) + strlen(path) + strlen(de->name) + 3)) == NULL ){
							perror("malloc()");
							return -1;
						}

						if( path[strlen(path)-1] == '/' )
							sprintf(filename_path, "%s/%s%s", dir_settings->path, path, de->name);
						else
							sprintf(filename_path, "%s/%s/%s", dir_settings->path, path, de->name);
					}

					run_file_handler(filename_path);
					free(filename_path);

					/* Redraw window */
					delwin(path_win);
					delwin(list_win);

					create_window_all();
					getmaxyx(__win_list, win_h, win_w);
					path_win = derwin(__win_list, 1, win_w - 2, 0, 1);
					list_win = derwin(__win_list, win_h - 2, win_w - 2, 1, 1);
					keypad(list_win, TRUE);

					if( path != NULL )
						mvwprintw(path_win, 0, 0, "%s/%s", dir_settings->name, path);
					else
						mvwprintw(path_win, 0, 0, "%s/", dir_settings->name );
					wrefresh(path_win);
				}
			break;
			case CTRL('l'):
			case KEY_RESIZE:

				delwin(path_win);
				delwin(list_win);

				create_window_all();
				getmaxyx(__win_list, win_h, win_w);
				path_win = derwin(__win_list, 1, win_w - 2, 0, 1);
				list_win = derwin(__win_list, win_h - 2, win_w - 2, 1, 1);
				keypad(list_win, TRUE);

				mvwprintw(path_win, 0, 0, "%s", path);
				wrefresh(path_win);
			break;
			default: /* Add to list filter */

				if( ch == (CTRL('u')) ){ /* Clear filter */
					filter_len = 0;
				}else if( ch == KEY_BACKSPACE ) {
					if( filter_len > 0 ) filter_len --;
				}else if( strlen(current_filter) < max_filter_len )
					current_filter[filter_len++] = ch; 
				current_filter[filter_len] = 0x00;

				/* Create menu with filter applied */
				vl_free_list(visual_list);
				n_entries = directory_create_visual_list(&visual_list, list, current_filter);

				/* Clear differing rows */
				if( n_entries < prev_n_entries ){
					for(i=n_entries;i<prev_n_entries;i++)
						mvwhline(list_win, i, 0, ' ', win_w - 2);
				}
				prev_n_entries = n_entries;
		}



		mvwhline(__win_list, win_h -1, 1, ACS_HLINE, win_w - 2);
		if( filter_len ){
			mvwprintw( __win_list , win_h - 1, 1, current_filter);
		}
		wrefresh(__win_list);

		wrefresh(list_win);
		doupdate();

		if( ch == 'q' )
			break;
	}

end_function:
	free(current_filter);

	if (visual_list)
		vl_free_list(visual_list);

	node = list;
	/* Free node data */
	while( node && node->prev)
		node = node->prev;
	while( node && node->next ){
		free(((__dir_entry*)node->data)->name);
		free(node->data);
		node = node->next;
	}
	/* Free last entry */
	if( node ){
		free(((__dir_entry*)node->data)->name);
		free(node->data);
	}

	if (list)
		dl_list_free(list);

	free(path);

	return exit_code;
}
