/* Defs.cpp */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#include "Defs.h"

#include <List.h>
#include <be/kernel/fs_index.h>
#include <posix/string.h>
#include <List.h>
#include <ListView.h>
#include <ListItem.h>
#include <be/kernel/fs_attr.h>
#include <posix/malloc.h>
#include <posix/stdio.h>
#include <Mime.h>

extern status_t 
get_attribute_indices(dev_t device, BList &index_list)
{
	DIR *index_dir;
	struct dirent *index_ent;
	
	index_dir = fs_open_index_dir(device);
	if (!index_dir) {
		return B_ERROR;
	}
	
	while ((index_ent = fs_read_index_dir(index_dir))) {
		char *text = strdup(index_ent->d_name);
		index_list.AddItem(text);
	}
	
	fs_close_index_dir(index_dir);
	
	return B_OK;	
}

extern bool 
make_view_items(void *item, void *view)
{
	char * string = (char *) item;
	BListView * list_view = (BListView *) view;
	
	BStringItem * string_item = new BStringItem(string);
	list_view->AddItem(string_item);
	return false;
}


extern status_t 
reindex_node(BNode &node, BList &index_list)
{
	
	attr_info info;

	int32 to_be_indexed = 0;
	int32 indexed = 0;
	int32 not_indexed = 0;
	
	int32 size = 1024;
	char *value = (char *) malloc(size * sizeof(char));
		

	//rewrite all of the appropriate attributes
	for (int32 i = 0; i < index_list.CountItems(); i++) {
		char *attr = (char *) index_list.ItemAt(i);
		
		if (node.GetAttrInfo(attr, &info) == B_OK) {
			to_be_indexed++;
			
			// adjust the size of our static buffer if necessary
			if (info.size > size) {
				value = (char *) realloc(value, info.size);
				size = info.size;			
			}

			if (node.ReadAttr(attr, info.type, 0, value, info.size) > 0) {
				if (node.WriteAttr(attr, info.type, 0, value, info.size) > 0)
					indexed++;
				else not_indexed++;
			} else not_indexed++;
		}	
	}

	free(value); value = NULL;	


	if (to_be_indexed > 0) {
		
		if (indexed > 0) {
			if (not_indexed > 0) return PARTIAL_INDEXED;
			else return INDEXED;
			
		} else return NOT_INDEXED;
		
	} else return NOT_INDEXED;
}


