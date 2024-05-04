/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2024 Maxime DOYEN
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

#include "hb-transaction.h"
#include "hb-tag.h"
#include "hb-split.h"

/****************************************************************************/
/* Debug macro								    */
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

static void
da_transaction_clean(Transaction *item)
{
	if(item != NULL)
	{
		if(item->memo != NULL)
		{
			g_free(item->memo);
			item->memo = NULL;
		}
		if(item->info != NULL)
		{
			g_free(item->info);
			item->info = NULL;
		}
		if(item->tags != NULL)
		{
			g_free(item->tags);
			item->tags = NULL;
		}

		if(item->splits != NULL)
		{
			da_split_destroy(item->splits);
			item->splits = NULL;
			item->flags &= ~(OF_SPLIT); //Flag that Splits are cleared
		}
	}
}


void
da_transaction_free(Transaction *item)
{
	if(item != NULL)
	{
		da_transaction_clean(item);
		g_free(item);
	}
}


Transaction *
da_transaction_malloc(void)
{
	return g_malloc0(sizeof(Transaction));
}


Transaction *da_transaction_init(Transaction *txn, guint32 kacc)
{
guint32 date;
	
	DB( g_print("da_transaction_init\n") );

	//#1860309 keep the date when init for add/inherit
	date = txn->date;
	da_transaction_clean(txn);
	memset(txn, 0, sizeof(Transaction));
	txn->date = date;

	//fix: 318733 / 1335285
	if( PREFS->heritdate == FALSE ) 
		txn->date = GLOBALS->today;

	txn->kacc = kacc;
	
	da_transaction_set_default_template(txn);
	
	return txn;
}


Transaction *da_transaction_init_from_template(Transaction *txn, Archive *arc)
{
guint32 date;

	DB( g_print("da_transaction_init_from_template\n") );

	//#5.4.2 date must remains when we set a template, as template has no date
	date = txn->date;
	da_transaction_clean(txn);
	txn->date = date;	

	txn->amount	= arc->amount;
	//#1258344 keep the current account if tpl is empty
	if(arc->kacc)
		txn->kacc	= arc->kacc;
	txn->paymode	= arc->paymode;
	txn->flags		= arc->flags;
	txn->status		= arc->status;
	txn->kpay		= arc->kpay;
	txn->kcat		= arc->kcat;
	txn->kxferacc	= arc->kxferacc;
	//#1673260
	txn->xferamount = arc->xferamount;
	if(arc->memo != NULL)
		txn->memo	    = g_strdup(arc->memo);
	if(arc->info != NULL)
		txn->info	    = g_strdup(arc->info);

	txn->tags = tags_clone(arc->tags);

	txn->splits = da_splits_clone(arc->splits);
	if( da_splits_length (txn->splits) > 0 )
		txn->flags |= OF_SPLIT; //Flag that Splits are active

	return txn;
}


Transaction *da_transaction_set_default_template(Transaction *txn)
{
Account *acc;
Archive *arc;

	DB( g_print("da_transaction_set_default_template\n") );

	acc = da_acc_get(txn->kacc);
	if(acc != NULL && acc->karc > 0)
	{
		arc = da_archive_get(acc->karc);
		if( arc )
		{
			DB( g_print(" - init with default template\n") );
			da_transaction_init_from_template(txn, arc);
		}
	}

	return txn;
}


Transaction *da_transaction_clone(Transaction *src_item)
{
Transaction *new_item = g_memdup(src_item, sizeof(Transaction));

	DB( g_print("da_transaction_clone\n") );

	if(new_item)
	{
		//duplicate the string
		new_item->memo = g_strdup(src_item->memo);
		new_item->info = g_strdup(src_item->info);

		//duplicate tags/splits
		//no g_free here to avoid free the src tags (memdup copied the ptr)
		new_item->tags = tags_clone(src_item->tags);
		
		new_item->splits = da_splits_clone(src_item->splits);
		if( da_splits_length (new_item->splits) > 0 )
			new_item->flags |= OF_SPLIT; //Flag that Splits are active

	}
	return new_item;
}


GList *
da_transaction_new(void)
{
	return NULL;
}


guint
da_transaction_length(void)
{
GList *lst_acc, *lnk_acc;
guint count = 0;

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;
	
		count += g_queue_get_length (acc->txn_queue);
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);
	return count;
}


static void da_transaction_queue_free_ghfunc(Transaction *item, gpointer data)
{
	da_transaction_free (item);
}


void da_transaction_destroy(void)
{
GList *lacc, *list;

	lacc = g_hash_table_get_values(GLOBALS->h_acc);
	list = g_list_first(lacc);
	while (list != NULL)
	{
	Account *acc = list->data;

		g_queue_foreach(acc->txn_queue, (GFunc)da_transaction_queue_free_ghfunc, NULL);
		//txn queue is freed into account
		list = g_list_next(list);
	}
	g_list_free(lacc);
}


// used from register only
static gint da_transaction_compare_datafunc(Transaction *a, Transaction *b, gpointer data)
{
gint retval = (gint)a->date - b->date;

	if(!retval) //#1749457
		retval = a->pos - b->pos;

	return retval;
}


void da_transaction_queue_sort(GQueue *queue)
{
	g_queue_sort(queue, (GCompareDataFunc)da_transaction_compare_datafunc, NULL);
}


static gint da_transaction_compare_func(Transaction *a, Transaction *b)
{
	return ((gint)a->date - b->date);
}


GList *da_transaction_sort(GList *list)
{
	return( g_list_sort(list, (GCompareFunc)da_transaction_compare_func));
}


gboolean da_transaction_insert_memo(gchar *memo, guint32 date)
{
gboolean retval = FALSE;

	if( memo != NULL )
	{
		//# 1673048 add filter on status and date obsolete
		if( (PREFS->txn_memoacp == TRUE) && (date >= (GLOBALS->today - PREFS->txn_memoacp_days)) )
		{
			if( g_hash_table_lookup(GLOBALS->h_memo, memo) == NULL )
			{
				retval = g_hash_table_insert(GLOBALS->h_memo, g_strdup(memo), NULL);
			}
		}
	}
	return retval;
}


gboolean da_transaction_insert_sorted(Transaction *newitem)
{
Account *acc;
GList *lnk_txn;

	acc = da_acc_get(newitem->kacc);
	if(!acc) 
		return FALSE;
	
	lnk_txn = g_queue_peek_tail_link(acc->txn_queue);
	while (lnk_txn != NULL)
	{
	Transaction *item = lnk_txn->data;

		if(item->date <= newitem->date)
			break;
		
		lnk_txn = g_list_previous(lnk_txn);
	}

	// we're at insert point, insert after txn
	g_queue_insert_after(acc->txn_queue, lnk_txn, newitem);

	da_transaction_insert_memo(newitem->memo, newitem->date);
	return TRUE;
}


// nota: this is called only when loading xml file
gboolean da_transaction_prepend(Transaction *item)
{
Account *acc;

	acc = da_acc_get(item->kacc);
	//#1661279
	if(!acc) 
		return FALSE;
	
	item->kcur = acc->kcur;
	g_queue_push_tail(acc->txn_queue, item);
	da_transaction_insert_memo(item->memo, item->date);
	return TRUE;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static guint32
da_transaction_get_max_kxfer(void)
{
GList *lst_acc, *lnk_acc;
GList *list;
guint32 max_key = 0;

	DB( g_print("da_transaction_get_max_kxfer\n") );

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		list = g_queue_peek_head_link(acc->txn_queue);
		while (list != NULL)
		{
		Transaction *item = list->data;

			if( item->flags & OF_INTXFER )
			{
				max_key = MAX(max_key, item->kxfer);
			}
			list = g_list_next(list);
		}
		
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	DB( g_print(" max_key : %d \n", max_key) );

	return max_key;
}


static void da_transaction_goto_orphan(Transaction *txn)
{
const gchar *oatn = "orphaned transactions";
Account *ori_acc, *acc;
gboolean found;

	DB( g_print("\n[transaction] goto orphan\n") );

	g_warning("txn consistency: moving to orphan %d '%s' %.2f", txn->date, txn->memo, txn->amount);

	acc = da_acc_get_by_name((gchar *)oatn);
	if(acc == NULL)
	{
		acc = da_acc_malloc();
		acc->name = g_strdup(oatn);
		da_acc_append(acc);
		DB( g_print(" - created orphan acc %d\n", acc->key) );
	}

	ori_acc = da_acc_get(txn->kacc);
	if( ori_acc )
	{
		found = g_queue_remove(ori_acc->txn_queue, txn);
		DB( g_print(" - found in origin ? %d\n", found) );
		if(found)
		{
			txn->kacc = acc->key;
			da_transaction_insert_sorted (txn);
			DB( g_print("moved txn to %d\n", txn->kacc) );
		}
	}
}


void da_transaction_set_flag(Transaction *item)
{
	DB( g_print("\n[transaction] set flag\n") );

	DB( g_print(" amnt=%f => %f\n", item->amount, item->xferamount) );
	DB( g_print(" kxfer=%d\n", item->kxfer) );
	DB( g_print(" in  :: of_inc is %s\n", item->flags & OF_INCOME ? "set" : "unset" ) ) ;

	//#2002348 no change if zero
	if( item->amount != 0.0 )
	{
		item->flags &= ~(OF_INCOME);
		if( item->amount > 0)
			item->flags |= (OF_INCOME);
	}

	DB( g_print(" out :: of_inc is %s\n", item->flags & OF_INCOME ? "set" : "unset" ) );
}


void da_transaction_consistency(Transaction *item)
{
Account *acc;
Category *cat;
Payee *pay;
guint nbsplit;

	DB( g_print("\n[transaction] consistency\n") );


	DB( g_print(" %d %.2f %s\n", item->date, item->amount, item->memo) );

	// ensure date is between range
	item->date = CLAMP(item->date, HB_MINDATE, HB_MAXDATE);

	// check account exists
	acc = da_acc_get(item->kacc);
	if(acc == NULL)
	{
		g_warning("txn consistency: fixed invalid acc %d", item->kacc);
		da_transaction_goto_orphan(item);
		GLOBALS->changes_count++;
	}

	// check category exists
	cat = da_cat_get(item->kcat);
	if(cat == NULL)
	{
		g_warning("txn consistency: fixed invalid cat %d", item->kcat);
		item->kcat = 0;
		GLOBALS->changes_count++;
	}

	//#1340142 check split category 
	if( item->splits != NULL )
	{
		nbsplit = da_splits_consistency(item->splits);
		//# 1416624 empty category when split
		if(nbsplit > 0 && item->kcat > 0)
		{
			g_warning("txn consistency: fixed invalid cat on split txn");
			item->kcat = 0;
			GLOBALS->changes_count++;
		}
	}
	
	// check payee exists
	pay = da_pay_get(item->kpay);
	if(pay == NULL)
	{
		g_warning("txn consistency: fixed invalid pay %d", item->kpay);
		item->kpay = 0;
		GLOBALS->changes_count++;
	}

	// 5.3: fix split on intxfer
	if( ((item->flags & OF_INTXFER) || (item->paymode == OLDPAYMODE_INTXFER)) && (item->splits != NULL) )
	{
		g_warning("txn consistency: fixed invalid split on xfer");
		item->flags &= ~(OF_INTXFER);
		item->paymode = PAYMODE_XFER;
		item->kxfer = 0;
		item->kxferacc = 0;
	}

	// reset dst acc for non xfer transaction
	if( !((item->flags & OF_INTXFER) || (item->paymode == OLDPAYMODE_INTXFER)) )
	{
		if( (item->kxfer != 0) || (item->kxferacc != 0) )
		{
			g_warning("txn consistency: fixed invalid xfer");
			item->kxfer = 0;
			item->kxferacc = 0;
		}
	}

	// intxfer: check dst account exists
	if( (item->flags & OF_INTXFER) || (item->paymode == OLDPAYMODE_INTXFER) )
	{
	gint tak = item->kxferacc;

		item->kxferacc = ABS(tak);  //I crossed negative here one day
		acc = da_acc_get(item->kxferacc);
		if(acc == NULL)
		{
			g_warning("txn consistency: fixed invalid dst_acc %d", item->kxferacc);
			da_transaction_goto_orphan(item);
			item->kxfer = 0;
			item->paymode = PAYMODE_XFER;
			GLOBALS->changes_count++;
		}
	}

	//#1628678 tags for internal xfer should be checked as well
	//#1787826 intxfer should not have split

	//#1295877 ensure income flag is correctly set
	da_transaction_set_flag(item);

	//#1308745 ensure remind flag unset if reconciled
	//useless since 5.0
	//if( item->flags & OF_VALID )
	//	item->flags &= ~(OF_REMIND);

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* new transfer functions */


Transaction *transaction_xfer_child_new_from_txn(Transaction *stxn)
{
Transaction *child;
Account *acc;

	child = da_transaction_clone(stxn);

	//#1673260 deal with multi currency amount
	if( !(stxn->flags & OF_ADVXFER) )
	{
		child->amount = -stxn->amount;
		//TODO:maybe
		//child->xferamount = 0;
	}
	else
	{
		child->amount = stxn->xferamount;
		child->xferamount = stxn->amount;
	}

	da_transaction_set_flag(child);	// set income/xpense

	//#1268026 #1690555
	if( (child->status == TXN_STATUS_CLEARED) || (child->status == TXN_STATUS_RECONCILED) )
		child->status = TXN_STATUS_NONE;
	//child->flags &= ~(OF_VALID);	// delete reconcile state

	child->kacc = child->kxferacc;
	child->kxferacc = stxn->kacc;

	acc = da_acc_get( child->kacc );
	if( acc != NULL )
		child->kcur = acc->kcur;

	return child;
}



static void transaction_xfer_create_child(Transaction *stxn)
{
Transaction *child;
Account *acc;

	DB( g_print("\n[transaction] xfer_create_child\n") );

	if( stxn->kxferacc > 0 )
	{
		child = transaction_xfer_child_new_from_txn(stxn);

		stxn->flags |= (OF_CHANGED | OF_INTXFER);
		child->flags |= (OF_ADDED | OF_INTXFER);

		/* update acc flags */
		acc = da_acc_get( child->kacc );
		if(acc != NULL)
		{
			acc->flags |= AF_ADDED;

			//strong link
			guint maxkey = da_transaction_get_max_kxfer();
			DB( g_print(" + maxkey is %d\n", maxkey) );

			stxn->kxfer  = maxkey+1;
			child->kxfer = maxkey+1;
			DB( g_print(" + strong link to %d\n", stxn->kxfer) );

			DB( g_print(" + add transfer, %p to acc %d\n", child, acc->key) );
			da_transaction_insert_sorted(child);
			account_balances_add (child);
		}
	}

}


//#1708974 enable different date
//#1987975 only suggest opposite sign amount txn
static gboolean transaction_xfer_child_might(Transaction *stxn, Transaction *dtxn, gushort *matchrate)
{
gboolean retval = FALSE;
gint32 daygap = PREFS->xfer_daygap;
gint rate = 100;

	DB( g_print("\n[transaction] xfer_child_might\n") );

	//paranoia check
	if ( (stxn == dtxn) || (stxn->kacc == dtxn->kacc) )
		return FALSE;

	DB( g_print(" src: %d %d %d %f %d\n", stxn->kcur, stxn->date, stxn->kacc, stxn->amount, stxn->kxfer ) );
	DB( g_print(" dst: %d %d %d %f %d\n", dtxn->kcur, dtxn->date, dtxn->kacc, dtxn->amount, dtxn->kxfer ) );

	//keep only normal dtxn
	if( dtxn->kxfer != 0 )
		return FALSE;

	//#1708974 date allow +/- daygap
	if(	(dtxn->date > (stxn->date + daygap)) && (dtxn->date > (stxn->date - daygap)) )
		return FALSE;

	daygap = (stxn->date - dtxn->date);
	rate -= ABS(daygap);

	if( stxn->kcur == dtxn->kcur )
	{
		//#1987975 only suggest opposite sign amount txn
		if( stxn->amount == -dtxn->amount )
		{
			retval = TRUE;
		}
	}
	//#2047647 cross currency target txn proposal, with 10% gap
	else
	{
	Currency *dstcur;
	gdouble rawamt, minamt, maxamt;

		dstcur = da_cur_get(dtxn->kcur);
		rawamt = -hb_amount_round((stxn->amount * dstcur->rate), dstcur->frac_digits);
		minamt = rawamt * 0.9;
		maxamt = rawamt * 1.1;

		if( (dtxn->amount <= maxamt) && (dtxn->amount >= minamt) )
		{
			rate -= ABS(dtxn->amount - rawamt); 
			DB( g_print(" gap = %f\n", ABS(dtxn->amount - rawamt) / rawamt) );		
			retval = TRUE; 
		}
		DB( g_print(" cross might: raw %f / min %f max %f, dst %f, retval=%d\n", 
			rawamt,	minamt, maxamt, dtxn->amount, retval) );
	}
	
	//other rate
	if( stxn->kpay != dtxn->kpay)
		rate -= 1;
	if( stxn->kcat != dtxn->kcat)
		rate -= 1;
	if( hb_string_compare(stxn->memo, dtxn->memo) != 0 )
		rate -= 1;
	//TODO: maybe add info & tag here	

	rate = CLAMP(rate, 0, 100);
	*matchrate = rate;

	//DB( g_print(" return %d\n", retval) );
	return retval;
}


static GList *transaction_xfer_child_might_list_get(Transaction *ope, guint32 kdstacc)
{
GList *lst_acc, *lnk_acc;
GList *list, *matchlist = NULL;
	
	DB( g_print("\n[transaction] xfer_child_might_list_get\n") );

	DB( g_print(" - kdstacc:%d\n", kdstacc) );

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		if( !(acc->flags & AF_CLOSED) && (acc->key != ope->kacc) && ( (acc->key == kdstacc) || kdstacc == 0 ) )
		{
			list = g_queue_peek_tail_link(acc->txn_queue);
			while (list != NULL)
			{
			Transaction *item = list->data;
	
				// no need to go higher than src txn date - daygap
				if(item->date < (ope->date - PREFS->xfer_daygap))
					break;
		
				if( transaction_xfer_child_might(ope, item, &item->matchrate) == TRUE )
				{
					//DB( g_print(" - match %3d: %d %s %f %d=>%d\n", item->matchrate, item->date, item->memo, item->amount, item->kacc, item->kxferacc) );
					matchlist = g_list_append(matchlist, item);
				}
				list = g_list_previous(list);
			}
		}
		
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	return matchlist;
}


void transaction_xfer_search_or_add_child(GtkWindow *parent, Transaction *ope, guint32 kdstacc)
{
GList *matchlist;
gint count;

	DB( g_print("\n[transaction] xfer_search_or_add_child\n") );

	matchlist = transaction_xfer_child_might_list_get(ope, kdstacc);
	count = g_list_length(matchlist);

	DB( g_print(" - found %d might match, switching\n", count) );

	if( count == 0 )
	{
		//we should create the child
		transaction_xfer_create_child(ope);
	}
	//the user must choose himself
	else
	{
	gint result;
	Transaction *child = NULL;

		result = ui_dialog_transaction_xfer_select_child(parent, ope, matchlist, &child);
		switch( result )
		{
			case HB_RESPONSE_SELECTION:
				//#1827193 in case child is null...
				DB( g_print(" child %p\n", child) );
				if( child != NULL )
					transaction_xfer_change_to_child(ope, child);
				break;
			case HB_RESPONSE_CREATE_NEW:
				transaction_xfer_create_child(ope);
				break;
			default:
				DB( g_print(" add normal txn\n") );
				transaction_xfer_change_to_normal(ope);
				da_transaction_set_flag(ope);				
				break;			
		}
	}

	g_list_free(matchlist);
}


Transaction *transaction_xfer_child_strong_get(Transaction *src)
{
Account *dstacc;
GList *list;

	DB( g_print("\n[transaction] xfer_child_strong_get\n") );

	dstacc = da_acc_get(src->kxferacc);
	if( !dstacc || src->kxfer <= 0 )
		return NULL;

	DB( g_print(" - search: %d %s %f %d=>%d - %d\n", src->date, src->memo, src->amount, src->kacc, src->kxferacc, src->kxfer) );

	list = g_queue_peek_tail_link(dstacc->txn_queue);
	while (list != NULL)
	{
	Transaction *item = list->data;

		//#1252230
		//if( item->paymode == PAYMODE_INTXFER 
		//	&& item->kacc == src->kxferacc
		if( (item->flags & OF_INTXFER) 
		 && (item->kxfer == src->kxfer) 
		 && (item != src) )
		{
			DB( g_print(" - found : %d %s %f %d=>%d - %d\n", item->date, item->memo, item->amount, item->kacc, item->kxferacc, src->kxfer) );
			return item;
		}
		list = g_list_previous(list);
	}
	
	DB( g_print(" - not found...\n") );
	return NULL;
}


void transaction_xfer_change_to_normal(Transaction *ope)
{

	DB( g_print("\n[transaction] xfer_change_to_normal\n") );

	//remove xfer flags
	ope->flags &= ~(OF_INTXFER|OF_ADVXFER);

	ope->kxfer = 0;
	ope->kxferacc = 0;
	ope->xferamount = 0;
}


//TODO: should be static but used in hb_import
void transaction_xfer_change_to_child(Transaction *stxn, Transaction *child)
{
Account *dstacc;

	DB( g_print("\n[transaction] xfer_change_to_child\n") );

	stxn->flags  |= (OF_CHANGED | OF_INTXFER);
	child->flags |= (OF_CHANGED | OF_INTXFER);

	/* update acc flags */
	dstacc = da_acc_get( child->kacc);
	if(dstacc != NULL)
		dstacc->flags |= AF_CHANGED;

	//strong link
	guint maxkey = da_transaction_get_max_kxfer();
	stxn->kxfer  = maxkey+1;
	child->kxfer = maxkey+1;

	stxn->kxferacc  = child->kacc;
	child->kxferacc = stxn->kacc;

	//#1673260 enable different currency
	if(stxn->kcur == child->kcur)
	{
		DB( g_print(" sync amount (==kcur)\n") );
		child->amount = -stxn->amount;
	}
	else
	{
		DB( g_print(" sync amount (!=kcur)\n") );
		//we keep the original child amount
		//child->amount     = stxn->xferamount;
		child->xferamount = stxn->amount;
		child->flags |= (OF_ADVXFER);
	}
}


void transaction_xfer_child_sync(Transaction *s_txn, Transaction *child)
{
Account *acc;

	DB( g_print("\n[transaction] xfer_child_sync\n") );

	if( child == NULL )
	{
		DB( g_print(" - no child found\n") );
		return;
	}

	DB( g_print(" - found do sync\n") );

	/* update acc flags */
	acc = da_acc_get( child->kacc);
	if(acc != NULL)
		acc->flags |= AF_CHANGED;

	account_balances_sub (child);

	//# 1708974 enable different date
	//child->date		= s_txn->date;

	//#2019193 option the sync xfer status
	if( PREFS->xfer_syncstat == TRUE )
	{
		child->status = s_txn->status;
		child->flags |= OF_CHANGED;
	}	

	//#1673260 enable different currency
	if( !(child->flags & OF_ADVXFER) )
	{
		DB( g_print(" sync amount (==kcur)\n") );
		child->amount   = -s_txn->amount;
	}
	else
	{
		DB( g_print(" sync amount (!=kcur)\n") );
		child->amount = s_txn->xferamount;
		child->xferamount = s_txn->amount;
	}

	child->flags	= child->flags | OF_CHANGED;
	//#1295877 flag update to avoid bad column display
	child->flags &= ~(OF_INCOME);
	if( child->amount > 0 )
	  child->flags |= (OF_INCOME);
	// ensure to have INC to good display payee direction
	else if( child->amount == 0 )
	{
		if( s_txn->amount < 0 )
			child->flags |= (OF_INCOME);
	}

	child->kpay		= s_txn->kpay;
	child->kcat		= s_txn->kcat;
	if(child->memo)
		g_free(child->memo);
	child->memo	= g_strdup(s_txn->memo);
	if(child->info)
		g_free(child->info);
	child->info		= g_strdup(s_txn->info);

	account_balances_add (child);

	//#1252230 sync account also
	//#1663789 idem after 5.1
	//source changed: update child key (move of s_txn is done in external_edit)
	if( s_txn->kacc != child->kxferacc )
	{
		child->kxferacc = s_txn->kacc;
	}

	//dest changed: move child & update child key
	if( s_txn->kxferacc != child->kacc )
	{
		transaction_acc_move(child, child->kacc, s_txn->kxferacc);
	}

	//synchronise tags since 5.1
	g_free(child->tags);
	child->tags = tags_clone (s_txn->tags);

}


void transaction_xfer_remove_child(Transaction *src)
{
Transaction *dst;

	DB( g_print("\n[transaction] xfer_remove_child\n") );

	dst = transaction_xfer_child_strong_get( src );
	if( dst != NULL )
	{
	Account *acc = da_acc_get(dst->kacc);

		if( acc != NULL )
		{
			DB( g_print("deleting...") );
			account_balances_sub(dst);
			g_queue_remove(acc->txn_queue, dst);
			//#1419304 we keep the deleted txn to a trash stack	
			//da_transaction_free (dst);
			//g_trash_stack_push(&GLOBALS->txn_stk, dst);
			GLOBALS->deltxn_list = g_slist_prepend(GLOBALS->deltxn_list, dst);

			//#1691992
			acc->flags |= AF_CHANGED;
		}
	}
	
	src->kxfer = 0;
	src->kxferacc = 0;
}


// still useful for upgrade from < file v0.6 (hb v4.4 kxfer)
Transaction *transaction_old_get_child_transfer(Transaction *src)
{
Account *acc;
GList *list;

	DB( g_print("\n[transaction] get_child_transfer\n") );

	//DB( g_print(" search: %d %s %f %d=>%d\n", src->date, src->memo, src->amount, src->account, src->kxferacc) );
	acc = da_acc_get(src->kxferacc);

	if( acc != NULL )
	{
		list = g_queue_peek_head_link(acc->txn_queue);
		while (list != NULL)
		{
		Transaction *item = list->data;

			// no need to go higher than src txn date
			if(item->date > src->date)
				break;

			// keep this one for backward compatibility
			if( item->paymode == OLDPAYMODE_INTXFER)
			{
				if( src->date == item->date &&
					src->kacc == item->kxferacc &&
					src->kxferacc == item->kacc &&
					ABS(src->amount) == ABS(item->amount) )
				{
					//DB( g_print(" found : %d %s %f %d=>%d\n", item->date, item->memo, item->amount, item->account, item->kxferacc) );

					return item;
				}
			}
			list = g_list_next(list);
		}
	}

	DB( g_print(" not found...\n") );

	return NULL;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


gchar *transaction_get_status_string(Transaction *txn)
{
gchar *retval = "";

	if(txn->status == TXN_STATUS_CLEARED)
		retval = "c";
	else
	if(txn->status == TXN_STATUS_RECONCILED)
		retval = "R";
	else
	//#2051307 5.7.4 allow remind 
	if(txn->status == TXN_STATUS_REMIND)
		retval = "?";
	else
	if(txn->status == TXN_STATUS_VOID)
		retval = "v";

	return retval;
}


gboolean transaction_is_balanceable(Transaction *ope)
{
gboolean retval = TRUE;

	if( (ope->status == TXN_STATUS_VOID) )
		retval = FALSE;
	//#1812598
	else if( (ope->status == TXN_STATUS_REMIND) && (PREFS->includeremind == FALSE) )
		retval = FALSE;

	   return retval;
}


void transaction_remove(Transaction *ope)
{
Account *acc;

	//controls accounts valid (archive scheduled maybe bad)
	acc = da_acc_get(ope->kacc);
	if(acc == NULL) return;

	account_balances_sub(ope);

	if( ope->flags & OF_INTXFER )
	{
		transaction_xfer_remove_child( ope );
	}
	
	g_queue_remove(acc->txn_queue, ope);
	acc->flags |= AF_CHANGED;
	//#1419304 we keep the deleted txn to a trash stack	
	//da_transaction_free(entry);
	//g_trash_stack_push(&GLOBALS->txn_stk, ope);
	GLOBALS->deltxn_list = g_slist_prepend(GLOBALS->deltxn_list, ope);
}


void transaction_changed(Transaction *txn, gboolean saverecondate)
{
Account *acc;

	if( txn == NULL )
		return;

	acc = da_acc_get(txn->kacc);
	if(acc == NULL)
		return;	

	acc->flags |= AF_CHANGED;
	
	//#1581863 store reconciled date
	if( saverecondate == TRUE )
		acc->rdate = GLOBALS->today;

}


Transaction *transaction_add(GtkWindow *parent, Transaction *ope)
{
Transaction *newope;
Account *acc;

	DB( g_print("\n[transaction] transaction_add\n") );

	//controls accounts valid (archive scheduled maybe bad)
	acc = da_acc_get(ope->kacc);
	//#1972078 forbid on a closed account
	if( (acc == NULL) || (acc->flags & AF_CLOSED) )
		return NULL;

	DB( g_print(" acc is '%s' %d\n", acc->name, acc->key) );

	ope->kcur = acc->kcur;
	
	if(ope->flags & OF_INTXFER)
	{
		acc = da_acc_get(ope->kxferacc);
		//#1972078 forbid on a closed account
		if( (acc == NULL) || (acc->flags & AF_CLOSED) ) 
			return NULL;
		
		// delete any splits
		da_split_destroy(ope->splits);
		ope->splits = NULL;
		ope->flags &= ~(OF_SPLIT); //Flag that Splits are cleared
	}


	//allocate a new entry and copy from our edited structure
	newope = da_transaction_clone(ope);

	//init flag and keep remind status
	// already done in deftransaction_get
	//ope->flags |= (OF_ADDED);
	//remind = (ope->flags & OF_REMIND) ? TRUE : FALSE;
	//ope->flags &= (~OF_REMIND);

	/* cheque number is already stored in deftransaction_get */
	/* todo:move this to transaction add
		store a new cheque number into account ? */

	if( (newope->paymode == PAYMODE_CHECK) && (newope->info) && !(newope->flags & OF_INCOME) )
	{
	guint cheque;

		/* get the active account and the corresponding cheque number */
		acc = da_acc_get( newope->kacc);
		if( acc != NULL )
		{
			cheque = atol(newope->info);

			//DB( g_print(" -> should store cheque number %d to %d", cheque, newope->account) );
			if( newope->flags & OF_CHEQ2 )
			{
				acc->cheque2 = MAX(acc->cheque2, cheque);
			}
			else
			{
				acc->cheque1 = MAX(acc->cheque1, cheque);
			}
		}
	}

	/* add normal transaction */
	acc = da_acc_get( newope->kacc);
	if(acc != NULL)
	{
		acc->flags |= AF_ADDED;

		DB( g_print(" + add normal %p to acc %d\n", newope, acc->key) );
		//da_transaction_append(newope);
		da_transaction_insert_sorted(newope);

		account_balances_add(newope);

		if(newope->flags & OF_INTXFER)
		{
			transaction_xfer_search_or_add_child(parent, newope, newope->kxferacc);
		}

		//#1581863 store reconciled date
		if( newope->status == TXN_STATUS_RECONCILED )
			acc->rdate = GLOBALS->today;
	}
	
	return newope;
}


gboolean transaction_acc_move(Transaction *txn, guint32 okacc, guint32 nkacc)
{
Account *oacc, *nacc;

	DB( g_print("\n[transaction] acc_move\n") );

	if( okacc == nkacc )
		return TRUE;

	oacc = da_acc_get(okacc);
	nacc = da_acc_get(nkacc);
	if( oacc && nacc )
	{
		account_balances_sub(txn);
		if( g_queue_remove(oacc->txn_queue, txn) )
		{
			g_queue_push_tail(nacc->txn_queue, txn);
			txn->kacc = nacc->key;
			txn->kcur = nacc->kcur;
			nacc->flags |= AF_CHANGED;
			account_balances_add(txn);
			//#1865083 src acc also changed (balance)
			oacc->flags |= AF_CHANGED;
			return TRUE;
		}
		else
		{
			//ensure to keep txn into current account
			txn->kacc = okacc;
			account_balances_add(txn);
		}
	}
	return FALSE;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static gboolean transaction_similar_match(Transaction *stxn, Transaction *dtxn, guint32 daygap)
{
gboolean retval = FALSE;

	if(stxn == dtxn)
		return FALSE;

	DB( g_print(" date: %d - %d = %d\n", stxn->date, dtxn->date, stxn->date - dtxn->date) );
	
	if( stxn->kcur == dtxn->kcur
	 &&	stxn->amount == dtxn->amount
	 && ( (stxn->date - dtxn->date) <= daygap )
	 //todo: at import we also check payee, but maybe too strict here
	 && (hb_string_compare(stxn->memo, dtxn->memo) == 0)
	  )
	{
		retval = TRUE;
	}
	return retval;
}


void transaction_similar_unmark(Account *acc)
{
GList *lnk_txn;

	lnk_txn = g_queue_peek_tail_link(acc->txn_queue);
	while (lnk_txn != NULL)
	{
	Transaction *stxn = lnk_txn->data;

		stxn->dspflags &= ~(TXN_DSPFLG_DUPSRC|TXN_DSPFLG_DUPDST);
		//stxn->marker = TXN_MARK_NONE;
		lnk_txn = g_list_previous(lnk_txn);
	}
}


gint transaction_similar_mark(Account *acc, guint32 daygap)
{
GList *lnk_txn, *list2;
gint nball = 0;
gint nbdup = 0;

	//warning the list must be sorted by date then amount
	//ideally (easier to parse) we shoudl get a list sorted by amount, then date
	DB( g_print("\n[transaction] check duplicate\n") );

	DB( g_print("\n - account:'%s' gap:%d\n", acc->name, daygap) );

	#if MYDEBUG == 1
	GTimer *t = g_timer_new();
	g_print(" - start parse\n");
	#endif


	/*
	llast = g_list_last(old ope list);
	DB( g_print("- end last : %f sec\n", g_timer_elapsed(t, NULL)) );
	g_timer_reset(t);

	ltxn = llast->data;
	g_date_clear(&gd, 1);
	g_date_set_julian(&gd, ltxn->date);
	g_print(" - last julian=%u %02d-%02d-%04d\n", ltxn->date, g_date_get_day (&gd), g_date_get_month (&gd), g_date_get_year(&gd));

	minjulian = ltxn->date - (366*2);
	g_date_clear(&gd, 1);
	g_date_set_julian(&gd, minjulian);
	g_print(" - min julian=%u %02d-%02d-%04d\n", minjulian, g_date_get_day (&gd), g_date_get_month (&gd), g_date_get_year(&gd));
	*/

	transaction_similar_unmark(acc);

	//mark duplicate
	lnk_txn = g_queue_peek_tail_link(acc->txn_queue);
	while (lnk_txn != NULL)
	{
	Transaction *stxn = lnk_txn->data;

		//if(stxn->date < minjulian)
		//	break;
		DB( g_print("------\n eval src: %d, '%s', '%s', %.2f\n", stxn->date, stxn->info, stxn->memo, stxn->amount) );

		list2 = g_list_previous(lnk_txn);
		while (list2 != NULL)
		{
		Transaction *dtxn = list2->data;

			DB( g_print(" + with dst: %d, '%s', '%s', %.2f\n", dtxn->date, dtxn->info, dtxn->memo, dtxn->amount) );

			if( (stxn->date - dtxn->date) > daygap )
			{
				DB( g_print(" break %d %d\n", (dtxn->date - daygap) , (stxn->date - daygap)) );
				break;
			}
				
			//if( dtxn->marker == TXN_MARK_NONE )
			if( (dtxn->dspflags & (TXN_DSPFLG_DUPSRC|TXN_DSPFLG_DUPDST)) == 0 )
			{
				if( transaction_similar_match(stxn, dtxn, daygap) )
				{
					//stxn->marker = TXN_MARK_DUPSRC;
					stxn->dspflags |= TXN_DSPFLG_DUPSRC;
					//dtxn->marker = TXN_MARK_DUPDST;
					dtxn->dspflags |= TXN_DSPFLG_DUPDST;
					DB( g_print(" = dtxn marker=%d\n", dtxn->dspflags) );
					nball++;
				}
			}
			else
			{
				DB( g_print(" already marked %d\n", dtxn->dspflags) );
			}

			
			list2 = g_list_previous(list2);
		}
	
		DB( g_print(" = stxn marker=%d\n", stxn->dspflags) );
		//if( stxn->marker == TXN_MARK_DUPSRC )
		if( stxn->dspflags & TXN_DSPFLG_DUPSRC )
			nbdup++;
		
		lnk_txn = g_list_previous(lnk_txn);
	}

	DB( g_print(" - end parse : %f sec\n", g_timer_elapsed(t, NULL)) );
	DB( g_timer_destroy (t) );

	DB( g_print(" - found: %d/%d dup\n", nbdup, nball ) );

	return nbdup;
}


guint
transaction_auto_all_from_payee(GList *txnlist)
{
GList *list;
Payee *pay;
guint changes = 0;

	DB( g_print("\n[transaction] auto from payee\n") );

	DB( g_print(" n_txn=%d\n", g_list_length(txnlist)) );

	list = g_list_first(txnlist);
	while(list != NULL)
	{
	Transaction *txn = list->data;

		if( txn != NULL )
		{
			pay = da_pay_get(txn->kpay);
			if( pay != NULL )
			{
				if( txn->kcat == 0 )
				{
					txn->kcat = pay->kcat;
					changes++;
				}
					
				if( txn->paymode == PAYMODE_NONE )
				{
					txn->paymode = pay->paymode;
					changes++;
				}
			}

		}
		list = g_list_next(list);
	}

	return changes;
}



/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* = = experimental = = */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


/*
probably add a structure hosted into a glist here
with kind of problem: duplicate, child xfer, orphan xfer	
and collect all that with target txn
*/


/*void future_transaction_test_account(Account *acc)
{
GList *lnk_txn, *list2;
gint nball = 0;
gint nbdup = 0;
gint nbxfer = 0;
GPtrArray *array;

//future
gint gapday = 0, i;

	//warning the list must be sorted by date then amount
	//ideally (easier to parse) we shoudl get a list sorted by amount, then date

	DB( g_print("\n[transaction] check duplicate\n") );



	DB( g_print("\n - account:'%s'\n", acc->name) );

	GTimer *t = g_timer_new();
	g_print(" - start parse\n");


	llast = g_list_last(old ope list);
	DB( g_print("- end last : %f sec\n", g_timer_elapsed(t, NULL)) );
	g_timer_reset(t);

	ltxn = llast->data;
	g_date_clear(&gd, 1);
	g_date_set_julian(&gd, ltxn->date);
	g_print(" - last julian=%u %02d-%02d-%04d\n", ltxn->date, g_date_get_day (&gd), g_date_get_month (&gd), g_date_get_year(&gd));

	minjulian = ltxn->date - (366*2);
	g_date_clear(&gd, 1);
	g_date_set_julian(&gd, minjulian);
	g_print(" - min julian=%u %02d-%02d-%04d\n", minjulian, g_date_get_day (&gd), g_date_get_month (&gd), g_date_get_year(&gd));

	array = g_ptr_array_sized_new (25);

	lnk_txn = g_queue_peek_tail_link(acc->txn_queue);
	while (lnk_txn != NULL)
	{
	Transaction *stxn = lnk_txn->data;

		//if(stxn->date < minjulian)
		//	break;
		DB( g_print("------\n eval src: %d, '%s', '%s', %2.f\n", stxn->date, stxn->info, stxn->memo, stxn->amount) );

		stxn->marker = 0;
		list2 = g_list_previous(lnk_txn);
		while (list2 != NULL)
		{
		Transaction *dtxn = list2->data;

			stxn->marker = 0;
			if( (dtxn->date + gapday) < (stxn->date + gapday) )
				break;

			DB( g_print(" + with dst: %d, '%s', '%s', %2.f\n", dtxn->date, dtxn->info, dtxn->memo, dtxn->amount) );

			if( transaction_similar_match(stxn, dtxn, gapday) )
			{
				g_ptr_array_add (array, stxn);
				g_ptr_array_add (array, dtxn);
				nbdup++;
				DB( g_print(" + dst=1 src=1\n") );
			}

			nball++;
			list2 = g_list_previous(list2);
		}

		lnk_txn = g_list_previous(lnk_txn);
	}

	DB( g_print(" - end parse : %f sec\n", g_timer_elapsed(t, NULL)) );
	DB( g_timer_destroy (t) );

	for(i=0;i<array->len;i++)
	{
	Transaction *txn = g_ptr_array_index(array, i);
		txn->marker = 1;
	}

	g_ptr_array_free(array, TRUE);

	DB( g_print(" - found: %d/%d dup, %d xfer\n", nbdup, nball, nbxfer ) );

}





//todo: add a limitation, no need to go through all txn
// 1 year in th past, or abolute number ?
gint future_transaction_test_notification(void)
{
GList *lst_acc, *lnk_acc;

	DB( g_print("\ntransaction_test_notification\n") );
	
	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		transaction_similar_mark(acc);

		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);
	
	return 0;
}
*/



