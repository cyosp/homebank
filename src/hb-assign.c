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
#include "hb-assign.h"

#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void
da_asg_free(Assign *item)
{
	DB( g_print("da_asg_free\n") );
	if(item != NULL)
	{
		DB( g_print(" => %d, %s\n", item->key, item->search) );

		g_free(item->search);
		g_free(item->notes);
		g_free(item);
	}
}


Assign *
da_asg_malloc(void)
{
	DB( g_print("da_asg_malloc\n") );
	return g_malloc0(sizeof(Assign));
}


void
da_asg_destroy(void)
{
	DB( g_print("da_asg_destroy\n") );
	g_hash_table_destroy(GLOBALS->h_rul);
}


void
da_asg_new(void)
{
	DB( g_print("da_asg_new\n") );
	GLOBALS->h_rul = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)da_asg_free);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
static void da_asg_max_key_ghfunc(gpointer key, Assign *item, guint32 *max_key)
{
	*max_key = MAX(*max_key, item->key);
}

static gboolean da_asg_name_grfunc(gpointer key, Assign *item, gchar *name)
{
	if( name && item->search )
	{
		if(!strcasecmp(name, item->search))
			return TRUE;
	}
	return FALSE;
}

/**
 * da_asg_length:
 *
 * Return value: the number of elements
 */
guint
da_asg_length(void)
{
	return g_hash_table_size(GLOBALS->h_rul);
}

/**
 * da_asg_remove:
 *
 * delete an rul from the GHashTable
 *
 * Return value: TRUE if the key was found and deleted
 *
 */
gboolean
da_asg_remove(guint32 key)
{
	DB( g_print("da_asg_remove %d\n", key) );

	return g_hash_table_remove(GLOBALS->h_rul, &key);
}


//#1889659: ensure name != null/empty
static gboolean
da_asg_ensure_name(Assign *item)
{
	// (no assign) does not exists
	if( item->key > 0 )
	{
		if( item->search == NULL || strlen(item->search) == 0 )
		{
			g_free(item->search);
			item->search = g_strdup_printf("no name %d", item->key);
			return TRUE;
		}
	}
	return FALSE;
}


/**
 * da_asg_insert:
 *
 * insert an rul into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean
da_asg_insert(Assign *item)
{
guint32 *new_key;

	DB( g_print("da_asg_insert\n") );

	new_key = g_new0(guint32, 1);
	*new_key = item->key;
	
	//#1889659: ensure name != null/empty
	da_asg_ensure_name(item);
	
	g_hash_table_insert(GLOBALS->h_rul, new_key, item);

	return TRUE;
}


/**
 * da_asg_append:
 *
 * append a new rul into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean
da_asg_append(Assign *item)
{
Assign *existitem;
guint32 *new_key;

	DB( g_print("da_asg_append\n") );

	DB( g_print(" -> try append: %s\n", item->search) );

	if( item->search != NULL )
	{
		/* ensure no duplicate */
		existitem = da_asg_get_by_name( item->search );
		if( existitem == NULL )
		{
			new_key = g_new0(guint32, 1);
			*new_key = da_asg_get_max_key() + 1;
			item->key = *new_key;
			//added 5.3.3
			item->pos = da_asg_length() + 1;
			
			DB( g_print(" -> append id: %d\n", *new_key) );

			g_hash_table_insert(GLOBALS->h_rul, new_key, item);
			return TRUE;
		}
	}

	DB( g_print(" -> %s already exist: %d\n", item->search, item->key) );

	return FALSE;
}

/**
 * da_asg_get_max_key:
 *
 * Get the biggest key from the GHashTable
 *
 * Return value: the biggest key value
 *
 */
guint32
da_asg_get_max_key(void)
{
guint32 max_key = 0;

	g_hash_table_foreach(GLOBALS->h_rul, (GHFunc)da_asg_max_key_ghfunc, &max_key);
	return max_key;
}




/**
 * da_asg_get_by_name:
 *
 * Get an rul structure by its name
 *
 * Return value: rul * or NULL if not found
 *
 */
Assign *
da_asg_get_by_name(gchar *name)
{
	DB( g_print("da_asg_get_by_name\n") );

	return g_hash_table_find(GLOBALS->h_rul, (GHRFunc)da_asg_name_grfunc, name);
}



/**
 * da_asg_get:
 *
 * Get an rul structure by key
 *
 * Return value: rul * or NULL if not found
 *
 */
Assign *
da_asg_get(guint32 key)
{
	DB( g_print("da_asg_get_rul\n") );

	return g_hash_table_lookup(GLOBALS->h_rul, &key);
}


void da_asg_consistency(Assign *item)
{
	//5.2.4 we drop internal xfer here as it will disapear
	//was not possible, but just in case
	if( item->paymode == OLDPAYMODE_INTXFER )
		item->paymode = PAYMODE_XFER;
}


/* = = = = = = = = = = = = = = = = = = = = */


Assign *da_asg_init_from_transaction(Assign *asg, Transaction *txn)
{
	DB( g_print("\n[scheduled] init from txn\n") );

	asg->search = g_strdup_printf("%s %s", _("**PREFILLED**"), txn->memo );
	asg->flags |= (ASGF_DOPAY|ASGF_DOCAT|ASGF_DOMOD);
	asg->kcat = txn->kcat;

	if(!(txn->flags & OF_INTXFER))
	{
		asg->kpay = txn->kpay;
		asg->paymode = txn->paymode;
	}
	return asg;
}


void da_asg_update_position(void)
{
GList *lrul, *list;
guint32 newpos = 1;

	DB( g_print("da_asg_update_position\n") );

	lrul = list = assign_glist_sorted(HB_GLIST_SORT_POS);
	while (list != NULL)
	{
	Assign *item = list->data;

		item->pos = newpos++;
		list = g_list_next(list);
	}
	g_list_free(lrul);
}


gchar *assign_get_target_payee(Assign *asgitem)
{
gchar *retval = NULL;

	if( asgitem && (asgitem->flags & (ASGF_DOPAY|ASGF_OVWPAY)) )
	{
	Payee *pay = da_pay_get(asgitem->kpay);

		if(pay != NULL)
			retval = pay->name;
	}

	return retval;
}


gchar *assign_get_target_category(Assign *asgitem)
{
gchar *retval = NULL;

	if( asgitem && (asgitem->flags & (ASGF_DOCAT|ASGF_OVWCAT)) )
	{
	Category *cat = da_cat_get(asgitem->kcat);

		if(cat != NULL)
			retval = cat->fullname;
	}

	return retval;
}



static gint
assign_glist_pos_compare_func(Assign *a, Assign *b)
{
	return a->pos - b->pos;
}


static gint
assign_glist_key_compare_func(Assign *a, Assign *b)
{
	return a->key - b->key;
}


GList *assign_glist_sorted(gint column)
{
GList *list = g_hash_table_get_values(GLOBALS->h_rul);

	switch(column)
	{
		case HB_GLIST_SORT_POS:
			return g_list_sort(list, (GCompareFunc)assign_glist_pos_compare_func);
			break;
		default:
			return g_list_sort(list, (GCompareFunc)assign_glist_key_compare_func);
			break;
	}
}


static gboolean misc_text_match(gchar *text, gchar *searchtext, gboolean exact)
{
gboolean match = FALSE;

	if(text == NULL)
		return FALSE;
	
	//DB( g_print("search %s in %s\n", rul->name, ope->memo) );
	if( searchtext != NULL )
	{
		if( exact == TRUE )
		{
			if( g_strrstr(text, searchtext) != NULL )
			{
				DB( g_print("-- found case '%s'\n", searchtext) );
				match = TRUE;
			}
		}
		else
		{
		gchar *word   = g_utf8_casefold(text, -1);
		gchar *needle = g_utf8_casefold(searchtext, -1);

			if( g_strrstr(word, needle) != NULL )
			{
				DB( g_print("-- found nocase '%s'\n", searchtext) );
				match = TRUE;
			}
			g_free(word);
			g_free(needle);
		}
	}

	return match;
}

static gboolean misc_regex_match(gchar *text, gchar *searchtext, gboolean exact)
{
gboolean match = FALSE;

	if(text == NULL)
		return FALSE;
	
	DB( g_print("-- match RE %s in %s\n", searchtext, text) );
	if( searchtext != NULL )
	{
		match = g_regex_match_simple(searchtext, text, ((exact == TRUE)?0:G_REGEX_CASELESS) | G_REGEX_OPTIMIZE, G_REGEX_MATCH_NOTEMPTY );
		if (match == TRUE) { DB( g_print("-- found pattern '%s'\n", searchtext) ); }
	}
	return match;
}


//#1710085 assignment based on amount 
static gboolean transaction_auto_assign_rule_match(Assign *rul, gchar *text, gdouble amount)
{
gboolean match1, match2;

	match1 = TRUE;
	match2 = FALSE;
	if( rul->flags & ASGF_AMOUNT )
	{
		if( amount != rul->amount )
			match1 = FALSE;
	}

	if( !(rul->flags & ASGF_REGEX) )
	{
		if( misc_text_match(text, rul->search, rul->flags & ASGF_EXACT) )
			match2 = TRUE;
	}
	else
	{
		if( misc_regex_match(text, rul->search, rul->flags & ASGF_EXACT) )
			match2 = TRUE;
	}

	return ((match1==TRUE) && (match2==TRUE)) ? TRUE : FALSE;
}


static GList *transaction_auto_assign_eval_txn(GList *l_rul, Transaction *txn)
{
GList *ret_list = NULL;
GList *list;
gchar *text = NULL;

	list = g_list_first(l_rul);
	while (list != NULL)
	{
	Assign *rul = list->data;

		text = txn->memo;
		if(rul->field == 1) //payee
		{
		Payee *pay = da_pay_get(txn->kpay);
			if(pay)
				text = pay->name;
		}
		
		if( transaction_auto_assign_rule_match(rul, text, txn->amount) == TRUE )
		{
			//TODO: perf must use preprend, see glib doc
			ret_list = g_list_append(ret_list, rul);
		}

		list = g_list_next(list);
	}

	DB( g_print("- %d rule(s) match on '%s'\n", g_list_length (ret_list), text) );
	
	return ret_list;
}


static GList *transaction_auto_assign_eval_split(GList *l_rul, gchar *text, gdouble amount)
{
GList *ret_list = NULL;
GList *list;

	list = g_list_first(l_rul);
	while (list != NULL)
	{
	Assign *rul = list->data;

		if( rul->field == 0 )   //memo
		{
			if( transaction_auto_assign_rule_match(rul, text, amount) == TRUE )
			{
				//TODO: perf must use preprend, see glib doc
				ret_list = g_list_append(ret_list, rul);
			}
		}
		list = g_list_next(list);
	}

	DB( g_print("- %d rule(s) match on '%s'\n", g_list_length (ret_list), text) );
	
	return ret_list;
}


guint transaction_auto_assign(GList *ope_list, guint32 kacc, gboolean lockrecon)
{
GList *l_ope;
GList *l_rul;
GList *l_match, *l_tmp;
guint changes = 0;

	DB( g_print("\n[transaction] auto_assign\n") );

	l_rul = assign_glist_sorted(HB_GLIST_SORT_POS);

	l_ope = g_list_first(ope_list);
	while (l_ope != NULL)
	{
	Transaction *ope = l_ope->data;
	gboolean changed = FALSE; 

		//#1909749 skip reconciled if lock is ON
		if( lockrecon && ope->status == TXN_STATUS_RECONCILED )
			goto next;

		DB( g_print("\n- curr txn '%s' : acc=%d, pay=%d, cat=%d, %s\n", ope->memo, ope->kacc, ope->kpay, ope->kcat, (ope->flags & OF_SPLIT) ? "is_split" : "" ) );

		//#1215521: added kacc == 0
		if( (kacc == ope->kacc || kacc == 0) )
		{
			if( !(ope->flags & OF_SPLIT) )
			{
				l_match = l_tmp = transaction_auto_assign_eval_txn(l_rul, ope);
				while( l_tmp != NULL )
				{
				Assign *rul = l_tmp->data;
					
					if( (ope->kpay == 0 && (rul->flags & ASGF_DOPAY)) || (rul->flags & ASGF_OVWPAY) )
					{
						if(ope->kpay != rul->kpay) { changed = TRUE; }
						ope->kpay = rul->kpay;
					}

					if( (ope->kcat == 0 && (rul->flags & ASGF_DOCAT)) || (rul->flags & ASGF_OVWCAT) )
					{
						if(ope->kcat != rul->kcat) { changed = TRUE; }
						ope->kcat = rul->kcat;
					}

					if( (ope->paymode == 0 && (rul->flags & ASGF_DOMOD)) || (rul->flags & ASGF_OVWMOD) )
					{
						//ugly hack - don't allow modify intxfer
						if( !(ope->flags & OF_INTXFER) )
						{
							if(ope->paymode != rul->paymode) { changed = TRUE; }
							ope->paymode = rul->paymode;
						}
					}
					l_tmp = g_list_next(l_tmp);
				}
				g_list_free(l_match);
			}
			else
			{
			guint i, nbsplit = da_splits_length(ope->splits);
				
				for(i=0;i<nbsplit;i++)
				{
				Split *split = da_splits_get(ope->splits, i);
					
					DB( g_print("- eval split '%s'\n", split->memo) );

					l_match = l_tmp = transaction_auto_assign_eval_split(l_rul, split->memo, split->amount);
					while( l_tmp != NULL )
					{
					Assign *rul = l_tmp->data;

						//#1501144: check if user wants to set category in rule
						if( (split->kcat == 0 || (rul->flags & ASGF_OVWCAT)) && (rul->flags & ASGF_DOCAT) )
						{
							if(split->kcat != rul->kcat) { changed = TRUE; }
							split->kcat = rul->kcat;
						}
						l_tmp = g_list_next(l_tmp);
					}	
					g_list_free(l_match);
				}
			}

			if(changed == TRUE)
			{
				ope->flags |= OF_CHANGED;
				changes++;
			}
		}
next:
		l_ope = g_list_next(l_ope);
	}

	g_list_free(l_rul);

	return changes;
}




/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

#if MYDEBUG

static void
da_asg_debug_list_ghfunc(gpointer key, gpointer value, gpointer user_data)
{
guint32 *id = key;
Assign *item = value;

	DB( g_print(" %d :: %s\n", *id, item->search) );

}

static void
da_asg_debug_list(void)
{

	DB( g_print("\n** debug **\n") );

	g_hash_table_foreach(GLOBALS->h_rul, da_asg_debug_list_ghfunc, NULL);

	DB( g_print("\n** end debug **\n") );

}

#endif

