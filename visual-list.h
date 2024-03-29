#ifndef __VISUAL_LIST_H
#define __VISUAL_LIST_H
#include <ncursesw/ncurses.h>
#include "list.h"

typedef struct __vl_list vl_list;

void vl_set_selected(vl_list *list, int count);
int vl_get_selected(vl_list *list);
unsigned int vl_draw_list(WINDOW *win, vl_list *list, void  **current_entry);
vl_list *vl_list_add(vl_list *list, char *str, int color_pair, void *user_ptr);
void vl_free_list(vl_list *list);

#endif
