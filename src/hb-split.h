/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2025 Maxime DOYEN
 *
 *  This file is part of HomeBank.
 *
 *  HomeBank is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  HomeBank is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __HB_SPLIT_H__
#define __HB_SPLIT_H__


//for the record, quicken is limited to 250
#define TXN_MAX_SPLIT 62

#include "hb-types.h"


struct _split
{
	guint32		kcat;
	gdouble		amount;
	gchar		*memo;
	//unsaved data
	gushort		pos;
};


void da_split_free(Split *item);
Split *da_split_malloc(void);
void da_split_destroy(GPtrArray *splits);
GPtrArray *da_split_new(void);

void da_splits_sort(GPtrArray *splits);
guint da_splits_length(GPtrArray *splits);
gboolean da_splits_delete(GPtrArray *splits, Split *item);
void da_splits_append(GPtrArray *splits, Split *item);
Split *da_split_duplicate(Split *src);

Split *da_splits_get(GPtrArray *splits, guint index);
GPtrArray *da_splits_clone(GPtrArray *src_splits);

guint da_splits_parse(GPtrArray *splits, gchar *cats, gchar *amounts, gchar *memos);
guint da_splits_tostring(GPtrArray *splits, gchar **cats, gchar **amounts, gchar **memos);

guint da_splits_consistency (GPtrArray *splits);
guint da_splits_anonymize (GPtrArray *splits);


#endif
