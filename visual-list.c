#include <sys/time.h>
#include <sys/types.h>
#include <ncursesw/ncurses.h>
#include <stdlib.h>
#include "list.h"
#include "visual-list.h"
#include "utf8.h"

#define TRUE	1
#define FALSE	0

#ifndef CTRL
#define CTRL(x) x & 037
#endif

typedef struct{
	int attributes;
	char *str;
	void *user_ptr;
}vl_entry;

struct __vl_list{
	dl_list *entries;
	dl_list *selected_entry;
	dl_list *first_visible_entry, *last_visible_entry;
	int n_entries;
};

vl_list *vl_list_add(vl_list *list, char *str, int attributes, void *user_ptr)
{
	vl_entry *ent;

	if( list == NULL ){
		list = malloc(sizeof(vl_list));
		list->entries = NULL;
		list->selected_entry = NULL;
		list->first_visible_entry = NULL;
		list->last_visible_entry = NULL;
		list->n_entries = 0;
	}

	ent = malloc(sizeof(vl_entry));

	ent->str = str;
	ent->attributes = attributes;
	ent->user_ptr = user_ptr;
	list->n_entries ++;

	list->entries = dl_list_add(list->entries, ent); 
	return list;
}

void vl_free_list( vl_list *list )
{
	dl_list *node;

	node = list->entries;
	while( node && node->prev )
		node = node->prev;
	
	while( node ){
		free(node->data);
		node = node->next;
	}

	dl_list_free( list->entries );
	free( list );
}

void vl_int_draw_list(WINDOW *win, 
	unsigned int width, dl_list *sel_entry, dl_list *start_entry,
	dl_list *stop_entry)
{
	vl_entry *ent;
	int line;
	int tmp_attr;
	int utf8_len;

	short std_color_pair;
	attr_t std_attr;

	wattr_get(win, &std_attr, &std_color_pair, NULL);
	line = 0;

	do{
		ent = (vl_entry *)start_entry->data;
		if( ent->attributes)
			tmp_attr = wattrset(win, ent->attributes);

		if(start_entry == sel_entry )
			wattron(win,A_STANDOUT);
		
		mvwaddstr(win, line, 0, ent->str);
		utf8_len = utf8_strlen(ent->str);
		mvwhline(win, line, utf8_len, ' ', width - utf8_len);

		if( ent->attributes)
			tmp_attr = wattrset(win, std_attr);
		if(start_entry == sel_entry )
			wattroff(win,A_STANDOUT);

		line ++;
		start_entry = start_entry->next;
	}while( start_entry != NULL && start_entry != stop_entry->next );
}


unsigned int vl_draw_list(WINDOW *win, vl_list *list, void **current_entry_user_ptr)
{
	dl_list *node;
	unsigned int max_y,max_x;
	int key, i;
	char update;

	update = TRUE;

	getmaxyx(win, max_y, max_x);

	if( list->first_visible_entry == NULL ){
		node = list->entries;
		while( node && node->prev )
			node = node->prev;

		list->first_visible_entry = node;

		i = 1;
		while( node && node->next && i < max_y ){
			node = node->next;
			i++;
		}

		list->last_visible_entry = node;
	}

	if( list->selected_entry == NULL )
		list->selected_entry = list->first_visible_entry;

#if 0
	if( current_user_entry ){
		selected_entry = current_user_entry;
		selected_entry_pos = 0;
		while( node ){
			if( node == current_user_entry )
				break;
			selected_entry_pos ++;
			node = node->next;
		}
		if( node == NULL )
			selected_entry = NULL;
	}
#endif


	while(1){
		if(update){
			vl_int_draw_list(win, max_x, list->selected_entry, list->first_visible_entry, list->last_visible_entry);
			update = FALSE;
			wrefresh(win);
		}
		key = wgetch(win);
		switch(key){
			case KEY_UP:
				if(list->selected_entry->prev != NULL){
					update = TRUE;
					if( list->selected_entry == list->first_visible_entry ){		/* Time to scroll (UPWARDS) */
						list->first_visible_entry = list->first_visible_entry->prev;
						list->last_visible_entry = list->last_visible_entry->prev;
						list->selected_entry = list->selected_entry->prev;
					}else{						/* No Scrolling needed */
						list->selected_entry = list->selected_entry->prev;
					}
				}
				break;
			case KEY_DOWN:
				if(list->selected_entry->next != NULL){
					update = TRUE;
					if( list->selected_entry == list->last_visible_entry ){	/* Time to scroll (DOWNWARDS) */
						list->first_visible_entry = list->first_visible_entry->next;
						list->last_visible_entry = list->last_visible_entry->next;
						list->selected_entry = list->selected_entry->next;
					}else{						/* No scrolling needed */
						list->selected_entry = list->selected_entry->next;
					}
				}
				break;
			case KEY_NPAGE: /* Scroll down by height/2 entries */
			case CTRL('d'):
				if( list->selected_entry->next != NULL){
					update = TRUE;
					for(i=0;i<(max_y/2);i++){
						if( list->selected_entry == list->last_visible_entry ){
							list->first_visible_entry = list->first_visible_entry->next;
							list->last_visible_entry = list->last_visible_entry->next;
							list->selected_entry = list->selected_entry->next;
						}else{
							list->selected_entry = list->selected_entry->next;
						}
						if( list->selected_entry->next == NULL )
							break;
					}
				}
			break;
			case KEY_PPAGE: /* Scroll up by height/2 entries */
			case CTRL('u'):
				if( list->selected_entry->prev != NULL ){
					update = TRUE;
					for(i=0;i<(max_y/2);i++){
						if( list->selected_entry == list->first_visible_entry ){
							list->first_visible_entry = list->first_visible_entry->prev;
							list->last_visible_entry = list->last_visible_entry->prev;
							list->selected_entry = list->selected_entry->prev;
						}else{
							list->selected_entry = list->selected_entry->prev;
						}
						if( list->selected_entry->prev == NULL )
							break;
					}
				}
			break;
			case KEY_HOME: /* Goto first entry */

				if( list->selected_entry->prev == NULL )
					break;

				update = TRUE;
				while( list->selected_entry->prev != NULL )
					list->selected_entry = list->selected_entry->prev;
				list->first_visible_entry = list->selected_entry;

				list->last_visible_entry = list->first_visible_entry;
				i = 1;
				while( list->last_visible_entry && list->last_visible_entry->next && i < max_y ){
					list->last_visible_entry = list->last_visible_entry->next;
					i++;
				}
			break;
			case KEY_END: /* Goto last entry */
				if( list->selected_entry->next == NULL )
					break;

				update = TRUE;
				while( list->selected_entry->next != NULL )
					list->selected_entry = list->selected_entry->next;

				while( list->last_visible_entry && list->last_visible_entry->next ){
					list->last_visible_entry = list->last_visible_entry->next;
					list->first_visible_entry = list->first_visible_entry->next;
				}
			break;
				/*case KEY_HOME:*/
#if 0
			case 'H':
			case 'h':
				if(selected_pos != 0){
					update = TRUE;
					if( min_y != 0 ){
						min_y = 0;
						start_entry = 0;
						stop_entry = (LINES-3);
						max_y = stop_entry;
					}else{
						y_pos = selected_pos;
						vl_int_draw_list(win,list,&y_pos,0,selected_pos,selected_pos);
						start_entry = 0;
						stop_entry = 0;
					}
					y_pos = 0;
					selected_pos = 0;
				}
				break;
				/*case KEY_END:*/
			case 'E':
			case 'e':
				if(selected_pos != last_entry){
					update = TRUE;
					if( max_y < last_entry ){	/* Need to update the whole list */
						y_pos = 0;
						start_entry = last_entry - (LINES-3);
						stop_entry = last_entry;
						min_y = start_entry;
						max_y = last_entry;
					}else{	/* Only update the current and last pos */
						y_pos = selected_pos - min_y;
						vl_int_draw_list(win,list,&y_pos,last_entry,selected_pos,selected_pos);
						start_entry = last_entry;
						stop_entry = last_entry;
						if(last_entry < (LINES - 3)){
							y_pos = last_entry;
						}else{
							y_pos = (LINES - 3);
						}
					}
					selected_pos = last_entry;
				}
				break;
#endif
			case KEY_ENTER:
			case 13:
			case 10:
				key = KEY_ENTER;
			default:
				*current_entry_user_ptr = ((vl_entry*)list->selected_entry->data)->user_ptr;
				return key;
				break;
		}
	}
	return 0;
}
