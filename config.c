#include <stdio.h>
#include <expat.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include "list.h"
#include "common.h"
#define __INT_CONFIG	1
#include "config.h"

#define BUFSIZE 4096

#define ELEMENT_MIMIC        0

#define ELEMENT_SHORTCUTS    1
#define ELEMENT_SHORTCUT     2
#define ELEMENT_NAME         3
#define ELEMENT_PATH         4
#define ELEMENT_TYPE         5
#define ELEMENT_FILTER       6
#define ELEMENT_EXEC         7
#define ELEMENT_HANDLER      8

#define ELEMENT_SETTINGS     20
#define ELEMENT_COLORS	     21
#define ELEMENT_COLOR_BACKGROUND     22
#define ELEMENT_COLOR_TEXT	     23
#define ELEMENT_COLOR_DIRECTORY	     24
#define ELEMENT_COLOR_LABEL	     25
#define ELEMENT_COLOR_FRAME	     26

static int depth = 0;
static int c_element = -1;
static int sub_element = -1;
SETTINGS settings;

struct string_id_pair{
	char *str;
	int id;
};

struct string_id_pair color_list[] =
{	{ "black", COLOR_BLACK },
	{ "red", COLOR_RED },
	{ "green", COLOR_GREEN },
	{ "yellow", COLOR_YELLOW },
	{ "blue", COLOR_BLUE },
	{ "magenta", COLOR_MAGENTA },
	{ "cyan", COLOR_CYAN },
	{ "white", COLOR_WHITE },
	{ NULL, 0 }
};

struct string_id_pair attr_list[] =
{	{ "normal", A_NORMAL },
	{ "standout", A_STANDOUT },
	{ "underline", A_UNDERLINE },
	{ "reverse", A_REVERSE },
	{ "blink", A_BLINK },
	{ "dim", A_DIM },
	{ "bold", A_BOLD },
	{ NULL, 0 }
};


static void str_trim( char *str )
{
	char *ptr;

	if( ! str )
		return;

	ptr = str + strlen(str) - 1;
	while( isspace(*ptr) && ptr > str )
		ptr --;
	*(ptr+1) = '\0';
}

static void XMLCALL ch_data_handler( void *user_data, const XML_Char *data, int len)
{
	SHORTCUT_SETTINGS *ss_ptr;
	char **str_ptr;
	int *str_len_ptr;

	str_ptr = NULL;
	str_len_ptr = NULL;
	if( c_element != ELEMENT_SHORTCUT )
		return;

	ss_ptr = (((SETTINGS *)user_data)->shortcut_settings)->data;
	if( sub_element == ELEMENT_NAME ){
		str_ptr = &ss_ptr->name;
		str_len_ptr = &ss_ptr->name_len;
	}else if( sub_element == ELEMENT_PATH ){
		str_ptr = &ss_ptr->path;
		str_len_ptr = &ss_ptr->path_len;
	}else if( sub_element == ELEMENT_EXEC ){
		str_ptr = &ss_ptr->exec;
		str_len_ptr = &ss_ptr->exec_len;
	}else if( sub_element == ELEMENT_FILTER ){
		str_ptr = &ss_ptr->filter;
		str_len_ptr = &ss_ptr->filter_len;
	}else if( sub_element == ELEMENT_TYPE ){
		str_ptr = &ss_ptr->type_str;
		str_len_ptr = &ss_ptr->type_str_len;
	}

	if( str_ptr ){
		*str_ptr = realloc(*str_ptr, *str_len_ptr + len + 1); 
		if( *str_ptr == NULL ){
			perror("realloc()");
			return;
		}
		strncpy(*str_ptr + *str_len_ptr, data, len);
		*str_len_ptr += len;
		(*str_ptr)[*str_len_ptr] = '\0';
	}
}

static void XMLCALL start(void *user_data, const XML_Char *el, const XML_Char **attr)
{
	SHORTCUT_SETTINGS *ss_ptr;
	int t_color, t_attr;
	int i,j;
	char *ptr, *end_ptr;

	if( depth == 0 && strcmp(el, "mimic") == 0 )
		c_element = ELEMENT_MIMIC;
	else if( strcmp(el, "shortcuts") == 0 ){
		c_element = ELEMENT_SHORTCUTS;
		depth ++;
		return;
	}else if( strcmp(el, "settings") == 0 ){
		c_element = ELEMENT_SETTINGS;
		depth ++;
		return;
	}

	/* Add a file-handler */
	if( ( c_element == ELEMENT_SETTINGS || c_element == ELEMENT_COLORS ) && strcmp(el, "handler") == 0 ){
		char *exe, *filter;
		HANDLER *handler_ptr;

		exe = filter = NULL;
		for(i=0;attr[i] != NULL;i+=2){
			if( strcmp(attr[i], "exec") == 0 ){
				exe = strdup(attr[i+1]);	
			}else if( strcmp(attr[i], "filter") == 0 ){
				filter = strdup(attr[i+1]);
			}
		}

		/* If no executable is specified, we won't open */
		if( exe == NULL ){
			if( filter ) free(filter);
		}else{
			if( (handler_ptr = malloc(sizeof(HANDLER))) == NULL ){
				free(exe);
				if( filter ) free(filter);
				return;
			}
			handler_ptr->exec = exe;
			handler_ptr->filter = filter;
			((SETTINGS*)user_data)->handlers = dl_list_add(((SETTINGS *)user_data)->handlers, handler_ptr);
		}
		depth ++;
		return;
	}

	if( c_element == ELEMENT_SETTINGS && strcmp(el, "colors") == 0 ){
		c_element = ELEMENT_COLORS;	
		depth ++;
		return;
	}

	
	if( c_element == ELEMENT_COLORS ){
		t_color = -1; t_attr = 0;
		for(i=0;attr[i] != NULL;i+=2){
			if( strcmp(attr[i], "color") == 0 ){
				for(j=0;color_list[j].str;j++){
					if( strcmp(attr[i+1], color_list[j].str) == 0 )
						t_color = color_list[j].id;
				}
			}else if( strcmp(attr[i], "attribute") == 0 ){
				ptr = attr[i+1];
				do{
					if( *ptr == ',' ) ptr ++;
					end_ptr = strchr(ptr, ',');
					for(j=0;attr_list[j].str;j++){
						if( (end_ptr != NULL && strncmp(ptr, attr_list[j].str, end_ptr-ptr)) || 
							(end_ptr == NULL && strcmp(ptr, attr_list[j].str) == 0 ) )
							t_attr |= attr_list[j].id;
					}
				}while( (ptr = strchr(ptr, ',')) != NULL );
			}
		}

		if( strcmp(el, "background") == 0 ){ settings.color_bg = t_color; settings.attr_bg = t_attr; }
		else if( strcmp(el, "text") == 0 ){ settings.color_text = t_color; settings.attr_text = t_attr; }
		else if( strcmp(el, "directory") == 0 ){ settings.color_dir = t_color; settings.attr_dir = t_attr; }
		else if( strcmp(el, "label") == 0 ){ settings.color_label = t_color; settings.attr_label = t_attr; }
		else if( strcmp(el, "frame") == 0 ){ settings.color_frame = t_color; settings.attr_frame = t_attr; }
		return;
	}else if( c_element == ELEMENT_SHORTCUTS || c_element == ELEMENT_SHORTCUT ){
		if( strcmp(el, "shortcut") == 0 ){
			c_element = ELEMENT_SHORTCUT;

			if( (ss_ptr = calloc(1, sizeof(SHORTCUT_SETTINGS))) == NULL ){
				perror("malloc()");
				depth ++;
				return;
			}
			
			((SETTINGS*)user_data)->shortcut_settings = dl_list_add(((SETTINGS *)user_data)->shortcut_settings, ss_ptr);
		}
		depth ++;
	}
	
	if( c_element == ELEMENT_SHORTCUT ){
		if( strcmp(el, "name") == 0 )
			sub_element = ELEMENT_NAME;
		else if( strcmp(el, "path") == 0 )
			sub_element = ELEMENT_PATH;
		else if( strcmp(el, "type") == 0 )
			sub_element = ELEMENT_TYPE;
		else if( strcmp(el, "filter") == 0 )
			sub_element = ELEMENT_FILTER;
		else if( strcmp(el, "exec") == 0 )
			sub_element = ELEMENT_EXEC;
	}

	depth ++;
}

static void XMLCALL end(void *user_data, const char *el)
{
	SHORTCUT_SETTINGS *ss_ptr;

	depth --;

	if( sub_element == ELEMENT_TYPE ){
		ss_ptr = (((SETTINGS *)user_data)->shortcut_settings)->data;
		str_trim(ss_ptr->type_str);

		if( ss_ptr->type_str && strcasecmp(ss_ptr->type_str, "browser") == 0){
			ss_ptr->type = SHORTCUT_TYPE_BROWSER;
		}else if( ss_ptr->type_str && strcasecmp(ss_ptr->type_str, "exec") == 0){
			ss_ptr->type = SHORTCUT_TYPE_EXEC;
		}else if( ss_ptr->type_str && strcasecmp(ss_ptr->type_str, "exec_show") == 0){
			ss_ptr->type = SHORTCUT_TYPE_EXEC_SHOW;
		}else if( ss_ptr->type_str && strcasecmp(ss_ptr->type_str, "automount") == 0){
			ss_ptr->type = SHORTCUT_TYPE_DIRECTORY_LIST;
		}else{	/* dummy label */
			ss_ptr->type = SHORTCUT_TYPE_LABEL;
		}
	}else if( sub_element == ELEMENT_NAME ){
		ss_ptr = (((SETTINGS *)user_data)->shortcut_settings)->data;
		str_trim(ss_ptr->name);
	}else if( sub_element == ELEMENT_FILTER ){
		ss_ptr = (((SETTINGS *)user_data)->shortcut_settings)->data;
		str_trim(ss_ptr->filter);
	}else if( sub_element == ELEMENT_EXEC ){
		ss_ptr = (((SETTINGS *)user_data)->shortcut_settings)->data;
		str_trim(ss_ptr->exec);
	}else if( sub_element == ELEMENT_PATH ){
		ss_ptr = (((SETTINGS *)user_data)->shortcut_settings)->data;
		str_trim(ss_ptr->path);
	}

	sub_element = -1;
}

int load_config()
{
	XML_Parser p;
	FILE *fp;
	char *buffer;
	char *config_file, *home;
	int nb;

	/* Try the following configuration-files: 
	 * ~/.mimic.xml
	 * /etc/mimic.xml
	 */

	fp = NULL; config_file = NULL;
	home = getenv("HOME");
	if( home ){
		if( (config_file = malloc(strlen(home) + 15)) == NULL){
			perror("malloc()");
			return -1;
		}
		sprintf(config_file, "%s/.mimic.xml", home);

		fp = fopen(config_file, "r"); 
	}

	/* Try with /etc/mimic.xml */
	if( !fp ){
		if( config_file )
			free(config_file);
		if( (config_file = strdup("/etc/mimic.xml")) == NULL ){
			perror("strdup()");
			return -1;
		}

		fp = fopen(config_file, "r");
	}

	/* Print error, and exit */
	if( !fp ){
		free(config_file);

		printf("No usable configuration file found! (Please create ~/.mimic.xml)\n");

		return -1;
	}

	p = XML_ParserCreate(NULL);
	if( ! p ) {
		fprintf(stderr, "XML_ParserCreate failed (not enough memory?)\n");
		free(config_file); fclose(fp);
		return -1;
	}

	if( (buffer = XML_GetBuffer(p, BUFSIZE)) == NULL ){
		perror("XML_GetBuffer()");
		free(config_file); fclose(fp);
		return -1;
	}

	XML_SetElementHandler(p, start, end);
	XML_SetCharacterDataHandler(p, ch_data_handler);
	XML_SetUserData(p, &settings);
	
	while( !feof(fp) ){
		nb = fread(buffer, 1, BUFSIZE, fp);		
		if( ferror(fp) ){
			perror("fread()");
			return -1;
		}

		if( nb == 0 )
			break;

		if( XML_ParseBuffer(p, nb, feof(fp)) == XML_STATUS_ERROR ){
			fprintf(stderr, "XML parsing error at line %lu:\n%s\n",
				XML_GetCurrentLineNumber(p),
				XML_ErrorString(XML_GetErrorCode(p)));
			fclose(fp);
			return -1;
		}
	}

	XML_ParserFree(p);
	fclose(fp);
	free(config_file);

	return 0;
}
