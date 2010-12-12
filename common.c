#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <curses.h>
#include "config.h"
#include "directory.h"

#define __INT_COMMON 1
#include "common.h"

WINDOW *__win_list = NULL;
WINDOW *__win_main = NULL;

int setup_colors()
{
	short color_bg;
/*
//	init_color(COLOR_BACKGROUND, settings.color_background.r, settings.color_background.g, settings.color_background.b);
	init_color(COLOR_TEXT, settings.color_text.r, settings.color_text.g, settings.color_text.b);
	init_color(COLOR_DIRECTORY, settings.color_directory.r, settings.color_directory.g, settings.color_directory.b);
	init_color(COLOR_LABEL, settings.color_label.r, settings.color_label.g, settings.color_label.b);
	init_color(COLOR_FRAME, settings.color_frame.r, settings.color_frame.g, settings.color_frame.b);
	*/

	if( settings.color_bg == -1 )
		color_bg = COLOR_BLACK;
	else
		color_bg = settings.color_bg;
	
	init_pair(COLOR_PAIR_TEXT, (settings.color_text==-1) ? COLOR_WHITE : settings.color_text, color_bg);
	init_pair(COLOR_PAIR_DIR, (settings.color_dir==-1) ? COLOR_WHITE : settings.color_dir, color_bg);
	init_pair(COLOR_PAIR_LABEL, (settings.color_label==-1) ? COLOR_WHITE : settings.color_label, color_bg);
	init_pair(COLOR_PAIR_FRAME, (settings.color_frame==-1) ? COLOR_WHITE : settings.color_frame, color_bg);
	
	assume_default_colors((settings.color_text==-1) ? COLOR_WHITE : settings.color_text, color_bg);

	return 0;
}

int create_window_all()
{
	int width_list;
	int max_y, max_x;

	if( __win_list ) delwin(__win_list);
	if( __win_main ) delwin(__win_main);

	__win_main = newwin(0,0,0,0);

	getmaxyx(__win_main, max_y, max_x);

	width_list = max_x;
	__win_list = derwin(__win_main, 0, width_list, 0, 0);

	/* draw borders */
	box(__win_list, 0, 0);

	/* add "logo" */
	mvwprintw(__win_main, 0, max_x - 8, "mimic");

	wrefresh(__win_list);
	wrefresh(__win_main);

	keypad(__win_list, TRUE);

	return 0;
}

int build_arg_list( char *program, char *arg, char ***arg_list_ptr)
{
	char *exec_name;
	char *ptr;
	char **arg_list;
	char *curr_arg;
	int n_args;

	int in_double, in_singel, add_arg;
	int max_arg_len = 10;
	int arg_len = 0;

	arg_list = malloc(sizeof(char*));

	ptr = strchr(program, ' ');
	if( ptr == NULL )
		exec_name = strdup(program);
	else{
		exec_name = malloc( ptr - program + 2);
		snprintf(exec_name, ptr - program + 1, "%s", program);
		exec_name[ptr-program + 1] = '\0';
		ptr ++;
	}
	arg_list[0] = exec_name;

	in_double = 0;
	in_singel = 0;
	add_arg = 0;

	arg_len = 0;
	curr_arg = malloc(max_arg_len);

	n_args = 1;
	while( ptr && *ptr ){
		switch( *ptr ){
			case '"':
				if( in_double ){
					in_double = 0;
					ptr ++;
					continue;
				}else if( ! in_singel ){
					in_double = 1;
					ptr ++;
					continue;
				}
			break;
			case '\'':
				if( in_singel ){
					in_singel = 0;
					ptr ++;
					continue;
				}else if( ! in_double ){
					in_singel = 1;
					ptr ++;
					continue;
				}
			break;
			case ' ':
				if( ! in_singel && ! in_double )
					add_arg = 1;
			break;
		}

		if( add_arg ){
			arg_list = realloc(arg_list, (n_args+1) * sizeof(char*));
			arg_list[n_args] = strdup(curr_arg);
			n_args ++;
			add_arg = 0;
			arg_len = 0;
			curr_arg[0] = '\0';
		}else if (*ptr == '%'){
			if( arg_len + strlen(arg) > max_arg_len - 1 ){
				curr_arg = realloc(curr_arg, max_arg_len + strlen(arg));
				max_arg_len += strlen(arg);
			}

			sprintf(curr_arg + arg_len, "%s", arg);
			arg_len += strlen(curr_arg);
			curr_arg[arg_len] = '\0';
		}else{
			if( arg_len == max_arg_len - 1 ){
				curr_arg = realloc(curr_arg, max_arg_len + 10);
				max_arg_len += 10;
			}
			curr_arg[arg_len++] = *ptr;
			curr_arg[arg_len] = '\0';
		}

		ptr ++;
	}

	if( arg_len ){
		arg_list = realloc(arg_list, (n_args+1) * sizeof(char*));
		arg_list[n_args] = strdup(curr_arg);
		n_args ++;
		add_arg = 0;
		arg_len = 0;
		curr_arg[0] = '\0';
	}
	arg_list = realloc(arg_list, (n_args+1) * sizeof(char*));
	arg_list[n_args] = NULL;

	*arg_list_ptr = arg_list;
	return n_args;
}

int run_program( char *program, char *arg)
{
	pid_t pid;
	int status;
	char **arg_list = NULL;
	int n_args;

	int i;

	n_args = build_arg_list(program, arg, &arg_list);
	pid = fork();

	if( pid == 0 ){ /* child */
	//	close(1);
	//	close(2);
		if( execvp( arg_list[0], arg_list ) == -1 ){
			perror("execvp");
			exit(-errno);
		}
	}else{
		waitpid( pid, &status, 0);
	}

	for(i=0;i<n_args;i++)
		free(arg_list[i]);
	free(arg_list);

	return 0;
}

int run_program_output( char *program, char *arg)
{
	pid_t pid;
	int status;
	char **arg_list = NULL;
	int n_args;
	int pipe_fd[2];
	WINDOW *win;
	char **output;

	int output_width;
	int output_pos;
	int output_n_lines;
	int max_width, max_height;

	char ch[1];
	int i;

	n_args = build_arg_list(program, arg, &arg_list);

	/* Create pipe for communication */
	if( pipe(pipe_fd) == -1 ){
		for(i=0;i<n_args;i++)
			free(arg_list[i]);
		free(arg_list);
		return -1;
	}

	pid = fork();

	if( pid == 0 ){ /* child */
		close( pipe_fd[0] );

		if(dup2( pipe_fd[1], 1 ) == -1 ){ /* make pipe -> stdout */
			perror("dup2()");
			exit(-errno);
		}
	//	close(2);
		if( execvp( arg_list[0], arg_list ) == -1 ){
			perror("execvp");
			exit(-errno);
		}
	}

	/* We have no use for these anymore */
	for(i=0;i<n_args;i++)
		free(arg_list[i]);
	free(arg_list);

	close(pipe_fd[1]); /* We don't need to write to the pipe */


	output_width = 0;
	output_n_lines = 1;
	output_pos = 0;

	getmaxyx(__win_main, max_height, max_width );
	max_width -= 4; /* Space for borders */
	max_height -= 4;

	if( (output = malloc(output_n_lines * sizeof(char *))) == NULL ){
		return -1;
	}
	if( (output[output_n_lines-1] = malloc(max_width*sizeof(char))) == NULL ){
		free(output);
		return -1;
	}

	while( read(pipe_fd[0], ch, 1) ){
		if( *ch == '\n'  || *ch == '\r' ){
			if( output_pos > output_width )
				output_width = output_pos;

			output[output_n_lines-1][output_pos] = '\0';

			output_n_lines ++;

			if ( output_n_lines ==  max_height - 1 ){
				close(pipe_fd[0]);
				break;
			}

			if( (output = realloc(output, output_n_lines * sizeof(char *))) == NULL ){
				return -1;
			}

			if( (output[output_n_lines-1] = malloc(max_width*sizeof(char))) == NULL ){
				free(output);
				return -1;
			}

			output_pos = 0;
			continue;
		}
		
		if( *ch == '\t' ) *ch = ' ';

		if( output_pos < max_width )
			output[output_n_lines-1][output_pos++] = *ch;
	}

	output[output_n_lines-1][output_pos] = '\0';
	if( output_pos > output_width )
		output_width = output_pos;

	output_n_lines ++;
	if( (output = realloc(output, output_n_lines * sizeof(char *))) == NULL ){
		return -1;
	}

	if( (output[output_n_lines-1] = malloc(max_width*sizeof(char))) == NULL ){
		free(output);
		return -1;
	}

	if( output_width < 16 )
		output_width = 16; 

	snprintf(output[output_n_lines-1], max_width - 1, "%*s", output_width / 2 + 8, "[Press any key]");

	waitpid( pid, &status, 0);

	win = derwin(__win_main, output_n_lines + 2, output_width + 3, max_height / 2 - output_n_lines / 2, max_width / 2 - output_width / 2);
	werase(win);
	box(win, 0, 0);

	for(i=0;i<output_n_lines;i++){
		mvwprintw(win, i+1, 1, "%s", output[i]);
		/*
		for(j=0;j<16,output[i][j]!=0x00;j++){
			mvwprintw(win, i+1, j*3, "%.2x ", output[i][j]);
		}
		*/
		free(output[i]);
	}
	free(output);

	wgetch(win);
	delwin(win);

	return 0;
}

/*
 * run_file_handler ( path )
 *
 * Finds the appropriate handler for the filetype of 'path',
 * and executes it, with 'path' as argument.
 */
int run_file_handler( char *path )
{
	dl_list *node;
	HANDLER *handler;
	int retval;

	node = settings.handlers;
	if( node == NULL ){ /* no handlers defined */
		return 0;
	}

	while( node->prev ) node = node->prev;

	while( node ){
		handler = node->data;
		if( dir_check_filter(path, handler->filter) ){
			if( handler->exec[0] == '!' ) /* Show result in window */
				retval = run_program_output(handler->exec+1, path);
			else
				retval = run_program(handler->exec, path);
			return retval;
		}
		node = node->next;
	}

	/* No appropriate handler found */
	/* FIXME! Write a message to the user that no
	   handler was found! */
	return 0;
}
