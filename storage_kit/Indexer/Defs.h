/* Defs.h */
/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

#ifndef INDEXER_DEFS_H
#define INDEXER_DEFS_H

#include <GraphicsDefs.h>
#include <Rect.h>
#include <Application.h>


static const rgb_color dark = {150,150,150};
static const rgb_color medium = {200,200,200};
static const rgb_color light = {245,245,245};

static const rgb_color black = {0,0,0};
static const rgb_color red = {200,0,0};

#define INFO_OFFSET	75
const BRect	infoRect (0,0,270,280);

const char lName[] = "Name";
const char lMountPt[] = "Mount Point";
const char lSize[] = "Size";
const char lVolFlags[] = "Volume Flags";
const char lAware[] = "Awareness";
const char lIndexed[] = "Indexed";
const char lAttr[] = "Attributes";
const char lPath[] = "Path";
const char lCreated[] = "Created";
const char lMod[] = "Modified";

const char Persistent[] = "Persistent";
const char Virtual[] = "Virtual";
const char ReadOnly[] = "Read Only";
const char Removable[] = "Removable";
const char Shared[] = "Shared";

const char KnowsAttr[] = "Attribute Aware";
const char KnowsMime[] = "MIME Aware";
const char KnowsQuery[] = "Query Aware";

const char lEntries[] = "Entries";
const char lSubDir[] = "Sub-Directories";
const char lLinks[] = "Links";
const char lFiles[] = "Files";
const char lNotIndexed[] = "Not Indexed";
const char lPartialIndexed[] = "Partially Indexed";
const char lInvalid[] ="Invalid";
const char lTypes[] = "Indexed/Not Indexed";

const char lKind[] = "Kind";
const char lFile[] = "File";
const char lDir[] = "Directory";
const char lLink[] = "Symbolic Link";
const char lVol[] = "Volume";
const char lMime[] = "MIME Type";
const char lStatus[] = "Status";
const char lType[] = "Type";
const char lLinkTo[] = "Link To";
const char lAbsolute[] = "Absolute";
const char lRelative[] = "Relative";

status_t 	get_attribute_indices(dev_t device, BList &index_list);
bool 		make_view_items(void *item, void *view);
status_t	reindex_node(BNode &node, BList &index_list);

enum {
	INDEXED			= 1,
	PARTIAL_INDEXED	= 2,
	NOT_INDEXED		= 3
};

#endif

