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
#include "hb-category.h"


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
extern struct Preferences *PREFS;

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

Category *
da_cat_clone(Category *src_item)
{
Category *new_item = g_memdup(src_item, sizeof(Category));

	DB( g_print("da_cat_clone\n") );
	if(new_item)
	{
		//duplicate the string
		new_item->name = g_strdup(src_item->name);
		new_item->fullname = g_strdup(src_item->fullname);
		new_item->typename = g_strdup(src_item->typename);
	}
	return new_item;
}


void
da_cat_free(Category *item)
{
	DB( g_print("da_cat_free\n") );
	if(item != NULL)
	{
		DB( g_print(" => %d, %s\n", item->key, item->name) );

		g_free(item->name);
		g_free(item->fullname);
		g_free(item->typename);
		g_free(item);
	}
}


Category *
da_cat_malloc(void)
{
	DB( g_print("da_cat_malloc\n") );
	return g_malloc0(sizeof(Category));
}


void
da_cat_destroy(void)
{
	DB( g_print("da_cat_destroy\n") );
	g_hash_table_destroy(GLOBALS->h_cat);
}


void
da_cat_new(void)
{
Category *item;

	DB( g_print("da_cat_new\n") );
	GLOBALS->h_cat = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)da_cat_free);

	// insert our 'no category'
	item = da_cat_malloc();
	item->key = 0;
	item->name = g_strdup("");
	item->fullname = g_strdup("");
	item->typename = g_strdup("");
	da_cat_insert(item);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

/**
 * da_cat_length:
 *
 * Return value: the number of elements
 */
guint
da_cat_length(void)
{
	return g_hash_table_size(GLOBALS->h_cat);
}


static void
da_cat_max_key_ghfunc(gpointer key, Category *cat, guint32 *max_key)
{
	*max_key = MAX(*max_key, cat->key);
}

/**
 * da_cat_get_max_key:
 *
 * Get the biggest key from the GHashTable
 *
 * Return value: the biggest key value
 *
 */
guint32
da_cat_get_max_key(void)
{
guint32 max_key = 0;

	g_hash_table_foreach(GLOBALS->h_cat, (GHFunc)da_cat_max_key_ghfunc, &max_key);
	return max_key;
}


static gboolean
da_cat_remove_grfunc(gpointer key, Category *cat, guint32 *remkey)
{
	if(cat->key == *remkey || cat->parent == *remkey)
		return TRUE;

	return FALSE;
}


/**
 * da_cat_delete:
 *
 * delete a category from the GHashTable
 *
 * Return value: TRUE if the key was found and deleted
 *
 */
guint
da_cat_delete(guint32 key)
{
	DB( g_print("\nda_cat_delete %d\n", key) );

	return g_hash_table_foreach_remove(GLOBALS->h_cat, (GHRFunc)da_cat_remove_grfunc, &key);
}


static void
da_cat_build_typename(Category *item)
{
gchar *newname = NULL;
gchar type;

	type = category_get_type_char(item);

	if(item->key == 0)
		newname = g_strdup(item->name);
	else
	{
		if( item->parent == 0 )
			newname = g_markup_printf_escaped("%s [%c]", item->name, type);
			//string = g_strdup_printf("%s [%c]", name, type);
		else
			newname = g_markup_printf_escaped(" %c <i>%s</i>", type, item->name);
			//string = g_strdup_printf("%c <i>%s</i>", type, name);
	}

	if( newname )
	{
		g_free(item->typename);
		item->typename = newname;
		DB( g_print("- updated %d:'%s' typename='%s'\n", item->key, item->name, item->typename) );
	}
}


static void
da_cat_build_fullname(Category *item)
{
Category *parent;

	g_free(item->fullname);
	if( item->parent == 0 )
		item->fullname = g_strdup(item->name);
	else
	{
		parent = da_cat_get(item->parent);
		if( parent != NULL )
			item->fullname = g_strconcat(parent->name, ":", item->name, NULL);
	}

	DB( g_print("- updated %d:'%s' fullname='%s'\n", item->key, item->name, item->fullname) );

}


//#1889659: ensure name != null/empty
static gboolean
da_cat_ensure_name(Category *item)
{
	// (no category) have name=""
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
da_cat_rename(Category *item, gchar *newname)
{

	DB( g_print("- renaming '%s' => '%s'\n", item->name, newname) );

	g_free(item->name);
	item->name = g_strdup(newname);

	//#1889659: ensure name != null/empty
	da_cat_ensure_name(item);
	da_cat_build_fullname(item);
	da_cat_build_typename(item);

	if( item->parent == 0 )
	{
	GHashTableIter iter;
	gpointer value;

		DB( g_print("- updating subcat fullname\n") );

		g_hash_table_iter_init (&iter, GLOBALS->h_cat);
		while (g_hash_table_iter_next (&iter, NULL, &value))
		{
		Category *subcat = value;

			if( subcat->parent == item->key )
				da_cat_build_fullname(subcat);
		}

	}
}


/**
 * da_cat_insert:
 *
 * insert a category into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean
da_cat_insert(Category *item)
{
guint32 *new_key;

	DB( g_print("\nda_cat_insert\n") );

	DB( g_print("- '%s'\n", item->name) );

	new_key = g_new0(guint32, 1);
	*new_key = item->key;

	//#1889659: ensure name != null/empty
	da_cat_ensure_name(item);
	da_cat_build_fullname(item);
	da_cat_build_typename(item);

	g_hash_table_insert(GLOBALS->h_cat, new_key, item);

	return TRUE;
}


/**
 * da_cat_append:
 *
 * append a category into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
// used only to add cat/subcat from ui_category with the 2 inputs
gboolean
da_cat_append(Category *cat)
{
Category *existitem;

	DB( g_print("\nda_cat_append\n") );

	if( !cat->fullname )
		da_cat_build_fullname(cat);

	if( !cat->typename )
		da_cat_build_typename(cat);

	existitem = da_cat_get_by_fullname( cat->fullname );
	if( existitem == NULL )
	{
		cat->key = da_cat_get_max_key() + 1;
		da_cat_insert(cat);
		return TRUE;
	}

	DB( g_print(" -> %s already exist\n", cat->name) );
	return FALSE;
}




/* fullname i.e. car:refuel */
struct fullcatcontext
{
	guint32	parent;
	gchar	*name;
};


static gboolean
da_cat_fullname_grfunc(gpointer key, Category *item, struct fullcatcontext *ctx)
{

	//DB( g_print("'%s' == '%s'\n", ctx->name, item->name) );
	if( item->parent == ctx->parent )
	{
		if( ctx->name && item->name )
			if(!strcasecmp(ctx->name, item->name))
				return TRUE;
	}
	return FALSE;
}


static Category *da_cat_get_by_name_find_internal(guint32 parent, gchar *name)
{
struct fullcatcontext ctx;

	ctx.parent = parent;
	ctx.name   = name;
	DB( g_print("- searching %s %d '%s'\n", (parent == 0) ? "lv1cat" : "lv2cat", parent, name) );
	return g_hash_table_find(GLOBALS->h_cat, (GHRFunc)da_cat_fullname_grfunc, &ctx);
}


static gchar **da_cat_get_by_fullname_split_clean(gchar *rawfullname, guint *outlen)
{
gchar **partstr = g_strsplit(rawfullname, ":", 2);
guint len = g_strv_length(partstr);
gboolean valid = TRUE;

	DB( g_print("- spliclean '%s' - %d parts\n", rawfullname, g_strv_length(partstr)) );

	if( outlen != NULL )
		*outlen = len;

	if(len >= 1)
	{
		g_strstrip(partstr[0]);
		if( strlen(partstr[0]) == 0 )
		   valid = FALSE;

		if(len == 2)
		{
			g_strstrip(partstr[1]);
			if( strlen(partstr[1]) == 0 )
			   valid = FALSE;
	   }
	}

	if(valid == TRUE)
		return partstr;

	DB( g_print("- is invalid\n") );

	g_strfreev(partstr);
	return NULL;
}


Category *
da_cat_get_by_fullname(gchar *rawfullname)
{
gchar **partstr;
Category *parent = NULL;
Category *retval = NULL;
guint len;

	DB( g_print("\nda_cat_get_by_fullname\n") );

	if( rawfullname )
	{
		if( (partstr = da_cat_get_by_fullname_split_clean(rawfullname, &len)) != NULL )
		{
			if( len >= 1 )
			{
				parent = da_cat_get_by_name_find_internal(0, partstr[0]);
				retval = parent;
			}

			if( len == 2 && parent != NULL )
			{
				retval = da_cat_get_by_name_find_internal(parent->key, partstr[1]);
			}

			g_strfreev(partstr);
		}
	}

	return retval;
}


/**
 * da_cat_append_ifnew_by_fullname:
 *
 * append a category if it is new by fullname
 *
 * Return value:
 *
 */
Category *
da_cat_append_ifnew_by_fullname(gchar *rawfullname)
{
gchar **partstr;
Category *parent = NULL;
Category *newcat = NULL;
Category *retval = NULL;
guint len;

	DB( g_print("\nda_cat_append_ifnew_by_fullname\n") );

	if( rawfullname )
	{
		if( (partstr = da_cat_get_by_fullname_split_clean(rawfullname, &len)) != NULL )
		{
			if( len >= 1 )
			{
				parent = da_cat_get_by_name_find_internal(0, partstr[0]);
				if( parent == NULL )
				{
					parent = da_cat_malloc();
					parent->key = da_cat_get_max_key() + 1;
					parent->name = g_strdup(partstr[0]);
					da_cat_insert(parent);
				}
				retval = parent;
			}

			/* if we have a subcategory - xxx:xxx */
			if( len == 2 && parent != NULL )
			{
				newcat = da_cat_get_by_name_find_internal(parent->key, partstr[1]);
				if( newcat == NULL )
				{
					newcat = da_cat_malloc();
					newcat->key = da_cat_get_max_key() + 1;
					newcat->parent = parent->key;
					newcat->name = g_strdup(partstr[1]);
					newcat->flags |= GF_SUB;
					//#1713413 take parent type into account
					if(parent->flags & GF_INCOME)
						newcat->flags |= GF_INCOME;
					da_cat_insert(newcat);
				}
				retval = newcat;
			}

			g_strfreev(partstr);
		}
	}

	return retval;
}


/**
 * da_cat_get:
 *
 * Get a category structure by key
 *
 * Return value: Category * or NULL if not found
 *
 */
Category *
da_cat_get(guint32 key)
{
	//DB( g_print("da_cat_get\n") );

	return g_hash_table_lookup(GLOBALS->h_cat, &key);
}


gchar *da_cat_get_name(Category *item)
{
gchar *name = NULL;

	if(item != NULL)
	{
		name = item->key == 0 ? _("(no category)") : item->fullname;
	}
	return name;
}


void da_cat_consistency(Category *item)
{
//gboolean isIncome;

	if((item->flags & GF_SUB) && item->key > 0)
	{
		//check for existing parent
		if( da_cat_get(item->parent) == NULL )
		{
		Category *parent = da_cat_append_ifnew_by_fullname ("orphaned");

			item->parent = parent->key;
			da_cat_build_fullname(item);
			g_warning("category consistency: fixed missing parent %d", item->parent);
		}
	}

	// ensure type equal for categories and its children
	/* no since #1740368
	if(!(item->flags & GF_SUB) && item->key > 0)
	{
		isIncome = (item->flags & GF_INCOME) ? TRUE : FALSE;
		if( category_change_type(item, isIncome) > 0 )
		{
			g_warning("category consistency: fixed type for child");
			GLOBALS->changes_count++;
		}
	}*/


	if( item->name != NULL )
		g_strstrip(item->name);
	else
	{
		if( da_cat_ensure_name(item) )
		{
			da_cat_build_fullname(item);
			g_warning("category consistency: fixed null name");
			GLOBALS->changes_count++;
		}
	}

}


//#2026641
void da_cat_anonymize(Category *item)
{
	g_free(item->name);
	item->name = g_strdup_printf("category %d", item->key);

	da_cat_build_fullname(item);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

#if MYDEBUG

static void
da_cat_debug_list_ghfunc(gpointer key, gpointer value, gpointer user_data)
{
guint32 *id = key;
Category *cat = value;

	DB( g_print(" %d :: %s (parent=%d\n", *id, cat->name, cat->parent) );

}

static void
da_cat_debug_list(void)
{

	DB( g_print("\n** debug **\n") );

	g_hash_table_foreach(GLOBALS->h_cat, da_cat_debug_list_ghfunc, NULL);

	DB( g_print("\n** end debug **\n") );

}

#endif



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


gboolean
category_key_budget_active(guint32 key)
{
Category *catitem = da_cat_get(key);
gboolean retval = FALSE;

	if( catitem != NULL && catitem->flags & (GF_BUDGET|GF_FORCED) )
		retval = TRUE;

	return retval;
}


gboolean
category_key_unbudgeted(guint32 key)
{
Category *catitem = da_cat_get(key);

	if( catitem == NULL )
		return FALSE;

	//#2101100 exclude subcat from unbudgeted
	if( PREFS->budg_unexclsub == TRUE )
	{
		if( catitem->parent > 0 )
		{
			catitem = da_cat_get(catitem->parent);
			if( catitem == NULL )
				return FALSE;
		}
	}

	return (catitem->flags & (GF_BUDGET|GF_FORCED)) ? FALSE : TRUE;
}


guint32
category_report_id(guint32 key, gboolean subcat)
{
guint32 retval = 0;

	if(subcat == FALSE)
	{
	Category *catentry = da_cat_get(key);
		if(catentry)
			retval = (catentry->flags & GF_SUB) ? catentry->parent : catentry->key;
	}
	else
	{
		retval = key;
	}
	//DB( g_print("- cat '%s' reportid = %d\n", catentry->name, retval) );
	return retval;
}


gint
category_delete_unused(void)
{
GList *lcat, *list;
gint count = 0;

	lcat = list = g_hash_table_get_values(GLOBALS->h_cat);
	while (list != NULL)
	{
	Category *entry = list->data;

		if(entry->nb_use_all <= 0 && entry->key > 0)
		{
			//da_cat_delete (entry->key);
			g_hash_table_remove(GLOBALS->h_cat, &entry->key);
			count++;
		}
		list = g_list_next(list);
	}
	g_list_free(lcat);
	return count;
}


static void
category_fill_usage_count(guint32 kcat, gboolean txn)
{
Category *parent, *cat = da_cat_get (kcat);

	if(cat)
	{
		if(txn == TRUE)
		{
			cat->nb_use_txn++;
			cat->nb_use_txncat++;
		}
		cat->nb_use_all++;
		cat->nb_use_allcat++;
		if( cat->parent > 0 )
		{
			parent = da_cat_get(cat->parent);
			if( parent )
			{
				if(txn == TRUE)
					parent->nb_use_txn++;

				parent->nb_use_all++;
			}
		}
	}
}


void
category_fill_usage(void)
{
GList *lcat;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
GList *lpay, *lrul, *list;
guint i, nbsplit;

	lcat = list = g_hash_table_get_values(GLOBALS->h_cat);
	while (list != NULL)
	{
	Category *entry = list->data;
		entry->nb_use_all = 0;
		entry->nb_use_allcat = 0;
		entry->nb_use_txn = 0;
		entry->nb_use_txncat = 0;
		list = g_list_next(list);
	}
	g_list_free(lcat);


	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			//#1689308 count split as well
			if( txn->flags & OF_SPLIT )
			{
				nbsplit = da_splits_length(txn->splits);
				for(i=0;i<nbsplit;i++)
				{
				Split *split = da_splits_get(txn->splits, i);

					category_fill_usage_count(split->kcat, TRUE);
				}
			}
			else
				category_fill_usage_count(txn->kcat, TRUE);

			lnk_txn = g_list_next(lnk_txn);
		}
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	lpay = list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *entry = list->data;

		category_fill_usage_count(entry->kcat, FALSE);
		list = g_list_next(list);
	}
	g_list_free(lpay);


	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *entry = list->data;

		//#1689308 count split as well
		if( entry->flags & OF_SPLIT )
		{
			nbsplit = da_splits_length(entry->splits);
			for(i=0;i<nbsplit;i++)
			{
			Split *split = da_splits_get(entry->splits, i);

				category_fill_usage_count(split->kcat, FALSE);
			}
		}
		else
			category_fill_usage_count(entry->kcat, FALSE);

		list = g_list_next(list);
	}


	lrul = list = g_hash_table_get_values(GLOBALS->h_rul);
	while (list != NULL)
	{
	Assign *entry = list->data;

		category_fill_usage_count(entry->kcat, FALSE);
		list = g_list_next(list);
	}
	g_list_free(lrul);

}

//#1875070 if srckey is a cat: any subcat should match as well
static gboolean
category_move_match(guint32 eltkey, guint32 srckey, gboolean dosubcat)
{
Category *cat = da_cat_get(eltkey);

	if(cat)
	{
		if(!dosubcat)
		{
			if(cat->key == srckey)
				return TRUE;
		}
		else
		{
			if(cat->key == srckey || cat->parent == srckey)
				return TRUE;
		}
	}
	return FALSE;
}


void
category_move(guint32 srckey, guint32 newkey, gboolean dosubcat)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn, *lpay;
GList *lrul, *list;
guint i, nbsplit;

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txnitem = lnk_txn->data;

			//#1875070
			//if(txn->kcat == srckey)
			if( category_move_match(txnitem->kcat, srckey, dosubcat) )
			{
				txnitem->kcat = newkey;
				txnitem->dspflags |= FLAG_TMP_EDITED;
			}

			// move split category #1340142
			nbsplit = da_splits_length(txnitem->splits);
			for(i=0;i<nbsplit;i++)
			{
			Split *split = da_splits_get(txnitem->splits, i);

				//#1875070
				//if( split->kcat == srckey )
				if( category_move_match(split->kcat, srckey, dosubcat) )
				{
					split->kcat = newkey;
					txnitem->dspflags |= FLAG_TMP_EDITED;
				}
			}

			lnk_txn = g_list_next(lnk_txn);
		}

		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);


	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *arcitem = list->data;

		//#1875070
		//if(entry->kcat == srckey)
		if( category_move_match(arcitem->kcat, srckey, dosubcat) )
		{
			arcitem->kcat = newkey;
		}

		//#2081379 handle split as well
		nbsplit = da_splits_length(arcitem->splits);
		for(i=0;i<nbsplit;i++)
		{
		Split *split = da_splits_get(arcitem->splits, i);

			//#1875070
			//if( split->kcat == srckey )
			if( category_move_match(split->kcat, srckey, dosubcat) )
			{
				split->kcat = newkey;
				arcitem->dspflags |= FLAG_TMP_EDITED;
			}
		}

		list = g_list_next(list);
	}

	//fix 5.4.2 missed payee here
	lpay = list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *payitem = list->data;

		if( category_move_match(payitem->kcat, srckey, dosubcat) )
		{
			payitem->kcat = newkey;
		}
		list = g_list_next(list);
	}
	g_list_free(lpay);


	lrul = list = g_hash_table_get_values(GLOBALS->h_rul);
	while (list != NULL)
	{
	Assign *asgitem = list->data;

		//#1875070
		//if(entry->kcat == srckey)
		if( category_move_match(asgitem->kcat, srckey, dosubcat) )
		{
			asgitem->kcat = newkey;
		}
		list = g_list_next(list);
	}
	g_list_free(lrul);

}


gboolean
category_rename(Category *item, const gchar *newname)
{
Category *parent, *existitem;
gchar *fullname = NULL;
gchar *stripname;
gboolean retval = FALSE;

	DB( g_print("\n(category) rename\n") );

	stripname = g_strdup(newname);
	g_strstrip(stripname);

	if( item->parent == 0)
		fullname = g_strdup(stripname);
	else
	{
		parent = da_cat_get(item->parent);
		if( parent )
		{
			fullname = g_strdup_printf("%s:%s", parent->name, stripname);
		}
	}

	DB( g_print(" - search: %s\n", fullname) );

	existitem = da_cat_get_by_fullname( fullname );

	if( existitem != NULL && existitem->key != item->key)
	{
		DB( g_print("- error, same name already exist with other key %d <> %d\n", existitem->key, item->key) );
	}
	else
	{
		DB( g_print("- renaming\n") );

		da_cat_rename (item, stripname);
		retval = TRUE;
	}

	g_free(fullname);
	g_free(stripname);

	return retval;
}


static gint
category_glist_name_compare_func(Category *c1, Category *c2)
{
gint retval = 0;

	if( c1 != NULL && c2 != NULL )
	{
		retval = hb_string_utf8_compare(c1->fullname, c2->fullname);
	}
	return retval;
}


static gint
category_glist_key_compare_func(Category *a, Category *b)
{
gint ka, kb, retval = 0;

	if(a->parent == 0 && b->parent == a->key)
		retval = -1;
	else
	if(b->parent == 0 && a->parent == b->key)
		retval = 1;
	else
	{
		ka = a->parent != 0 ? a->parent : a->key;
		kb = b->parent != 0 ? b->parent : b->key;
		retval = ka - kb;
	}


	#if MYDEBUG == 1
	gchar *str;

	if(retval < 0)
		str = "a < b";
	else
	if(retval ==0)
		str = "a = b";
	else
	if(retval > 0)
		str = "a > b";

	DB( g_print("compare a=%2d:%2d to b=%2d:%2d :: %d [%s]\n", a->key, a->parent, b->key, b->parent, retval, str ) );
	#endif

	return retval;
}


GList *
category_glist_sorted(gint column)
{
GList *list = g_hash_table_get_values(GLOBALS->h_cat);

	switch(column)
	{
		case HB_GLIST_SORT_NAME:
			return g_list_sort(list, (GCompareFunc)category_glist_name_compare_func);
			break;
		//case HB_GLIST_SORT_KEY:
		default:
			return g_list_sort(list, (GCompareFunc)category_glist_key_compare_func);
			break;
	}
}


gboolean
category_load_csv(gchar *filename, gchar **error)
{
gboolean retval;
GIOChannel *io;
gchar *tmpstr;
gint io_stat;
gchar **str_array;
gchar *lastcatname = NULL;
gchar *fullcatname;
GError *err = NULL;
Category *item;
gboolean isIncome;
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
			if( *error != NULL )
				break;
			io_stat = g_io_channel_read_line(io, &tmpstr, NULL, NULL, &err);

			DB( g_print(" + iostat %d\n", io_stat) );

			if( io_stat == G_IO_STATUS_ERROR )
			{
				DB (g_print(" + ERROR %s\n",err->message));
				break;
			}
			if( io_stat == G_IO_STATUS_EOF)
				break;
			if( io_stat == G_IO_STATUS_NORMAL)
			{
				if( tmpstr != NULL )
				{
					DB( g_print(" + strip %s\n", tmpstr) );
					hb_string_strip_crlf(tmpstr);

					DB( g_print(" + split\n") );
					str_array = g_strsplit (tmpstr, ";", 3);
					// type; sign; name

					if( g_strv_length (str_array) != 3 )
					{
						*error = _("invalid CSV format");
						retval = FALSE;
						DB( g_print(" + error %s\n", *error) );
					}
					else
					{
						DB( g_print(" + read %s : %s : %s\n", str_array[0], str_array[1], str_array[2]) );

						fullcatname = NULL;
						if( g_str_has_prefix(str_array[0], "1") )
						{
							fullcatname = g_strdup(str_array[2]);
							g_free(lastcatname);
							lastcatname = g_strdup(str_array[2]);
						}
						else
						if( g_str_has_prefix(str_array[0], "2") )
						{
							fullcatname = g_strdup_printf("%s:%s", lastcatname, str_array[2]);
						}

						item = da_cat_append_ifnew_by_fullname(fullcatname);
						DB( g_print(" + item %p\n", item) );

						if( item != NULL)
						{
							isIncome = g_str_has_prefix(str_array[1], "+") ? TRUE : FALSE;
							category_change_type(item, isIncome, FALSE);
						}

						g_free(fullcatname);
						g_strfreev (str_array);
					}
				}
			}
			g_free(tmpstr);
		}
		g_io_channel_unref (io);
	}

	g_free(lastcatname);

	return retval;
}


gboolean
category_save_csv(gchar *filename, gchar **error)
{
gboolean retval = FALSE;
GIOChannel *io;
gchar *outstr;
GList *lcat, *list;

	io = g_io_channel_new_file(filename, "w", NULL);
	if(io != NULL)
	{
		lcat = list = category_glist_sorted(HB_GLIST_SORT_NAME);

		while (list != NULL)
		{
		Category *item = list->data;

			if(item->key != 0)
			{
			gchar lvel, type;

				//#1740368 changes here
				type = category_get_type_char(item);
				lvel = ( item->parent == 0 ) ? '1' : '2';

				outstr = g_strdup_printf("%c;%c;%s\n", lvel, type, item->name);

				DB( g_print(" + export %s\n", outstr) );

				g_io_channel_write_chars(io, outstr, -1, NULL, NULL);

				g_free(outstr);
			}
			list = g_list_next(list);
		}

		retval = TRUE;

		g_list_free(lcat);

		g_io_channel_unref (io);
	}

	return retval;
}


// only used to warn in txn dialog
gint
category_type_get(Category *item)
{
	if( (item->flags & (GF_INCOME)) )
		return 1;
	return -1;
}


gint
category_root_type_get(guint32 key)
{
Category *item;

	if(key == 0)
		return 0;
	item = da_cat_get(key);
	if(item == NULL)
		return 0;
	if(item->parent > 0)
		item = da_cat_get(item->parent);
	return category_type_get(item);
}


gchar
category_get_type_char(Category *item)
{
	return (item->flags & GF_INCOME) ? '+' : '-';
}


static gint
category_change_type_eval(Category *item, gboolean isIncome)
{
gboolean flaginc;
gint retval = 0;

	flaginc = (item->flags & (GF_INCOME)) ? TRUE : FALSE;
	if( flaginc != isIncome )
		retval = 1;

	DB( g_print(" change:%d\n", retval) );
	return retval;
}


gint
category_change_type(Category *item, gboolean isIncome, gboolean doChild)
{
GList *lcat, *list;
gint changes = 0;

	DB( g_print("\n[category] category_change_type\n") );

	DB( g_print(" set '%s' inc=%d dochild=%d\n", item->fullname, isIncome, doChild) );

	//flag reset & set income
	changes += category_change_type_eval(item, isIncome);
	item->flags &= ~(GF_INCOME);
	if(isIncome == TRUE)
		item->flags |= GF_INCOME;

	//if item is a subcat we override by its parent for below child processing
	if( item->parent != 0 )
		item = da_cat_get(item->parent);

	if( item && !(item->flags & GF_SUB) )
	{
		item->flags &= ~(GF_MIXED);
		lcat = list = g_hash_table_get_values(GLOBALS->h_cat);
		while (list != NULL)
		{
		Category *child = list->data;

			if(child->parent == item->key)
			{
				// propagate to child
				if( doChild )
				{
					changes += category_change_type_eval(child, isIncome);
					DB( g_print(" set child '%s' %d\n", child->fullname, isIncome) );
					child->flags &= ~(GF_INCOME);	//delete flag
					if(isIncome == TRUE)
						child->flags |= GF_INCOME;
				}
				
				da_cat_build_typename(child);
				
				// set mixed if child has != sign
				if((item->flags & GF_INCOME) != (child->flags & GF_INCOME) )
					item->flags |= GF_MIXED;
			}
			list = g_list_next(list);
		}
		g_list_free(lcat);
	}
	
	da_cat_build_typename(item);

	return changes;
}


/**
 * category_find_preset:
 *
 * find a user language compatible file for category preset
 *
 * Return value: a pathname to the file or NULL
 *
 */
gchar *
category_find_preset(gchar **lang)
{
gchar **langs;
gchar *filename;
gboolean exists;
guint i;

	DB( g_print("** category_find_preset **\n") );

	langs = (gchar **)g_get_language_names ();

	DB( g_print(" -> %d languages detected\n", g_strv_length(langs)) );

	for(i=0;i<g_strv_length(langs);i++)
	{
		DB( g_print(" -> %d '%s'\n", i, langs[i]) );
		filename = g_strdup_printf("hb-categories-%s.csv", langs[i]);
		gchar *pathfilename = g_build_filename(homebank_app_get_datas_dir(), filename, NULL);
		exists = g_file_test(pathfilename, G_FILE_TEST_EXISTS);
		DB( g_print(" -> '%s' exists=%d\n", pathfilename, exists) );
		if(exists)
		{
			g_free(filename);
			*lang = langs[i];
			return pathfilename;
		}
		g_free(filename);
		g_free(pathfilename);
	}

	DB( g_print("return NULL\n") );

	*lang = NULL;
	return NULL;
}

