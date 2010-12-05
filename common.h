#ifndef __COMMON_H
#define __COMMON_H

#ifndef __INT_COMMON
extern WINDOW *__win_main;
extern WINDOW *__win_list;
extern WINDOW *__win_info;
#endif

#ifndef CTRL
#define CTRL(x) x & 037
#endif

typedef struct{
	char *exec;
	char *filter;
}HANDLER;

int setup_colors();
int create_window_all();
int run_program( char * program, char *arg);
int run_program_output( char * program, char *arg);
int run_file_handler( char *path );

#endif
