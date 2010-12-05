#include <stdlib.h>
#include <stdio.h> /* perror() */
#include "list.h"


dl_list *dl_list_add(dl_list *list, void *data)
{
	dl_list *node;

	if( list )
		while( list->next ) list = list->next;

	if( (node = malloc(sizeof(dl_list))) == NULL ){
		perror("malloc()");
		return NULL;
	}

	node->prev = list;
	node->next = NULL;
	node->data = data;

	if( list )
		list->next = node;
	
	return node;
}

void dl_list_free_int(dl_list *list, int free_data)
{
	dl_list *node, *tmp;

	node = list;
	while( node->prev ) node = node->prev;

	while( node->next ) {
		tmp = node->next;
		if( free_data && node->data )
			free(node->data);
		free(node);
		node = tmp;
	}

	if( node )
		free(node);
}

dl_list *dl_list_qsort( dl_list *list, int (*sort_func)(void *, void *) )
{
	int l, r;
	int n;
	int cmp;
	dl_list *node, *first_node, *last_node,
		*i_node, *j_node,
		*left_list = NULL, *right_list = NULL;

	if( list == NULL )
		return list;

	/* Get first node */
	node = list;
	while( node && node->prev )
		node = node->prev;
	first_node = node;
	
	/* Count number of objects, and get last node */
	n = 0;
	while( node ){
		if( node->next == NULL )
			last_node = node;
		node = node->next;
		n++;
	}

	if( n <= 1 )
		return list;

	l = 1; r = n;

	i_node = first_node;
	j_node = last_node;

	while( i_node != j_node ){
		if( (cmp = sort_func( i_node->data, j_node->data )) <= 0 )
			left_list = dl_list_add(left_list, i_node->data); 
		else
			right_list = dl_list_add(right_list, i_node->data);
		i_node = i_node->next;
	}
	
	/* Sort sublists */
	left_list = dl_list_qsort( left_list, sort_func );
	right_list = dl_list_qsort( right_list, sort_func );


	/* Add data from j_node */
	left_list = dl_list_add(left_list, j_node->data);

	/* Merge sublists */
	node = left_list;
	while( node && node->next )
		node = node->next;
	left_list = node;

	node = right_list;
	while( node && node->prev )
		node = node->prev;
	right_list = node;


	left_list->next = right_list;
	if( right_list ) right_list->prev = left_list;

	dl_list_free(list);

	return left_list;
}
