#ifndef __CONFIG_H
#define __CONFIG_H
#include "list.h"

typedef struct{
	dl_list *shortcut_settings;
	dl_list *handlers;

	/* some other settings ... */
	int color_bg;
	int attr_bg;

	int color_text;
	int attr_text;

	int color_dir;
	int attr_dir;

	int color_label;
	int attr_label;

	int color_frame;
	int attr_frame;
}SETTINGS;

typedef struct{
	char *name;
	int name_len;

	char *path;
	int path_len;

	char *filter;
	int filter_len;

	/* Common settings */
	char *exec;
	int exec_len;

	int type;
	char *type_str;
	int type_str_len;
}SHORTCUT_SETTINGS;

#define SHORTCUT_TYPE_BROWSER 	0
#define SHORTCUT_TYPE_EXEC	1
#define SHORTCUT_TYPE_LABEL	2
#define SHORTCUT_TYPE_EXEC_SHOW	3
#define SHORTCUT_TYPE_DIRECTORY_LIST	4
#define SHORTCUT_TYPE_DIRECTORY_ENTRY	5

#define COLOR_BACKGROUND	0
#define COLOR_TEXT		1
#define COLOR_DIRECTORY		2
#define COLOR_LABEL		3
#define COLOR_FRAME		4

#define COLOR_PAIR_TEXT		1
#define COLOR_PAIR_DIR		2
#define COLOR_PAIR_LABEL	3
#define COLOR_PAIR_FRAME	4


#ifndef __INT_CONFIG
extern SETTINGS settings;
#endif

int load_config();

#endif
