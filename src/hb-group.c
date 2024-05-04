/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2023 Maxime DOYEN
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

#include "homebank.h"
#include "hb-group.h"

#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void da_grp_free(Group *item)
{
	DB( g_print("da_group_free\n") );
	if(item != NULL)
	{
		DB( g_print(" => %d, %s\n", item->key, item->name) );

		g_free(item->name);
		g_free(item);
	}
}


Group *da_grp_malloc(void)
{
	DB( g_print("da_group_malloc\n") );
	return g_malloc0(sizeof(Group));
}


void da_grp_destroy(void)
{
	DB( g_print("da_group_destroy\n") );
	g_hash_table_destroy(GLOBALS->h_grp);
}


void da_grp_new(void)
{
Group *item;

	DB( g_print("da_group_new\n") );
	GLOBALS->h_grp = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)da_grp_free);

	// insert our 'no group'
	item = da_grp_malloc();
	item->name = g_strdup("");
	da_grp_insert(item);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


guint
da_grp_length(void)
{
	return g_hash_table_size(GLOBALS->h_grp);
}


static void 
da_grp_max_key_ghfunc(gpointer key, Group *item, guint32 *max_key)
{
	*max_key = MAX(*max_key, item->key);
}

guint32
da_grp_get_max_key(void)
{
guint32 max_key = 0;

	g_hash_table_foreach(GLOBALS->h_grp, (GHFunc)da_grp_max_key_ghfunc, &max_key);
	return max_key;
}


gboolean
da_grp_remove(guint32 key)
{
	DB( g_print("da_grp_remove %d\n", key) );

	return g_hash_table_remove(GLOBALS->h_grp, &key);
}


gboolean
da_grp_insert(Group *item)
{
guint32 *new_key;

	DB( g_print("da_grp_insert\n") );

	new_key = g_new0(guint32, 1);
	*new_key = item->key;
	g_hash_table_insert(GLOBALS->h_grp, new_key, item);

	return TRUE;
}


gboolean
da_grp_append(Group *item)
{
Group *existitem;

	DB( g_print("da_grp_append\n") );

	existitem = da_grp_get_by_name( item->name);
	if( existitem == NULL )
	{
		item->key = da_grp_get_max_key() + 1;
		da_grp_insert(item);
		return TRUE;
	}

	DB( g_print(" -> %s already exist: %d\n", item->name, item->key) );

	return FALSE;
}


Group *
da_grp_get_by_name(gchar *rawname)
{
Group *retval = NULL;
gchar *stripname;
GHashTableIter iter;
gpointer key, value;
	
	DB( g_print("da_grp_get_by_name\n") );

	if( rawname )
	{
		stripname = g_strdup(rawname);
		g_strstrip(stripname);
		if( strlen(stripname) > 0 )
		{
			g_hash_table_iter_init (&iter, GLOBALS->h_grp);
			while (g_hash_table_iter_next (&iter, &key, &value))
			{
			Group *item = value;

				//if( item->type != type )
				//	continue;
			
				if( stripname && item->name )
				{
					if(!strcasecmp(stripname, item->name))
					{
						retval = item;
						break;
					}
				}
			}			
		}	
		g_free(stripname);
	}

	return retval; 
}


Group *
da_grp_get(guint32 key)
{
	//DB( g_print("da_grp_get\n") );

	return g_hash_table_lookup(GLOBALS->h_grp, &key);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


void group_delete_unused(void)
{
guint32 i, max_key;
gboolean *used;
GList *lst_acc, *lnk_acc;

	max_key = da_grp_get_max_key();
	used = g_malloc0((max_key + 1) * sizeof(gboolean));
	if( used )
	{
		// collect usage
		lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
		lnk_acc = g_list_first(lst_acc);
		while (lnk_acc != NULL)
		{
		Account *acc = lnk_acc->data;

			if(acc)
				used[acc->kgrp] = TRUE;

			lnk_acc = g_list_next(lnk_acc);
		}
		g_list_free(lst_acc);

		//clean unused
		for(i=1;i<max_key;i++)
		{
			if( used[i] == 0 )
			{
			Group *grp = da_grp_get(i);
				
				if(grp)
				{
					DB( g_print(" - '%s' is unused, removing\n", grp->name) );
					da_grp_remove(i);
				}
			}
		}
		
		g_free(used);
	}
}


static gint
group_glist_name_compare_func(Group *a, Group *b)
{
	return hb_string_utf8_compare(a->name, b->name);
}


static gint
group_glist_key_compare_func(Group *a, Group *b)
{
	return a->key - b->key;
}


GList *group_glist_sorted(gint column)
{
GList *list = g_hash_table_get_values(GLOBALS->h_grp);

	switch(column)
	{
		case HB_GLIST_SORT_NAME:
			return g_list_sort(list, (GCompareFunc)group_glist_name_compare_func);
			break;
		//case HB_GLIST_SORT_KEY:
		default:
			return g_list_sort(list, (GCompareFunc)group_glist_key_compare_func);
			break;
	}
}

