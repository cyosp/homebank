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
#include "hb-payee.h"


/****************************************************************************/
/* Debug macros										 */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

//Payee *
//da_pay_clone


void
da_pay_free(Payee *item)
{
	DB( g_print("da_pay_free\n") );
	if(item != NULL)
	{
		DB( g_print(" => %d, %s\n", item->key, item->name) );

		g_free(item->name);
		g_free(item->notes);
		g_free(item);
	}
}


Payee *
da_pay_malloc(void)
{
	DB( g_print("da_pay_malloc\n") );
	return g_malloc0(sizeof(Payee));
}


void
da_pay_destroy(void)
{
	DB( g_print("da_pay_destroy\n") );
	g_hash_table_destroy(GLOBALS->h_pay);
}


void
da_pay_new(void)
{
Payee *item;

	DB( g_print("da_pay_new\n") );
	GLOBALS->h_pay = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)da_pay_free);

	// insert our 'no payee'
	item = da_pay_malloc();
	item->name = g_strdup("");
	da_pay_insert(item);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

/**
 * da_pay_length:
 *
 * Return value: the number of elements
 */
guint
da_pay_length(void)
{
	return g_hash_table_size(GLOBALS->h_pay);
}


static void
da_pay_max_key_ghfunc(gpointer key, Payee *item, guint32 *max_key)
{
	*max_key = MAX(*max_key, item->key);
}


/**
 * da_pay_get_max_key:
 *
 * Get the biggest key from the GHashTable
 *
 * Return value: the biggest key value
 *
 */
guint32
da_pay_get_max_key(void)
{
guint32 max_key = 0;

	g_hash_table_foreach(GLOBALS->h_pay, (GHFunc)da_pay_max_key_ghfunc, &max_key);
	return max_key;
}


/**
 * da_pay_delete:
 *
 * delete an payee from the GHashTable
 *
 * Return value: TRUE if the key was found and deleted
 *
 */
gboolean
da_pay_delete(guint32 key)
{
	DB( g_print("da_pay_delete %d\n", key) );

	return g_hash_table_remove(GLOBALS->h_pay, &key);
}



//#1889659: ensure name != null/empty
static gboolean
da_pay_ensure_name(Payee *item)
{
	// (no payee) have name=""
	if( item->key > 0 )
	{
		if( item->name == NULL || strlen(item->name) == 0 )
		{
			g_free(item->name);
			item->name = g_strdup_printf("no name %d", item->key);
			return TRUE;
		}
	}
	return FALSE;
}


static void
da_pay_rename(Payee *item, gchar *newname)
{

	DB( g_print("- renaming '%s' => '%s'\n", item->name, newname) );

	g_free(item->name);
	item->name = g_strdup(newname);

	//#1889659: ensure name != null/empty
	da_pay_ensure_name(item);
}


/**
 * da_pay_insert:
 *
 * insert an payee into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean
da_pay_insert(Payee *item)
{
guint32 *new_key;

	DB( g_print("da_pay_insert\n") );

	new_key = g_new0(guint32, 1);
	*new_key = item->key;

	//#1889659: ensure name != null/empty
	da_pay_ensure_name(item);

	g_hash_table_insert(GLOBALS->h_pay, new_key, item);

	return TRUE;
}


/**
 * da_pay_append:
 *
 * append a new payee into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean
da_pay_append(Payee *item)
{
Payee *existitem;

	DB( g_print("da_pay_append\n") );

	existitem = da_pay_get_by_name( item->name );
	if( existitem == NULL )
	{
		item->key = da_pay_get_max_key() + 1;
		da_pay_insert(item);
		return TRUE;
	}

	DB( g_print(" -> %s already exist: %d\n", item->name, item->key) );
	return FALSE;
}


/**
 * da_pay_append_if_new:
 *
 * append a new payee into the GHashTable
 *
 * Return value: existing or new payee
 *
 */
Payee *
da_pay_append_if_new(gchar *rawname)
{
Payee *retval = NULL;

	retval = da_pay_get_by_name(rawname);
	if(retval == NULL)
	{
		retval = da_pay_malloc();
		retval->key = da_pay_get_max_key() + 1;
		retval->name = g_strdup(rawname);
		g_strstrip(retval->name);
		da_pay_insert(retval);
	}

	return retval;
}


static gboolean
da_pay_name_grfunc(gpointer key, Payee *item, gchar *name)
{
	if( name && item->name )
	{
		if(!strcasecmp(name, item->name))
			return TRUE;
	}
	return FALSE;
}


/**
 * da_pay_get_by_name:
 *
 * Get an payee structure by its name
 *
 * Return value: Payee * or NULL if not found
 *
 */
Payee *
da_pay_get_by_name(gchar *rawname)
{
Payee *retval = NULL;
gchar *stripname;

	DB( g_print("da_pay_get_by_name\n") );

	if( rawname )
	{
		stripname = g_strdup(rawname);
		g_strstrip(stripname);
		if( strlen(stripname) == 0 )
			retval = da_pay_get(0);
		else
			retval = g_hash_table_find(GLOBALS->h_pay, (GHRFunc)da_pay_name_grfunc, stripname);
		g_free(stripname);
	}
	return retval;
}


/**
 * da_pay_get:
 *
 * Get a payee structure by key
 *
 * Return value: Payee * or NULL if not found
 *
 */
Payee *
da_pay_get(guint32 key)
{
	//DB( g_print("da_pay_get\n") );

	return g_hash_table_lookup(GLOBALS->h_pay, &key);
}


//gchar *da_pay_get_name(Payee *item)


void da_pay_consistency(Payee *item)
{
	g_strstrip(item->name);
	//5.2.4 we drop internal xfer here as it will disapear
	//was faulty possible
	if( item->paymode == OLDPAYMODE_INTXFER )
		item->paymode = PAYMODE_XFER;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

#if MYDEBUG

static void
da_pay_debug_list_ghfunc(gpointer key, gpointer value, gpointer user_data)
{
guint32 *id = key;
Payee *item = value;

	DB( g_print(" %d :: %s\n", *id, item->name) );

}

static void
da_pay_debug_list(void)
{

	DB( g_print("\n** debug **\n") );

	g_hash_table_foreach(GLOBALS->h_pay, da_pay_debug_list_ghfunc, NULL);

	DB( g_print("\n** end debug **\n") );

}

#endif


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

gint
payee_delete_unused(void)
{
GList *lpay, *list;
gint count = 0;

	lpay = list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *entry = list->data;

		if(entry->nb_use_all <= 0 && entry->key > 0)
		{
			da_pay_delete (entry->key);
			count++;
		}
		list = g_list_next(list);
	}
	g_list_free(lpay);
	return count;
}


void
payee_fill_usage(void)
{
GList *lpay;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
GList *lrul, *list;

	DB( g_print("[da_pay] fill usage\n") );


	lpay = list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *entry = list->data;
		entry->nb_use_all = 0;
		entry->nb_use_txn = 0;
		list = g_list_next(list);
	}
	g_list_free(lpay);


	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;
		Payee *pay = da_pay_get (txn->kpay);

			if(pay)
			{
				pay->nb_use_all++;
				pay->nb_use_txn++;
			}
			lnk_txn = g_list_next(lnk_txn);
		}

		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);


	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *entry = list->data;
	Payee *pay = da_pay_get (entry->kpay);

		if(pay)
			pay->nb_use_all++;
		list = g_list_next(list);
	}

	lrul = list = g_hash_table_get_values(GLOBALS->h_rul);
	while (list != NULL)
	{
	Assign *entry = list->data;
	Payee *pay = da_pay_get (entry->kpay);

		if(pay)
			pay->nb_use_all++;

		list = g_list_next(list);
	}
	g_list_free(lrul);

}


void
payee_move(guint32 key1, guint32 key2)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
GList *lrul, *list;

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			if(txn->kpay == key1)
			{
				txn->kpay = key2;
				txn->dspflags |= FLAG_TMP_EDITED;
			}
			lnk_txn = g_list_next(lnk_txn);
		}
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);


	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *entry = list->data;
		if(entry->kpay == key1)
		{
			entry->kpay = key2;
		}
		list = g_list_next(list);
	}

	lrul = list = g_hash_table_get_values(GLOBALS->h_rul);
	while (list != NULL)
	{
	Assign *entry = list->data;

		if(entry->kpay == key1)
		{
			entry->kpay = key2;
		}
		list = g_list_next(list);
	}
	g_list_free(lrul);
}


gboolean
payee_rename(Payee *item, const gchar *newname)
{
Payee *existitem;
gchar *stripname;
gboolean retval = FALSE;

	stripname = g_strdup(newname);
	g_strstrip(stripname);

	existitem = da_pay_get_by_name(stripname);

	if( existitem != NULL && existitem->key != item->key)
	{
		DB( g_print("- error, same name already exist with other key %d <> %d\n", existitem->key, item->key) );
	}
	else
	{
		DB( g_print("- renaming\n") );

		da_pay_rename (item, stripname);
		retval = TRUE;
	}

	g_free(stripname);

	return retval;
}


static gint
payee_glist_name_compare_func(Payee *a, Payee *b)
{
	return hb_string_utf8_compare(a->name, b->name);
}


static gint
payee_glist_key_compare_func(Payee *a, Payee *b)
{
	return a->key - b->key;
}


GList *
payee_glist_sorted(gint column)
{
GList *list = g_hash_table_get_values(GLOBALS->h_pay);

	switch(column)
	{
		case HB_GLIST_SORT_NAME:
			return g_list_sort(list, (GCompareFunc)payee_glist_name_compare_func);
			break;
		//case HB_GLIST_SORT_KEY:
		default:
			return g_list_sort(list, (GCompareFunc)payee_glist_key_compare_func);
			break;
	}
}


gboolean
payee_load_csv(gchar *filename, gchar **error)
{
gboolean retval;
GIOChannel *io;
gchar *tmpstr;
gint io_stat;
gchar **str_array;
const gchar *encoding;
gint nbcol;

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
					DB( g_print("\n + strip\n") );
					hb_string_strip_crlf(tmpstr);

					DB( g_print(" + split '%s'\n", tmpstr) );
					str_array = g_strsplit (tmpstr, ";", 2);
					// payee;category   : later paymode?

					nbcol = g_strv_length (str_array);
					if( nbcol > 2 )
					{
						*error = _("invalid CSV format");
						retval = FALSE;
						DB( g_print(" + error %s\n", *error) );
					}
					else
					{
					Payee *pay = NULL;
					Category *cat = NULL;

						if( nbcol >= 1 )
						{
							DB( g_print(" add pay:'%s' ?\n", str_array[0]) );
							pay = da_pay_append_if_new(str_array[0]);
							DB( g_print(" pay: %p\n", pay) );
							if(	pay != NULL )
							{
								GLOBALS->changes_count++;
							}
						}

						if( nbcol == 2 )
						{
							DB( g_print(" add cat:'%s'\n", str_array[1]) );
							cat = da_cat_append_ifnew_by_fullname(str_array[1]);

							DB( g_print(" cat: %p %p\n", cat, pay) );
							if( cat != NULL )
							{
								if( pay != NULL)
								{
									DB( g_print(" set default cat to %d\n", cat->key) );
									pay->kcat = cat->key;
								}
								GLOBALS->changes_count++;
							}
						}
					}
					g_strfreev (str_array);
				}
				g_free(tmpstr);
			}

		}
		g_io_channel_unref (io);
	}

	return retval;
}


void
payee_save_csv(gchar *filename)
{
GIOChannel *io;
GList *lpay, *list;
gchar *outstr;

	io = g_io_channel_new_file(filename, "w", NULL);
	if(io != NULL)
	{
		lpay = list = payee_glist_sorted(HB_GLIST_SORT_NAME);

		while (list != NULL)
		{
		Payee *item = list->data;
		gchar *fullcatname;

			if(item->key != 0)
			{
				fullcatname = NULL;
				if( item->kcat > 0 )
				{
				Category *cat = da_cat_get(item->kcat);

					if( cat != NULL )
					{
						fullcatname = cat->fullname;
					}
				}

				if( fullcatname != NULL )
					outstr = g_strdup_printf("%s;%s\n", item->name, fullcatname);
				else
					outstr = g_strdup_printf("%s;\n", item->name);

				DB( g_print(" + export %s %s\n", item->name, fullcatname) );

				g_io_channel_write_chars(io, outstr, -1, NULL, NULL);

				g_free(outstr);
				//#1999250 don't free here...
				//g_free(fullcatname);
			}
			list = g_list_next(list);
		}
		g_list_free(lpay);

		g_io_channel_unref (io);
	}
	
	DB( g_print(" export ok\n") );
}

