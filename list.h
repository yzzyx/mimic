#ifndef __LIST_H
#define __LIST_H

typedef struct __dl_list dl_list;

struct __dl_list{ 
	dl_list *next;
	dl_list *prev;
	void *data;
};

dl_list *dl_list_add(dl_list *list, void *data);
void dl_list_free_int(dl_list *list, int free_data);
#define dl_list_free(list) dl_list_free_int(list, 0)
#define dl_list_free_data(list) dl_list_free_int(list, 1)
dl_list *dl_list_qsort( dl_list *list, int (*sort_func)(void *, void *) );


#endif
