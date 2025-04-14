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

#include "homebank.h"
#include "hb-tag.h"

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;


#if MYDEBUG
static void da_tag_debug_array(guint32 *tags);
#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void da_tag_free(Tag *item)
{
	DB( g_print("da_tag_free\n") );
	if(item != NULL)
	{
		DB( g_print(" => %d, %s\n", item->key, item->name) );

		g_free(item->name);
		g_free(item);
	}
}


Tag *da_tag_malloc(void)
{
	DB( g_print("da_tag_malloc\n") );
	return g_malloc0(sizeof(Tag));
}


void da_tag_destroy(void)
{
	DB( g_print("da_tag_destroy\n") );
	g_hash_table_destroy(GLOBALS->h_tag);
}


void da_tag_new(void)
{
Tag *item;

	DB( g_print("da_tag_new\n") );
	GLOBALS->h_tag = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)da_tag_free);

	// insert our 'no tag'
	item = da_tag_malloc();
	item->name = g_strdup("");
	da_tag_insert(item);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
static void da_tag_max_key_ghfunc(gpointer key, Tag *item, guint32 *max_key)
{
	*max_key = MAX(*max_key, item->key);
}

static gboolean da_tag_name_grfunc(gpointer key, Tag *item, gchar *name)
{
	if( name && item->name )
	{
		if(!strcasecmp(name, item->name))
			return TRUE;
	}
	return FALSE;
}

/**
 * da_tag_length:
 *
 * Return value: the number of elements
 */
guint da_tag_length(void)
{
	return g_hash_table_size(GLOBALS->h_tag);
}

/**
 * da_tag_delete:
 *
 * delete an tag from the GHashTable
 *
 * Return value: TRUE if the key was found and deleted
 *
 */
gboolean da_tag_delete(guint32 key)
{
	DB( g_print("da_tag_delete %d\n", key) );

	return g_hash_table_remove(GLOBALS->h_tag, &key);
}

/**
 * da_tag_insert:
 *
 * insert an tag into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean da_tag_insert(Tag *item)
{
guint32 *new_key;

	DB( g_print("da_tag_insert\n") );

	new_key = g_new0(guint32, 1);
	*new_key = item->key;
	g_hash_table_insert(GLOBALS->h_tag, new_key, item);

	return TRUE;
}


/**
 * da_tag_append:
 *
 * append a new tag into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean da_tag_append(Tag *item)
{
Tag *existitem;
guint32 *new_key;

	DB( g_print("da_tag_append\n") );

	if( item->name != NULL )
	{
		/* ensure no duplicate */
		//g_strstrip(item->name);
		existitem = da_tag_get_by_name( item->name );
		if( existitem == NULL )
		{
			new_key = g_new0(guint32, 1);
			*new_key = da_tag_get_max_key() + 1;
			item->key = *new_key;

			DB( g_print(" -> append id: %d\n", *new_key) );

			g_hash_table_insert(GLOBALS->h_tag, new_key, item);
			return TRUE;
		}
	}

	DB( g_print(" -> %s already exist: %d\n", item->name, item->key) );

	return FALSE;
}

Tag *
da_tag_append_if_new(gchar *rawname)
{
Tag *retval = NULL;

	retval = da_tag_get_by_name(rawname);
	if(retval == NULL)
	{
		retval = da_tag_malloc();
		retval->key = da_tag_get_max_key() + 1;
		retval->name = g_strdup(rawname);
		g_strstrip(retval->name);
		da_tag_insert(retval);
	}

	return retval;
}


/**
 * da_tag_get_max_key:
 *
 * Get the biggest key from the GHashTable
 *
 * Return value: the biggest key value
 *
 */
guint32 da_tag_get_max_key(void)
{
guint32 max_key = 0;

	g_hash_table_foreach(GLOBALS->h_tag, (GHFunc)da_tag_max_key_ghfunc, &max_key);
	return max_key;
}


/**
 * da_tag_get_by_name:
 *
 * Get an tag structure by its name
 *
 * Return value: Tag * or NULL if not found
 *
 */
Tag *da_tag_get_by_name(gchar *name)
{
	DB( g_print("da_tag_get_by_name\n") );

	return g_hash_table_find(GLOBALS->h_tag, (GHRFunc)da_tag_name_grfunc, name);
}



/**
 * da_tag_get:
 *
 * Get an tag structure by key
 *
 * Return value: Tag * or NULL if not found
 *
 */
Tag *da_tag_get(guint32 key)
{
	DB( g_print("da_tag_get_tag\n") );

	return g_hash_table_lookup(GLOBALS->h_tag, &key);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

gboolean
tags_equal(guint32 *stags, guint32 *dtags)
{
guint count = 0;

	DB( g_print("\n[tags] compare\n") );

	if( stags == NULL && dtags == NULL )
		return TRUE;

	if( stags == NULL || dtags == NULL )
		return FALSE;

	while(*stags != 0 && *dtags != 0 && count < 32)
	{
		if(*stags++ != *dtags++)
			return FALSE;
	}
	// both should be 0
	if(*stags != *dtags)	
		return FALSE;

	return TRUE;
}


guint
tags_count(guint32 *tags)
{
guint count = 0;

	DB( g_print("\n[tags] count\n") );

	if( tags == NULL )
		return 0;

	while(*tags++ != 0 && count < 32)
		count++;

	return count;
}


guint32 *tags_clone(guint32 *tags)
{
guint32 *newtags = NULL;
guint count;

	DB( g_print("\n[tags] clone %p\n", tags) );

	if( tags == NULL )
		return NULL;

	count = tags_count(tags);
	if(count > 0)
	{
		//1501962: we must also copy the final 0
		newtags = g_memdup(tags, (count+1)*sizeof(guint32));
	}

	return newtags;	
}


static gboolean
tags_key_exists(guint32 *tags, guint32 key)
{
guint count = 0;

	if( tags == NULL )
		return FALSE;

	while(*tags != 0 && count < 32)
	{
		if( *tags == key )
			return TRUE;
		tags++;
		count++;
	}
	return FALSE;
}


static void
tags_deduplicate(guint32 *tags)
{
guint32 *tmp, *s, *d;
guint count = 0;
	
	DB( g_print("\n[tags] deduplicate\n") );

	if( tags == NULL )
		return;

	DB( g_print(" tags %p\n", tags) );
	
	tmp = s = tags_clone(tags);
	d = tags;

	DB( g_print(" tmp %p\n", tmp) );
	DB( g_print(" s %p\n", s) );


	*d = 0;
	while(*s != 0 && count < 32)
	{
		DB( g_print(" - tst %d\n", *s) );

		if( tags_key_exists(tags, *s) == FALSE )
		{
			*d = *s;
			d++;
			*d = 0;
			DB( g_print(" - add %d\n", *s) );
		}
		else
		{
			DB( g_print(" - skip %d\n", *s) );
		}
		s++;
		count++;
	}

	g_free(tmp);
}


gint
tags_delete_unused(void)
{
GList *ltag, *list;
gint count = 0;

	ltag = list = g_hash_table_get_values(GLOBALS->h_tag);
	while (list != NULL)
	{
	Tag *entry = list->data;

		if(entry->nb_use_all <= 0 && entry->key > 0)
		{
			da_tag_delete (entry->key);
			count++;
		}
		list = g_list_next(list);
	}
	g_list_free(ltag);
	return count;
}


static void
_tags_fill_usage(guint32 *tags, gboolean txn)
{
guint count = 0;

	if(tags != NULL)
	{
		while(*tags != 0 && count < 32)
		{
		Tag *tag = da_tag_get(*tags);

			//#2106027 crash
			if( tag != NULL )
			{
				tag->nb_use_all++;
				if( txn == TRUE )
					tag->nb_use_txn++;
			}
			tags++;
			count++;
		}
	}
}


void
tags_fill_usage(void)
{
GList *ltag;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
GList *list;

	DB( g_print("[tags] fill usage\n") );


	ltag = list = g_hash_table_get_values(GLOBALS->h_tag);
	while (list != NULL)
	{
	Tag *entry = list->data;
		entry->nb_use_all = 0;
		entry->nb_use_txn = 0;
		list = g_list_next(list);
	}
	g_list_free(ltag);


	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			_tags_fill_usage(txn->tags, TRUE);
			lnk_txn = g_list_next(lnk_txn);
		}

		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);


	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *entry = list->data;

		_tags_fill_usage(entry->tags, FALSE);
		list = g_list_next(list);
	}

	//future assign


}


static void
tags_move(guint32 *tags, guint32 key1, guint32 key2)
{
guint count = 0;
guint countdup = 0;
guint32 *p;

	if( tags == NULL )
		return;

	DB( g_print("\n[tags] move\n") );

	p = tags;
	while(*p != 0 && count < 32)
	{
		if( *p == key1 )
		{
			DB( g_print(" change %d to %d\n", key1, key2) );
			*p = key2;
		}

		//count potential duplicate
		if( *p == key2 )
			countdup++;

		p++;
		count++;
	}

	//TODO: ensure no duplicate on key2
	if( countdup )
		tags_deduplicate(tags);

}


guint32 *
tags_parse(const gchar *tagstring)
{
gchar **str_array;
guint32 *tags = NULL;
guint32 *ptags;
guint count, i;
Tag *tag;

	DB( g_print("\n[tags] parse\n") );

	if( tagstring )
	{
		str_array = g_strsplit (tagstring, " ", 0);
		count = g_strv_length( str_array );
		DB( g_print("- %d tags '%s'\n", count, tagstring) );
		if( count > 0 )
		{
			tags = g_new0(guint32, count + 1);
			ptags = tags;
			for(i=0;i<count;i++)
			{
				//5.2.3 fixed empty tag
				if( strlen(str_array[i]) == 0 )
					continue;

				DB( g_print("- %d search '%s'\n", i, str_array[i]) );
				tag = da_tag_get_by_name(str_array[i]);
				if(tag == NULL)
				{
				Tag *newtag = da_tag_malloc();

					newtag->name = g_strdup(str_array[i]);
					da_tag_append(newtag);
					tag = da_tag_get_by_name(str_array[i]);
				}
				DB( g_print("- array add %d '%s'\n", tag->key, tag->name) );

				//5.3 fixed duplicate tag in same tags
				if( tags_key_exists(tags, tag->key) == FALSE )
					*ptags++ = tag->key;
			}
			*ptags = 0;
		}

		g_strfreev (str_array);
	}
	return tags;
}



gchar *
tags_tostring(guint32 *tags)
{
guint count, i;
gchar **str_array, **tptr;
gchar *tagstring;
Tag *tag;

	DB( g_print("\n[tags] tostring\n") );
	if( tags == NULL )
	{
		return NULL;
	}
	else
	{
		count = tags_count(tags);
		str_array = g_new0(gchar*, count+1);
		tptr = str_array;
		for(i=0;i<count;i++)
		{
			tag = da_tag_get(tags[i]);
			if( tag )
			{
				*tptr++ = tag->name;
			}
		}
		*tptr = NULL;
		
		tagstring = g_strjoinv(" ", str_array);
		g_free (str_array);
	}
	return tagstring;
}


void da_tag_consistency(Tag *item)
{
	//#2018414 replace any space by -
	hb_string_replace_char(' ', '-', item->name);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


#if MYDEBUG

static void
da_tag_debug_array(guint32 *tags)
{
guint32 count = 0;

	if( tags == NULL )
	{
		DB( g_print(" dbg: no tags\n") );
		return;
	}

	while(*tags != 0 && count < 32)
	{
		DB( g_print(" [%d]=%d\n", count, *tags) );
		tags++;
		count++;
	}
	DB( g_print(" [%d]=%d\n", count, *tags) );
	return;
}

static void
da_tag_debug_list_ghfunc(gpointer key, gpointer value, gpointer user_data)
{
guint32 *id = key;
Tag *item = value;

	DB( g_print(" %d :: %s\n", *id, item->name) );

}

static void
da_tag_debug_list(void)
{

	DB( g_print("\n** debug **\n") );

	g_hash_table_foreach(GLOBALS->h_tag, da_tag_debug_list_ghfunc, NULL);

	DB( g_print("\n** end debug **\n") );

}

#endif

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */



void
tag_move(guint32 key1, guint32 key2)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;

	DB( g_print("\n[tag] move %d => %d\n", key1, key2) );

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			tags_move(txn->tags, key1, key2);
			
			lnk_txn = g_list_next(lnk_txn);
		}
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

}


gboolean
tag_rename(Tag *item, const gchar *newname)
{
Tag *existitem;
gchar *stripname;
gboolean retval = FALSE;

	DB( g_print("\n[tag] rename\n") );
	
	stripname = g_strdup(newname);
	g_strstrip(stripname);
	
	//#2018414 replace any space by -
	hb_string_replace_char(' ', '-', stripname);

	existitem = da_tag_get_by_name(stripname);

	if( existitem != NULL && existitem->key != item->key)
	{
		DB( g_print("- error, same name already exist with other key %d <> %d\n",existitem->key, item->key) );
		g_free(stripname);
	}
	else
	{
		DB( g_print("- renaming\n") );
		g_free(item->name);
		item->name = stripname;
		retval = TRUE;
	}

	return retval;
}



static gint
tag_glist_name_compare_func(Tag *a, Tag *b)
{
	return hb_string_utf8_compare(a->name, b->name);
}


static gint
tag_glist_key_compare_func(Tag *a, Tag *b)
{
	return a->key - b->key;
}


GList *tag_glist_sorted(gint column)
{
GList *list = g_hash_table_get_values(GLOBALS->h_tag);

	switch(column)
	{
		case HB_GLIST_SORT_NAME:
			return g_list_sort(list, (GCompareFunc)tag_glist_name_compare_func);
			break;
		//case HB_GLIST_SORT_KEY:
		default:
			return g_list_sort(list, (GCompareFunc)tag_glist_key_compare_func);		
	}
}


gboolean
tag_load_csv(gchar *filename, gchar **error)
{
gboolean retval;
GIOChannel *io;
gchar *tmpstr;
gint io_stat;
const gchar *encoding;

	encoding = homebank_file_getencoding(filename);
	DB( g_print(" -> encoding should be %s\n", encoding) );

	retval = TRUE;
	*error = NULL;
	io = g_io_channel_new_file(filename, "r", NULL);
	if(io != NULL)
	{
		if( encoding != NULL )
		{
			g_io_channel_set_encoding(io, encoding, NULL);
		}

		for(;;)
		{
			io_stat = g_io_channel_read_line(io, &tmpstr, NULL, NULL, NULL);
			if( io_stat == G_IO_STATUS_EOF)
				break;
			if( io_stat == G_IO_STATUS_NORMAL)
			{
				if( tmpstr != NULL)
				{
				Tag *tag;
				
					DB( g_print("\n + strip\n") );
					hb_string_strip_crlf(tmpstr);
					
					//#2018414 replace any space by -
					hb_string_replace_char(' ', '-', tmpstr);

					DB( g_print(" add tag:'%s' ?\n", tmpstr) );
					tag = da_tag_append_if_new(tmpstr);
					if(	tag != NULL )
					{
						GLOBALS->changes_count++;
					}
				}
				g_free(tmpstr);
			}

		}
		g_io_channel_unref (io);
	}

	return retval;
}


void
tag_save_csv(gchar *filename)
{
GIOChannel *io;
GList *ltag, *list;
gchar *outstr;

	io = g_io_channel_new_file(filename, "w", NULL);
	if(io != NULL)
	{
		ltag = list = tag_glist_sorted(HB_GLIST_SORT_NAME);

		while (list != NULL)
		{
		Tag *item = list->data;

			if(item->key != 0)
			{
				outstr = g_strdup_printf("%s\n", item->name);

				DB( g_print(" + export %s\n", outstr) );

				g_io_channel_write_chars(io, outstr, -1, NULL, NULL);

				g_free(outstr);
			}
			list = g_list_next(list);
		}
		g_list_free(ltag);

		g_io_channel_unref (io);
	}
}

