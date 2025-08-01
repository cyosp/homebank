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
#include "hb-account.h"

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


/* = = = = = = = = = = = = = = = = */


void
da_acc_free(Account *item)
{
	DB( g_print("da_acc_free\n") );
	if(item != NULL)
	{
		DB( g_print(" => %d, %s\n", item->key, item->name) );

		g_free(item->name);
		g_free(item->number);
		g_free(item->bankname);
		g_free(item->notes);
		g_free(item->website);
		
		g_free(item->xferexpname);
		g_free(item->xferincname);

		g_queue_free (item->txn_queue);
		
		g_free(item);
	}
}


Account *
da_acc_malloc(void)
{
Account *item;

	DB( g_print("da_acc_malloc\n") );
	item = g_malloc0(sizeof(Account));
	item->kcur = GLOBALS->kcur;
	item->txn_queue = g_queue_new ();
	return item;
}


void
da_acc_destroy(void)
{
	DB( g_print("da_acc_destroy\n") );
	g_hash_table_destroy(GLOBALS->h_acc);
}


void
da_acc_new(void)
{
	DB( g_print("da_acc_new\n") );
	GLOBALS->h_acc = g_hash_table_new_full(g_int_hash, g_int_equal, (GDestroyNotify)g_free, (GDestroyNotify)da_acc_free);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


/**
 * da_acc_length:
 *
 * Return value: the number of elements
 */
guint
da_acc_length(void)
{
	return g_hash_table_size(GLOBALS->h_acc);
}


static void da_acc_max_key_ghfunc(gpointer key, Account *item, guint32 *max_key)
{
	*max_key = MAX(*max_key, item->key);
}


/**
 * da_acc_get_max_key:
 *
 * Get the biggest key from the GHashTable
 *
 * Return value: the biggest key value
 *
 */
guint32
da_acc_get_max_key(void)
{
guint32 max_key = 0;

	g_hash_table_foreach(GLOBALS->h_acc, (GHFunc)da_acc_max_key_ghfunc, &max_key);
	return max_key;
}


/**
 * da_acc_remove:
 *
 * delete an account from the GHashTable
 *
 * Return value: TRUE if the key was found and deleted
 *
 */
gboolean
da_acc_delete(guint32 key)
{
	DB( g_print("da_acc_remove %d\n", key) );

	return g_hash_table_remove(GLOBALS->h_acc, &key);
}


static void
da_acc_build_xfername(Account *item)
{

	g_free(item->xferexpname);
	g_free(item->xferincname);
	
	item->xferexpname = g_strdup_printf("> %s", item->name);
	item->xferincname = g_strdup_printf("< %s", item->name);

	DB( g_print("- updated %d:'%s' xferexpname='%s' xferincname='%s'\n", item->key, item->name, item->xferexpname, item->xferincname) );
}


//#1889659: ensure name != null/empty
static gboolean
da_acc_ensure_name(Account *item)
{
	// (no account) do not exists
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
da_acc_rename(Account *item, gchar *newname)
{

	DB( g_print("- renaming '%s' => '%s'\n", item->name, newname) );

	g_free(item->name);
	item->name = g_strdup(newname);

	//#1889659: ensure name != null/empty
	da_acc_ensure_name(item);
	da_acc_build_xfername(item);

}


/**
 * da_acc_insert:
 *
 * insert an account into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean
da_acc_insert(Account *item)
{
guint32 *new_key;

	DB( g_print("da_acc_insert\n") );

	new_key = g_new0(guint32, 1);
	*new_key = item->key;
	
	//#1889659: ensure name != null/empty
	da_acc_ensure_name(item);
	da_acc_build_xfername(item);

	g_hash_table_insert(GLOBALS->h_acc, new_key, item);

	return TRUE;
}


/**
 * da_acc_append:
 *
 * insert an account into the GHashTable
 *
 * Return value: TRUE if inserted
 *
 */
gboolean
da_acc_append(Account *item)
{
Account *existitem;

	DB( g_print("da_acc_append\n") );

	existitem = da_acc_get_by_name( item->name );
	if( existitem == NULL )
	{
		item->key = da_acc_get_max_key() + 1;
		item->pos = da_acc_length() + 1;
		da_acc_insert(item);
		return TRUE;
	}

	DB( g_print(" -> %s already exist: %d\n", item->name, item->key) );

	return FALSE;
}


static gboolean da_acc_name_grfunc(gpointer key, Account *item, gchar *name)
{
	if( name && item->name )
	{
		if(!strcasecmp(name, item->name))
			return TRUE;
	}
	return FALSE;
}

/**
 * da_acc_get_by_name:
 *
 * Get an account structure by its name
 *
 * Return value: Account * or NULL if not found
 *
 */
Account *
da_acc_get_by_name(gchar *rawname)
{
Account *retval = NULL;
gchar *stripname;

	DB( g_print("da_acc_get_by_name\n") );

	if( rawname )
	{
		stripname = g_strdup(rawname);
		g_strstrip(stripname);
		if( strlen(stripname) > 0 )
			retval = g_hash_table_find(GLOBALS->h_acc, (GHRFunc)da_acc_name_grfunc, stripname);

		g_free(stripname);
	}

	return retval; 
}


/**
 * da_acc_get:
 *
 * Get an account structure by key
 *
 * Return value: Account * or NULL if not found
 *
 */
Account *
da_acc_get(guint32 key)
{
	//DB( g_print("da_acc_get\n") );

	return g_hash_table_lookup(GLOBALS->h_acc, &key);
}


static gint da_acc_glist_compare_pos_func(Account *a, Account *b) { return ((gint)a->pos - b->pos); }

guint32
da_acc_get_first_key(void)
{
GList *lacc, *list;

guint32 retval = 0;

	list = g_hash_table_get_values(GLOBALS->h_acc);
	lacc = list = g_list_sort(list, (GCompareFunc)da_acc_glist_compare_pos_func);
	if( list != NULL )
	{
	Account *accitem = list->data;
		retval = accitem->key;
	}
	g_list_free(lacc);
	return retval;
}


void da_acc_consistency(Account *item)
{
	g_strstrip(item->name);
	
	//#1889659: ensure name != null/empty
	da_acc_ensure_name(item);
}


//#2026641
void da_acc_anonymize(Account *item)
{
	g_free(item->name);
	item->name = g_strdup_printf("account %d", item->key);

	g_free(item->number);
	item->number = NULL;

	g_free(item->bankname);
	item->bankname = NULL;

	//#2026641 account notes, start balance, overdraft
	g_free(item->notes);
	item->notes = NULL;

	item->initial = 0.0;
	item->minimum = 0.0;

	da_acc_build_xfername(item);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
#if MYDEBUG

static void
da_acc_debug_list_ghfunc(gpointer key, gpointer value, gpointer user_data)
{
guint32 *id = key;
Account *item = value;

	DB( g_print(" %d :: %s\n", *id, item->name) );

}

static void
da_acc_debug_list(void)
{

	DB( g_print("\n** debug **\n") );

	g_hash_table_foreach(GLOBALS->h_acc, da_acc_debug_list_ghfunc, NULL);

	DB( g_print("\n** end debug **\n") );

}

#endif


static gint
account_glist_name_compare_func(Account *a, Account *b)
{
	return hb_string_utf8_compare(a->name, b->name);
}

static gint
account_glist_key_compare_func(Account *a, Account *b)
{
	return a->key - b->key;
}

static gint
account_glist_pos_compare_func(Account *a, Account *b)
{
	return a->pos - b->pos;
}


GList *account_glist_sorted(gint column)
{
GList *list = g_hash_table_get_values(GLOBALS->h_acc);

	switch(column)
	{
		case HB_GLIST_SORT_POS:
			return g_list_sort(list, (GCompareFunc)account_glist_pos_compare_func);
			break;
		case HB_GLIST_SORT_NAME:
			return g_list_sort(list, (GCompareFunc)account_glist_name_compare_func);
			break;
		//case HB_GLIST_SORT_KEY:
		default:
			return g_list_sort(list, (GCompareFunc)account_glist_key_compare_func);
			break;
	}
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

void
account_transaction_sort(void)
{
GList *lst_acc, *lnk_acc;

	DB( g_print("\n[account] transaction sort\n") );

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;
	
		da_transaction_queue_sort(acc->txn_queue);
		lnk_acc = g_list_next(lnk_acc);
	}

	g_list_free(lst_acc);
}


/**
 * account_is_used:
 *
 * controls if an account is used by any archive or transaction
 *
 * Return value: TRUE if used, FALSE, otherwise
 */
guint
account_is_used(guint32 key)
{
Account *acc;
GList *list;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
guint retval;

	DB( g_print("\n[account] is used\n") );

	retval = ACC_USAGE_NONE;
	lst_acc = NULL;

	acc = da_acc_get(key);
	if( g_queue_get_length(acc->txn_queue) > 0 )
	{
		retval = ACC_USAGE_TXN;
		goto end;
	}

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;
	
		if(acc->key != key)
		{
			lnk_txn = g_queue_peek_head_link(acc->txn_queue);
			while (lnk_txn != NULL)
			{
			Transaction *entry = lnk_txn->data;
			
				if(key == entry->kxferacc)
				{
					retval = ACC_USAGE_TXN_XFER;
					goto end;
				}

				lnk_txn = g_list_next(lnk_txn);
			}
		}
		lnk_acc = g_list_next(lnk_acc);
	}

	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *entry = list->data;

		if(key == entry->kacc) 
		{
			retval = ACC_USAGE_ARC;
			goto end;
		}
		
		if(key == entry->kxferacc)
		{
			retval = ACC_USAGE_ARC_XFER;
			goto end;
		}

		list = g_list_next(list);
	}

end:
	g_list_free(lst_acc);

	return retval;
}


gboolean
account_has_website(Account *item)
{
gboolean retval = FALSE;

	if( item != NULL && item->website != NULL )
	{
		//TODO: reinforce controls here
		if( strlen(item->website) > 4 )
			retval = TRUE;
	}
	return retval;
}


void
account_set_dirty(Account *acc, guint32 key, gboolean isdirty)
{
	if( acc == NULL )
		acc = da_acc_get(key);

	if( acc != NULL )
	{
		if( isdirty )
			acc->dspflags |= FLAG_ACC_TMP_DIRTY;
		else
			acc->dspflags &= ~(FLAG_ACC_TMP_DIRTY);
	}
}


gboolean
account_exists(gchar *name)
{
Account *existitem;
gchar *stripname;

	stripname = g_strdup(name);
	g_strstrip(stripname);

	existitem = da_acc_get_by_name(stripname);

	g_free(stripname);

	return existitem == NULL ? FALSE : TRUE;
}


gboolean
account_rename(Account *item, gchar *newname)
{
Account *existitem;
gchar *stripname;
gboolean retval = FALSE;

	DB( g_print("\n[account] rename\n") );

	stripname = g_strdup(newname);
	g_strstrip(stripname);

	if( strlen(stripname) > 0 )
	{
		existitem = da_acc_get_by_name(stripname);
		
		//#2083124 enable case renaming
		if( existitem != NULL && existitem->key != item->key )
		{
			DB( g_print("- error, same name already exist with other key %d <> %d\n", existitem->key, item->key) );
		}
		else
		{
			DB( g_print("- renaming\n") );

			da_acc_rename (item, stripname);
			retval = TRUE;
		}
	}

	g_free(stripname);
	
	return retval;
}


/* 
 * change the account currency
 * change every txn to currency
 * #2026594 no more change target currency
 * #1673260 internal transfer with different currency
 *  => no more ensure dst xfer transaction account will be set to same currency
 */
void account_set_currency(Account *acc, guint32 kcur)
{
GList *list;
/*Account *dstacc;
gboolean *xfer_list;
guint32 maxkey, i;*/

	DB( g_print("\n[account] set currency\n") );

	if(acc->kcur == kcur)
	{
		DB( g_print(" - already ok, return\n") );
		return;
	}

	DB( g_print(" - set for '%s'\n", acc->name)  );

	//#1673260 internal transfer with different currency
	/*maxkey = da_acc_get_max_key () + 1;
	xfer_list = g_malloc0(sizeof(gboolean) * maxkey );
	DB( g_print(" - alloc for %d account\n", da_acc_length() ) );*/

	list = g_queue_peek_head_link(acc->txn_queue);
	while (list != NULL)
	{
	Transaction *txn = list->data;

		txn->kcur = kcur;
		/*if( (txn->flags & OF_INTXFER) && (txn->kxferacc > 0) && (txn->kxfer > 0) )
		{
			xfer_list[txn->kxferacc] = TRUE;
		}*/
		list = g_list_next(list);
	}

	acc->kcur = kcur;
	DB( g_print(" - '%s'\n", acc->name) );
	
	//#1673260 internal transfer with different currency
	/*for(i=1;i<maxkey;i++)
	{
		DB( g_print(" - %d '%d'\n", i, xfer_list[i]) );
		if( xfer_list[i] == TRUE )
		{
			dstacc = da_acc_get(i);
			account_set_currency(dstacc, kcur);
		}
	}

	g_free(xfer_list);*/

}


/**
 * private function to sub transaction amount from account balances
 */
static void account_balances_sub_internal(Account *acc, Transaction *txn)
{
	acc->bal_future -= txn->amount;

	if(txn->date <= GLOBALS->today)
		acc->bal_today -= txn->amount;

	if(txn->status == TXN_STATUS_CLEARED)
		acc->bal_clear -= txn->amount;

	if(txn->status == TXN_STATUS_RECONCILED)
	{
		acc->bal_recon -= txn->amount;
		acc->bal_clear -= txn->amount;
	}
}

/**
 * private function to add transaction amount from account balances
 */
static void account_balances_add_internal(Account *acc, Transaction *txn)
{
	acc->bal_future += txn->amount;

	if(txn->date <= GLOBALS->today)
		acc->bal_today += txn->amount;

	if(txn->status == TXN_STATUS_CLEARED)
		acc->bal_clear += txn->amount;

	if(txn->status == TXN_STATUS_RECONCILED)
	{
		acc->bal_recon += txn->amount;
		acc->bal_clear += txn->amount;
	}
}


/**
 * public function to sub transaction amount from account balances
 */
gboolean account_balances_sub(Transaction *txn)
{

	if( transaction_is_balanceable(txn) )
	//if(!(txn->flags & OF_REMIND))
	{
		Account *acc = da_acc_get(txn->kacc);
		if(acc == NULL) return FALSE;
		account_balances_sub_internal(acc, txn);
		return TRUE;
	}
	return FALSE;
}


/**
 * public function to add transaction amount from account balances
 */
gboolean account_balances_add(Transaction *txn)
{
	if( transaction_is_balanceable(txn) )
	//if(!(txn->flags & OF_REMIND))
	{
		Account *acc = da_acc_get(txn->kacc);
		if(acc == NULL) return FALSE;
		account_balances_add_internal(acc, txn);
		return TRUE;
	}
	return FALSE;
}


void account_flags_eval(Account *item)
{
	g_return_if_fail(item != NULL);

	//#2109854 fix residual flag
	item->flags &= ~(AF_HASNOTICE);
	if( item->nb_pending > 0 )
	{
		item->flags |= AF_HASNOTICE;
	}
}


//todo: optim called 2 times from dsp_mainwindow
void account_compute_balances(gboolean init)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;

	DB( g_print("\naccount_compute_balances start\n") );

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;
	
		acc->nb_pending = 0;
		/* set initial amount */
		acc->bal_clear = acc->initial;
		acc->bal_recon = acc->initial;
		acc->bal_today = acc->initial;
		acc->bal_future = acc->initial;
		
		/* add every txn */
		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;
		
			if( transaction_is_balanceable(txn) )
			{
				account_balances_add_internal(acc, txn);
			}
			
			//5.9 moved completion memo here
			if( (init == TRUE) && (PREFS->txn_memoacp == TRUE) )
			{
				da_transaction_insert_memos(txn);
			}

			//5.9 add flags
			if( txn->flags & (OF_ISPAST | OF_ISIMPORT) )
			{
				acc->nb_pending++;
			}

			lnk_txn = g_list_next(lnk_txn);
		}

		account_flags_eval(acc);

		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	DB( g_print("\naccount_compute_balances end\n") );

}


void account_convert_euro(Account *acc)
{
GList *lnk_txn;

	//5.9: ignore already EUR account
	if( currency_is_euro(acc->kcur) == TRUE )
		return;

	lnk_txn = g_queue_peek_head_link(acc->txn_queue);
	while (lnk_txn != NULL)
	{
	Transaction *txn = lnk_txn->data;
	gdouble oldamount = txn->amount;

		txn->amount = hb_amount_to_euro(oldamount);
		DB( g_print("%10.6f => %10.6f, %s\n", oldamount, txn->amount, txn->memo) );
		//todo: sync child xfer
		lnk_txn = g_list_next(lnk_txn);
	}

	acc->initial = hb_amount_to_euro(acc->initial);
//	acc->warning = hb_amount_to_euro(acc->warning);
	acc->minimum = hb_amount_to_euro(acc->minimum);
	acc->maximum = hb_amount_to_euro(acc->maximum);

}

