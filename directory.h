#ifndef __DIRECTORY_H
#define __DIRECTORY_H
#include "config.h"

typedef struct{
	int type;
	char *name;
}__dir_entry;

int dir_check_filter( char *path, char *filter);
int dir_get_list(dl_list **list, char *path, char *filter, int include_parent);
int directory_list(SHORTCUT_SETTINGS *settings);

#endif
