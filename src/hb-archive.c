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
#include "hb-archive.h"
#include "hb-split.h"

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

#define DB2(x);
//#define DB2(x) (x);


/* our global datas */
extern struct HomeBank *GLOBALS;


extern HbKvData CYA_ARC_UNIT[];
extern HbKvData CYA_ARC_WEEKEND[];

#if MYDEBUG
gchar *WDAY[] = { "bad", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
#endif


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void
da_archive_clean(Archive *item)
{
	if(item != NULL)
	{
		g_free(item->memo);
		item->memo = NULL;

		g_free(item->number);
		item->number = NULL;

		//5.3 added as it was a leak
		g_free(item->tags);
		item->tags = NULL;

		if(item->splits != NULL)
		{
			da_split_destroy(item->splits);
			item->splits = NULL;
			item->flags &= ~(OF_SPLIT); //Flag that Splits are cleared
		}
	}
}


void
da_archive_free(Archive *item)
{
	if(item != NULL)
	{
		da_archive_clean(item);
		g_free(item);
	}
}


Archive *
da_archive_malloc(void)
{
Archive *item;

	item = g_malloc0(sizeof(Archive));
	item->key = 1;
	return item;
}


Archive *
da_archive_clone(Archive *src_item)
{
Archive *new_item = g_memdup(src_item, sizeof(Archive));

	if(new_item)
	{
		//duplicate the string
		new_item->memo = g_strdup(src_item->memo);
		new_item->number = g_strdup(src_item->number);

		//duplicate tags
		//no g_free here to avoid free the src tags (memdup copie dthe ptr)
		new_item->tags = tags_clone(src_item->tags);

		//duplicate splits
		//no g_free here to avoid free the src tags (memdup copie dthe ptr)
		new_item->splits = da_splits_clone(src_item->splits);
		if( da_splits_length (new_item->splits) > 0 )
			new_item->flags |= OF_SPLIT; //Flag that Splits are active
	}
	return new_item;
}


void
da_archive_destroy(GList *list)
{
GList *tmplist = g_list_first(list);

	while (tmplist != NULL)
	{
	Archive *item = tmplist->data;
		da_archive_free(item);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(list);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


guint
da_archive_length(void)
{
	return g_list_length(GLOBALS->arc_list);
}


guint32
da_archive_get_max_key(void)
{
GList *tmplist = g_list_first(GLOBALS->arc_list);
guint32 max_key = 0;

	while (tmplist != NULL)
	{
	Archive *item = tmplist->data;

		max_key = MAX(item->key, max_key);		
		tmplist = g_list_next(tmplist);
	}
	
	return max_key;
}


//delete


//insert


/* append a fav with an existing key (from xml file only) */
gboolean
da_archive_append(Archive *item)
{
	//perf must use preprend, see glib doc
	//GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);
	GLOBALS->arc_list = g_list_prepend(GLOBALS->arc_list, item);
	return TRUE;
}


gboolean
da_archive_append_new(Archive *item)
{
	item->key = da_archive_get_max_key() + 1;
	//TODO: perf must use preprend, see glib doc
	GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);
	return TRUE;
}


Archive *
da_archive_get(guint32 key)
{
GList *tmplist;
Archive *retval = NULL;

	tmplist = g_list_first(GLOBALS->arc_list);
	while (tmplist != NULL)
	{
	Archive *item = tmplist->data;

		if(item->key == key)
		{
			retval = item;
			break;
		}
		tmplist = g_list_next(tmplist);
	}
	return retval;
}

//#1968249 build a non empty label, when memo/payee/category are empty
void da_archive_get_display_label(GString *tpltitle, Archive *item)
{
	g_string_truncate(tpltitle, 0);
	if(item->memo != NULL)
	{
		g_string_append(tpltitle, item->memo);
	}
	else
	{
		g_string_append(tpltitle, _("(no memo)") );
		if(item->kpay > 0)
		{
		Payee *pay = da_pay_get(item->kpay);

			if(pay != NULL)
			{
				g_string_append_c(tpltitle, ' ');
				g_string_append(tpltitle, pay->name);
			}
		}

		if(item->kcat > 0)
		{
		Category *cat = da_cat_get(item->kcat);

			if(cat != NULL)
			{
				g_string_append(tpltitle, " / ");
				g_string_append(tpltitle, cat->fullname);
			}
		}

	}
}


void da_archive_consistency(Archive *item)
{
Account *acc;
Category *cat;
Payee *pay;
guint nbsplit;

	// check category exists
	cat = da_cat_get(item->kcat);
	if(cat == NULL)
	{
		g_warning("tpl consistency: fixed invalid cat %d", item->kcat);
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
			g_warning("tpl consistency: fixed invalid cat on split txn");
			item->kcat = 0;
			GLOBALS->changes_count++;
		}
	}
	
	// check payee exists
	pay = da_pay_get(item->kpay);
	if(pay == NULL)
	{
		g_warning("tpl consistency: fixed invalid pay %d", item->kpay);
		item->kpay = 0;
		GLOBALS->changes_count++;
	}

	// 5.3: fix split on intxfer
	if( ((item->flags & OF_INTXFER) || (item->paymode == OLDPAYMODE_INTXFER)) && (item->splits != NULL) )
	{
		g_warning("tpl consistency: fixed invalid split on xfer");
		item->flags &= ~(OF_INTXFER);
		item->paymode = PAYMODE_XFER;
		item->kxferacc = 0;
	}
	
	// reset dst acc for non xfer transaction
	if( !((item->flags & OF_INTXFER) || (item->paymode == OLDPAYMODE_INTXFER)) )
		item->kxferacc = 0;

	// delete automation if dst_acc not exists
	if( (item->flags & OF_INTXFER) || (item->paymode == OLDPAYMODE_INTXFER) )
	{
		acc = da_acc_get(item->kxferacc);
		if(acc == NULL)
		{
			item->flags &= ~(OF_AUTO);	//delete flag
		}
	}

}


/* = = = = = = = = = = = = = = = = = = = = */


static gint
da_archive_glist_name_compare_func(Archive *a, Archive *b)
{
	return hb_string_utf8_compare(a->memo, b->memo);
}


static gint
da_archive_glist_key_compare_func(Archive *a, Archive *b)
{
	return hb_string_utf8_compare(a->memo, b->memo);
}


GList *da_archive_glist_sorted(gint column)
{

	switch(column)
	{
		case HB_GLIST_SORT_NAME:
			GLOBALS->arc_list = g_list_sort(GLOBALS->arc_list, (GCompareFunc)da_archive_glist_name_compare_func);
			break;
		//case HB_GLIST_SORT_KEY:
		default:
			GLOBALS->arc_list = g_list_sort(GLOBALS->arc_list, (GCompareFunc)da_archive_glist_key_compare_func);
			break;
	}

	return GLOBALS->arc_list;
}


void da_archive_stats(gint *nbtpl, gint *nbsch)
{
GList *tmplist = g_list_first(GLOBALS->arc_list);
gint nbt, nbs;

	nbt = nbs = 0;
	while (tmplist != NULL)
	{
	Archive *item = tmplist->data;

		if(item->flags & OF_AUTO)
			nbs++;
		else
			nbt++;

		tmplist = g_list_next(tmplist);
	}

	if( nbtpl != NULL)
		*nbtpl = nbt;
	if( nbsch != NULL)
		*nbsch = nbs;
}


//#1872140 don't prefix if not from ledger
Archive *da_archive_init_from_transaction(Archive *arc, Transaction *txn, gboolean fromledger)
{
	DB( g_print("\n[scheduled] init from txn\n") );

	da_archive_clean(arc);

	//fill it
	arc->amount		= txn->amount;
	arc->kacc		= txn->kacc;
	//#1673260
	arc->xferamount = txn->xferamount;
	arc->kxferacc	= txn->kxferacc;
	arc->paymode	= txn->paymode;
	arc->flags		= txn->flags;
	arc->status		= txn->status;
	arc->kpay		= txn->kpay;
	arc->kcat		= txn->kcat;

	//5.3.3 prefixed with prefilled
	//#1883063 **PREFILLED** only when fromledger==TRUE, test order was wrogn here
	if( fromledger == FALSE )
	{
		if(txn->memo != NULL)
			arc->memo = g_strdup(txn->memo);	
	}
	else
	{
		arc->flags |= OF_PREFILLED;
		//#2018680
		if(txn->memo != NULL)
			arc->memo = g_strdup( txn->memo );
			//arc->memo = g_strdup_printf("%s %s", _("**PREFILLED**"), txn->memo );
		//else
		//	arc->memo = g_strdup(_("**PREFILLED**"));
	}	

	if(txn->number != NULL)
		arc->number = g_strdup(txn->number);

	arc->tags    = tags_clone(txn->tags);
	arc->splits  = da_splits_clone(txn->splits);

	if( da_splits_length (arc->splits) > 0 )
		arc->flags |= OF_SPLIT; //Flag that Splits are active
	
	return arc;
}


/* = = = = = = = = = = = = = = = = = = = = */


gboolean
template_is_account_used(Archive *arc)
{
GList *lacc, *list;
gboolean retval = FALSE;

	lacc = list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *acc = list->data;

		if( acc->karc == arc->key )
			retval = TRUE;

		list = g_list_next(list);
	}
	g_list_free(lacc);
	
	return retval;
}


/* = = = = = = = = = = = = = = = = = = = = */


static void _sched_nextdate_weekend_adjust(GDate *date)
{
GDateWeekday wday = g_date_get_weekday(date);

	if( wday == G_DATE_SATURDAY )
		g_date_add_days (date, 2);
	else
	if( wday == G_DATE_SUNDAY )
		g_date_add_days (date, 1);
}


static guint32 _sched_date_get_next_post(GDate *date, Archive *arc, guint32 nextdate)
{
guint32 nextpostdate = nextdate;

	DB2( g_print("\n[scheduled] date_get_next_post\n") );

	g_date_set_julian(date, nextpostdate);

	DB2( hb_print_date(g_date_get_julian(date), "in:") );

	switch(arc->unit)
	{
		case AUTO_UNIT_DAY:
			g_date_add_days(date, arc->every);
			break;
		case AUTO_UNIT_WEEK:
			g_date_add_days(date, 7 * arc->every);
			break;
		case AUTO_UNIT_MONTH:
			g_date_add_months(date, arc->every);
			break;
		case AUTO_UNIT_YEAR:
			g_date_add_years(date, arc->every);
			break;
	}

	DB2( hb_print_date(g_date_get_julian(date), "out:") );

	//#1906953 add skip weekend
	if( arc->weekend == ARC_WEEKEND_SKIP )
	{
		_sched_nextdate_weekend_adjust(date);
		DB2( hb_print_date(g_date_get_julian(date), "out:") );
	}

	/* get the final post date */
	nextpostdate = g_date_get_julian(date);
	
	return nextpostdate;
}


//#1906953 add skip weekend
//ui-archive
void scheduled_nextdate_weekend_adjust(Archive *arc)
{
	if( arc->weekend == ARC_WEEKEND_SKIP )
	{
	GDate date;
		g_date_set_julian(&date, arc->nextdate);
		_sched_nextdate_weekend_adjust(&date);
		arc->nextdate = g_date_get_julian(&date);
	}
}


//hb-report
guint32 scheduled_date_get_next_post(GDate *date, Archive *arc, guint32 nextdate)
{
	return _sched_date_get_next_post(date, arc, nextdate);
}


// hub_scheduled / hb-report
gboolean scheduled_is_postable(Archive *arc)
{
	if( !(arc->flags & OF_AUTO) )
		return FALSE;
	if( arc->kacc == 0 )
		return FALSE;
	if( hb_amount_equal(arc->amount, 0.0) )
		return FALSE;

	return TRUE;
}


// hub_scheduled
guint32 scheduled_get_latepost_count(GDate *date, Archive *arc, guint32 jrefdate)
{
guint32 jcurdate = arc->nextdate;
guint32 nblate = 0;
	
	while (jcurdate <= jrefdate)
	{
		jcurdate = _sched_date_get_next_post(date, arc, jcurdate);
		nblate++;
		// break if over limit or at 11 max (to display +10)
		if( nblate >= 11 || ( (arc->flags & OF_LIMIT) && (nblate >= arc->limit) ) )
			break;
	}
	return nblate;
}


//used here + hub-scheduled
guint32 scheduled_get_postdate(Archive *arc, guint32 postdate)
{
GDate date;
GDateWeekday wday;
guint32 finalpostdate;
gint shift;

	DB2( g_print("\n[scheduled] get_postdate\n") );

	finalpostdate = postdate;
	
	DB2( hb_print_date(finalpostdate, "in:") );
	
	g_date_set_julian(&date, finalpostdate);

	/* manage weekend exception */
	if( arc->weekend > 0 )
	{
		wday = g_date_get_weekday(&date);

		DB2( g_print(" %s wday=%d '%s'\n", arc->memo, wday, WDAY[wday]) );

		if( wday >= G_DATE_SATURDAY )
		{
			switch(arc->weekend)
			{
				case ARC_WEEKEND_BEFORE: /* shift before : sun 7-5=+2 , sat 6-5=+1 */
					shift = wday - G_DATE_FRIDAY;
					DB2( g_print("sub=%d\n", shift) );
					g_date_subtract_days (&date, shift);
					break;

				case ARC_WEEKEND_AFTER: /* shift after : sun 8-7=1 , sat 8-6=2 */
					shift = 8 - wday;
					DB2( g_print("add=%d\n", shift) );
					g_date_add_days (&date, shift);
					break;
			}
		}
	}
	
	/* get the final post date and free */
	finalpostdate = g_date_get_julian(&date);
	
	DB2( hb_print_date(finalpostdate, "out:") );
	
	return finalpostdate;
}


/* return 0 is max number of post is reached */
//used here + hub_scheduled
//this modify the arc
guint32 scheduled_date_advance(Archive *arc)
{
GDate date;
gushort lastday;

	DB2( g_print("\n[scheduled] date_advance\n") );

	g_date_set_julian(&date, arc->nextdate);
	// saved the current day number
	lastday = g_date_get_day(&date);

	arc->nextdate = _sched_date_get_next_post(&date, arc, arc->nextdate);

	DB2( g_print(" raw next post date: %02d-%02d-%4d\n", g_date_get_day(&date), g_date_get_month (&date), g_date_get_year(&date) ) );

	//for day > 28 we might have a gap to compensate later
	if( (arc->unit==AUTO_UNIT_MONTH) || (arc->unit==AUTO_UNIT_YEAR) )
	{
		if( lastday >= 28 )
		{
			DB2( g_print(" lastday:%d, daygap:%d\n", lastday, arc->daygap) );
			if( arc->daygap > 0 )
			{
				g_date_add_days (&date, arc->daygap);
				arc->nextdate = g_date_get_julian (&date);
				lastday += arc->daygap;
				DB2( g_print(" adjusted post date: %2d-%2d-%4d\n", g_date_get_day(&date), g_date_get_month (&date), g_date_get_year(&date) ) );
			}

			arc->daygap = CLAMP(lastday - g_date_get_day(&date), 0, 3);
		
			DB2( g_print(" daygap is %d\n", arc->daygap) );
		}
		else
			arc->daygap = 0;
	}

	//#1556289
	/* check limit, update and maybe break */
	if(arc->flags & OF_LIMIT)
	{
		arc->limit--;
		if(arc->limit <= 0)
		{
			arc->flags ^= (OF_LIMIT | OF_AUTO);	// invert flags
			arc->nextdate = 0;
		}
	}

	return arc->nextdate;
}


//hb-scheduled
void scheduled_date_get_show_minmax(gint select, guint32 *mindate, guint32 *maxdate)
{
GDate *date;
guint16 month, year;

	if( (mindate == NULL) || (maxdate == NULL) )
		return;

	date  = g_date_new_julian(GLOBALS->today);
	switch( select )
	{
		case FLT_SCHEDULED_THISMONTH:
		case FLT_SCHEDULED_NEXTMONTH:
			g_date_set_day(date, 1);
			if( select == FLT_SCHEDULED_NEXTMONTH)
				g_date_add_months(date, 1);
			*mindate = g_date_get_julian(date);
			month = g_date_get_month(date);
			year  = g_date_get_year(date);		
			g_date_add_days(date, g_date_get_days_in_month(month, year));
			*maxdate = g_date_get_julian(date) - 1;
			break;
		case FLT_SCHEDULED_NEXT30DAYS:
			*mindate = GLOBALS->today;
			*maxdate = GLOBALS->today + 30;
			break;
		case FLT_SCHEDULED_NEXT60DAYS:
			*mindate = GLOBALS->today;
			*maxdate = GLOBALS->today + 60;
			break;
		case FLT_SCHEDULED_NEXT90DAYS:
			*mindate = GLOBALS->today;
			*maxdate = GLOBALS->today + 90;
			break;
		default:
			*mindate = HB_MINDATE;
			*maxdate = HB_MAXDATE;
			break;	
	}

	g_date_free(date);
}


/*
 *  return the maximum date a scheduled txn can be posted to
 */
//hb-scheduled / ui-hbfile
guint32 scheduled_date_get_post_max(guint32 start, gint auto_smode, gint auto_nbdays, gint auto_weekday, gint nbmonth)
{
guint32 nbdays;

	DB2( g_print("\n[scheduled] get max post date\n") );

	switch( auto_smode)
	{
		//add until xx of the yy month (excluded)
		//if(auto_smode == 0)
		case ARC_POSTMODE_PAYOUT:
		{
		GDate *tdate, *date;
		gshort tday;
		
			DB2( g_print(" payout %d of %d months\n", auto_weekday, nbmonth) );

			tdate = g_date_new_julian(start);
			tday = g_date_get_day(tdate);

			//set <payoutday>/xx/xxxx			
			date = g_date_new_julian(start);
			g_date_set_day(date, auto_weekday);

			//if today is payout day (or future)
			if( tday >= auto_weekday )
			{
				DB2( g_print(" day: %d >= %d\n", tday, auto_weekday) );
				//we add nbmonth
				g_date_add_months(date, 1);
				if( nbmonth > 1 )
				{
					//here we consider start is january
					//TODO: ? use also fisc_year_day + fisc_year_month
					for(gshort i=1;i<nbmonth;i++)
					{
						if( (g_date_get_month(date) % nbmonth) == 1 )
							break;
						DB2( g_print(" >add 1 month\n") );
						g_date_add_months(date, 1);
					}
				}
			}

			//#2065740 as we now post maxpostdate included: -1
			g_date_subtract_days(date, 1);

			nbdays = g_date_days_between(tdate, date);
			g_date_free(date);
			g_date_free(tdate);
		}
		break;

		//add xx days in advance the current date
		//if(auto_smode == 1)
		case ARC_POSTMODE_ADVANCE:
			nbdays = auto_nbdays;
			break;

		case ARC_POSTMODE_DUEDATE:
		default:
			nbdays = 0;
			break;
	}
	DB2( g_print(" nbdays=%d\n", nbdays) );

	DB2( hb_print_date(start + nbdays, "out:") );

	return start + nbdays;
}


//mainwindows / hub-scheduled
gint scheduled_post_all_pending(void)
{
GList *list;
gint count = 0;
guint32 maxpostdate;

	DB( g_print("\n[scheduled] -- post_all_pending --\n") );
	
	maxpostdate = scheduled_date_get_post_max(GLOBALS->today, GLOBALS->auto_smode, GLOBALS->auto_nbdays, GLOBALS->auto_weekday, GLOBALS->auto_nbmonths);
	DB( hb_print_date(maxpostdate, " >maxpostdate") );

	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *arc = list->data;

		if( !(arc->flags & OF_AUTO) )
			goto nextarchive;

		DB( g_print("----\neval %d w.e=%s limit=%d '%s' flags=0x%04x\n", 
			arc->nextdate, 
			hbtk_get_label(CYA_ARC_WEEKEND, arc->weekend), 
			arc->flags & OF_LIMIT ? arc->limit : -1,
			arc->memo,
			arc->flags) );

		if( !scheduled_is_postable(arc) )
		{
			DB( g_print(" >skip: not postable - auto=%d kacc=%d amt=%.2f \n", arc->flags & OF_AUTO, arc->kacc, arc->amount) );
			goto nextarchive;
		}

		//TODO: maybe add a security here
		for(;;)
		{
		Transaction *txn;
		//#2065955 to get weekend before/after posted
		guint32 jpostdate = scheduled_get_postdate(arc, arc->nextdate);

			#if MYDEBUG
			if( arc->nextdate != jpostdate )
				DB( g_print(" >shift: %d >> %d\n", arc->nextdate, jpostdate) );
			#endif

			//#2064839 <=
			if( jpostdate <= maxpostdate )
			{
				DB( g_print(" >post: date=%d (%+d)\n", jpostdate, jpostdate - arc->nextdate) );

				//5.5.3 fixed leak as this was outside the loop
				txn = da_transaction_malloc();
				da_transaction_init_from_template(txn, arc);
				txn->date = jpostdate;
				/* todo: ? fill in cheque number */
				transaction_add(NULL, FALSE, txn);
				
				da_transaction_free (txn);
				count++;
				
				//can switch OF_AUTO off, if limit reached
				scheduled_date_advance(arc);
				if( !(arc->flags & OF_AUTO) )
				{
					DB( g_print(" >stop: limit reached\n") );
					goto nextarchive;
				}
			}
			else
			{
				DB( g_print(" >skip: no pending\n") );
				goto nextarchive;
			}
		}

nextarchive:
		list = g_list_next(list);
	}

	GLOBALS->changes_count += count;
	return count;
}

