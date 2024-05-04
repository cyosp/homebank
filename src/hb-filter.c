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
#include "hb-filter.h"

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif


//DB out of bounds
#define DBOOB(x);
//#define DBOOB(x) (x);


/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


/* = = = = = = = = = = = = = = = = */


static void da_flt_clean(Filter *flt)
{
	if(flt != NULL)
	{
		g_free(flt->tag);
		flt->tag = NULL;
		
		g_free(flt->memo);
		flt->memo = NULL;

		g_free(flt->info);
		flt->info = NULL;

		g_free(flt->name);
		flt->name = NULL;
		
		if(flt->gbtag != NULL)
		{
			g_array_free(flt->gbtag, TRUE);
			flt->gbtag = NULL;
		}

		if(flt->gbcat != NULL)
		{
			g_array_free(flt->gbcat, TRUE);
			flt->gbcat = NULL;
		}

		if(flt->gbpay != NULL)
		{
			g_array_free(flt->gbpay, TRUE);
			flt->gbpay = NULL;
		}

		if(flt->gbacc != NULL)
		{
			g_array_free(flt->gbacc, TRUE);
			flt->gbacc = NULL;
		}
	}
}


static void da_flt_init(Filter *flt)
{

	DB( g_print("da_flt_init\n") );

	//always allocate 1 elt minimum
	// /!\ each array still ->len = 0
	flt->gbacc = g_array_sized_new(FALSE, TRUE, sizeof(gchar), 1);
	flt->gbpay = g_array_sized_new(FALSE, TRUE, sizeof(gchar), 1);
	flt->gbcat = g_array_sized_new(FALSE, TRUE, sizeof(gchar), 1);
	flt->gbtag = g_array_sized_new(FALSE, TRUE, sizeof(gchar), 1);

}


void da_flt_free(Filter *flt)
{
	DB( g_print("da_flt_free\n") );
	if(flt != NULL)
	{
		da_flt_clean(flt);
		g_free(flt);
	}
}


Filter *da_flt_malloc(void)
{
Filter *flt;

	DB( g_print("da_flt_malloc\n") );
	flt = g_malloc0(sizeof(Filter));

	da_flt_init(flt);

	return flt;
}


void da_flt_copy(Filter *src, Filter *dst)
{
	DB( g_print("da_flt_copy\n") );

	DB( g_print(" %p (%s) to %p (%s)\n", src, src->name, dst, dst->name) );

	if(!src || !dst)
		return;

	DB( g_print(" %d acc\n", src->gbacc->len) );
	DB( g_print(" %d pay\n", src->gbpay->len) );
	DB( g_print(" %d cat\n", src->gbcat->len) );

	//clean any previous extra memory
	da_flt_clean(dst);
	//raw duplicate the memory segment
	memcpy(dst, src, sizeof(Filter));

	//duplicate extra memory
	dst->name = g_strdup(src->name);
	dst->info = g_strdup(src->info);
	dst->memo = g_strdup(src->memo);
	dst->tag  = g_strdup(src->tag);
	
	dst->gbacc = g_array_copy(src->gbacc);
	dst->gbpay = g_array_copy(src->gbpay);
	dst->gbcat = g_array_copy(src->gbcat);
	dst->gbtag = g_array_copy(src->gbtag);

	DB( g_print(" %d acc\n", dst->gbacc->len) );
	DB( g_print(" %d pay\n", dst->gbpay->len) );
	DB( g_print(" %d cat\n", dst->gbcat->len) );

	DB( g_print(" copied\n\n") );

}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


#if MYDEBUG == 1
static void my_debug_garray(gchar *type, GArray *array)
{
guint i;

	g_print(" debug '%s' len=%d\n", type, array->len);
	
	for(i=0 ; i < array->len ; i++)
	{
		g_print("%02d ", array->data[i]);
	}
	g_print("\n");
}
#endif


static void da_flt_item_set(GArray *array, guint32 key, gboolean status)
{
	if(key < array->len)
	{
	gchar *sel = &g_array_index(array, gchar, key);
		*sel = status;
		DB( g_print(" modified existing [%d]=>%d\n", key, status) );
	}
	else
	{
		DB( g_print(" insert new val [%d]=>%d\n", key, status) );
		g_array_insert_vals(array, key, &status, 1);
	}
}

void da_flt_status_tag_set(Filter *flt, guint32 ktag, gboolean status)
{
	da_flt_item_set(flt->gbtag, ktag, status);
	DB( g_print(" set tag %d to %d\n", ktag, status) );
}

void da_flt_status_cat_set(Filter *flt, guint32 kcat, gboolean status)
{
	da_flt_item_set(flt->gbcat, kcat, status);
	DB( g_print(" set cat %d\n", kcat) );
}

void da_flt_status_pay_set(Filter *flt, guint32 kpay, gboolean status)
{
	da_flt_item_set(flt->gbpay, kpay, status);
	DB( g_print(" set pay %d\n", kpay) );
}

void da_flt_status_acc_set(Filter *flt, guint32 kacc, gboolean status)
{
	da_flt_item_set(flt->gbacc, kacc, status);
	DB( g_print(" set acc %d\n", kacc) );
}


gboolean da_flt_status_tag_get(Filter *flt, guint32 ktag)
{
	if(flt->gbtag == NULL)
		return FALSE;

	if(ktag < flt->gbtag->len)
		return flt->gbtag->data[ktag];

	DBOOB( g_warning("filter get tag out of bounds %d of %d", ktag, flt->gbtag->len) );
	return FALSE;
}

gboolean da_flt_status_cat_get(Filter *flt, guint32 kcat)
{
	if(flt->gbcat == NULL)
		return FALSE;

	if(kcat < flt->gbcat->len)
		return flt->gbcat->data[kcat];

	DBOOB( g_warning("filter get cat out of bounds %d of %d", kcat, flt->gbcat->len) );
	return FALSE;
}

gboolean da_flt_status_pay_get(Filter *flt, guint32 kpay)
{
	if(flt->gbpay == NULL)
		return FALSE;

	if(kpay < flt->gbpay->len)
		return flt->gbpay->data[kpay];

	DBOOB( g_warning("filter get pay out of bounds %d of %d", kpay, flt->gbpay->len) );
	return FALSE;
}

gboolean da_flt_status_acc_get(Filter *flt, guint32 kacc)
{
	if(flt->gbacc == NULL)
		return FALSE;


	if(kacc < flt->gbacc->len)
		return flt->gbacc->data[kacc];

	DBOOB( g_warning("filter get acc out of bounds %d of %d", kacc, flt->gbacc->len) );
	return FALSE;
}




/* TODO: check this : user in rep_time only */
void filter_status_acc_clear_except(Filter *flt, guint32 selkey)
{
guint i;

	DB( g_print("[filter] acc clear %d\n", flt->gbacc->len) );
	
	for(i=0;i<flt->gbacc->len;i++)
	{
		flt->gbacc->data[i] = 0;
	}
	da_flt_status_acc_set(flt, selkey, TRUE);
}

void filter_status_pay_clear_except(Filter *flt, guint32 selkey)
{
guint i;

	DB( g_print("[filter] pay clear %d\n", flt->gbpay->len) );
	
	for(i=0;i<flt->gbpay->len;i++)
	{
		flt->gbpay->data[i] = 0;
	}
	da_flt_status_pay_set(flt, selkey, TRUE);
}

void filter_status_cat_clear_except(Filter *flt, guint32 selkey)
{
guint i;

	DB( g_print("[filter] cat clear %d\n", flt->gbcat->len) );

	
	for(i=0;i<flt->gbcat->len;i++)
	{
		flt->gbcat->data[i] = 0;
	}

	da_flt_status_cat_set(flt, selkey, TRUE);
	//todo
	//#1824561 don't forget subcat

}


/* = = = = = = = = = = = = = = = = */


void filter_reset(Filter *flt)
{
gint i;

	DB( g_print("\n[filter] default reset all %p\n", flt) );

	for(i=0;i<FLT_GRP_MAX;i++)
	{
		flt->option[i] = 0;
	}

	flt->option[FLT_GRP_DATE] = 1;

	//5.4.2: useless, as it is always changed after all
	//flt->range  = FLT_RANGE_LAST12MONTHS;
	//filter_preset_daterange_set(flt, flt->range, 0);
	flt->range = FLT_RANGE_LAST_30DAYS;
	flt->mindate = GLOBALS->today-30;
	flt->maxdate = GLOBALS->today;

	for(i=0;i<NUM_PAYMODE_MAX;i++)
		flt->paymode[i] = TRUE;

	//reinit the array acc/pay/cat/tag + text
	da_flt_clean(flt);
	da_flt_init(flt);

	//unsaved
	flt->nbdaysfuture = 0;
	flt->type   = FLT_TYPE_ALL;
	flt->status = FLT_STATUS_ALL;
	flt->forceremind = PREFS->showremind;
	flt->forcevoid = PREFS->showvoid;

	*flt->last_tab = '\0';
}


/* TODO: check this : used in rep_time only */

void filter_set_tag_by_id(Filter *flt, guint32 key)
{
Tag *tag;

	DB( g_print("\n[filter] set tag by id\n") );
	
	if(flt->tag)
	{
		g_free(flt->tag);
		flt->tag = NULL;
	}

	//todo change this
	tag = da_tag_get(key);
	if(tag)
	{
		flt->tag = g_strdup(tag->name);
	}
}


static void filter_set_date_bounds(Filter *flt, guint32 kacc)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;

	DB( g_print("\n[filter] set date bounds %p\n", flt) );

	flt->mindate = 0;
	flt->maxdate = 0;

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;
	
		//#1674045 only rely on nosummary
		//if( !(acc->flags & AF_CLOSED) )
		{
		Transaction *txn;
		
			DB( g_print(" collect date for '%s'\n", acc->name) );

			lnk_txn = g_queue_peek_head_link(acc->txn_queue);
			if(lnk_txn) {
				txn = lnk_txn->data;
				if( (kacc == 0) || (txn->kacc == kacc) )
				{
					if( flt->mindate == 0 )
						flt->mindate = txn->date;
					else
						flt->mindate = MIN(flt->mindate, txn->date);
				}
			}

			lnk_txn = g_queue_peek_tail_link(acc->txn_queue);
			if(lnk_txn) {
				txn = lnk_txn->data;
				if( (kacc == 0) || (txn->kacc == kacc) )
				{
					if( flt->maxdate == 0 )
						flt->maxdate = txn->date;
					else
						flt->maxdate = MAX(flt->maxdate, txn->date);
				}
			}

		}
		lnk_acc = g_list_next(lnk_acc);
	}
	
	if( flt->mindate == 0 )
		//changed 5.3
		//flt->mindate = HB_MINDATE;
		flt->mindate = GLOBALS->today - 365;
	
	if( flt->maxdate == 0 )
		//changed 5.3
		//flt->maxdate = HB_MAXDATE;
		flt->maxdate = GLOBALS->today + flt->nbdaysfuture;
	
	g_list_free(lst_acc);
}


gboolean filter_preset_daterange_future_enable(Filter *flt, gint range)
{
gboolean retval = FALSE;

	DB( g_print("\n[filter] range future enabled\n") );

	DB( g_print(" fltrang=%d range=%d\n", flt->range, range) );

	switch( range )
	{
		case FLT_RANGE_THIS_DAY:
		case FLT_RANGE_THIS_WEEK:
		case FLT_RANGE_THIS_FORTNIGHT:
		case FLT_RANGE_THIS_MONTH:
		case FLT_RANGE_THIS_QUARTER:
		case FLT_RANGE_THIS_YEAR:
		case FLT_RANGE_LAST_30DAYS:
		case FLT_RANGE_LAST_60DAYS:
		case FLT_RANGE_LAST_90DAYS:
		case FLT_RANGE_LAST_12MONTHS:
		case FLT_RANGE_LAST_6MONTHS:
			retval = TRUE;
			break;
	}

	//TODO: custom date
	if( range == FLT_RANGE_MISC_ALLDATE )
	{
	GDate *date1, *date2;

		DB( g_print(" eval alldate\n") );
	
		date1 = g_date_new_julian(GLOBALS->today);
		date2 = g_date_new_julian(flt->maxdate);

		if( date2 > date1 && (g_date_get_year(date2) == g_date_get_year(date1)
		 && g_date_get_month(date2) == g_date_get_month(date1)) )
		{
			retval = TRUE;
		}
		g_date_free(date2);
		g_date_free(date1);
	}

	DB( hb_print_date(flt->maxdate , " maxdate ") );
	DB( g_print(" return: %s\n", retval==TRUE ? "yes" : "no") );

	return retval;
}


guint32 filter_get_maxdate_forecast(Filter *filter)
{
guint32 retval = filter->maxdate;

	DB( g_print("\n[filter] get maxdate to forecast\n") );

	if( filter_preset_daterange_future_enable(filter, filter->range) )
	{
	GDate *post_date = g_date_new();

		g_date_set_time_t(post_date, time(NULL));
		g_date_add_months(post_date, PREFS->rep_forecat_nbmonth);
		g_date_set_day(post_date, g_date_get_days_in_month(g_date_get_month(post_date), g_date_get_year(post_date)));
		retval = g_date_get_julian(post_date);
		g_date_free(post_date);
	}

	DB( hb_print_date(retval, "retval:") );

	return retval;
}


//used only in register
void filter_preset_daterange_add_futuregap(Filter *filter, gint nbdays)
{
	DB( g_print("\n[filter] range add future gap\n") );
	
	filter->nbdaysfuture = 0;
	//#1840998 if future active and visible: we should always maxdate to today + nbdays
	if( filter_preset_daterange_future_enable(filter, filter->range) )
	{
	guint32 jforcedmax = GLOBALS->today + nbdays;

		if( filter->maxdate < jforcedmax )
			filter->nbdaysfuture = jforcedmax - filter->maxdate;
		//else
		//	filter->nbdaysfuture = nbdays;

		DB( g_print(" today=%d, tmpmax=%d, nbdays=%d\n final=%d", GLOBALS->today, jforcedmax, nbdays, filter->nbdaysfuture) );
	}
}


void filter_preset_daterange_set(Filter *flt, gint range, guint32 kacc)
{
GDate *tmpdate;
guint32 jtoday, jfiscal;
guint16 month, year, yfiscal, qnum;
GDateWeekday wday;

	DB( g_print("\n[filter] daterange set %p %d\n", flt, range) );

	flt->range = range;
	
	jtoday = GLOBALS->today;

	tmpdate  = g_date_new_julian(jtoday);

	month = g_date_get_month(tmpdate);
	year  = g_date_get_year(tmpdate);
	DB( hb_print_date(jtoday , "today ") );

	qnum = 0;
	yfiscal = year;
	if( range == FLT_RANGE_LAST_QUARTER || range == FLT_RANGE_THIS_QUARTER ||range == FLT_RANGE_NEXT_QUARTER ||
		//#2000834    
	    range == FLT_RANGE_LAST_YEAR || range == FLT_RANGE_THIS_YEAR ||range == FLT_RANGE_NEXT_YEAR
	  )
	{
		g_date_set_dmy(tmpdate, PREFS->fisc_year_day, PREFS->fisc_year_month, year);
		jfiscal = g_date_get_julian(tmpdate);
		DB( hb_print_date(jfiscal, "fiscal") );

		yfiscal = (jtoday >= jfiscal) ? year : year-1;

		if( range == FLT_RANGE_LAST_QUARTER || range == FLT_RANGE_THIS_QUARTER ||range == FLT_RANGE_NEXT_QUARTER )
		{
			g_date_set_dmy(tmpdate, PREFS->fisc_year_day, PREFS->fisc_year_month, yfiscal);
			while( (qnum < 5) && (g_date_get_julian(tmpdate) < jtoday) )
			{
				qnum++;
				g_date_add_months (tmpdate, 3);
			}
			DB( g_print(" qnum: %d\n", qnum ) );
		}
	}
		
	switch( range )
	{
		case FLT_RANGE_LAST_DAY:
			flt->mindate = flt->maxdate = jtoday - 1;
			break;
		case FLT_RANGE_THIS_DAY:
			flt->mindate = flt->maxdate = jtoday;
			break;
		case FLT_RANGE_NEXT_DAY:
			flt->mindate = flt->maxdate = jtoday + 1;
			break;

		case FLT_RANGE_LAST_WEEK:
		case FLT_RANGE_THIS_WEEK:
		case FLT_RANGE_NEXT_WEEK:
			//ISO 8601 from must be monday, to slice in correct weekœ
			wday = g_date_get_weekday(tmpdate);
			g_date_subtract_days (tmpdate, wday-G_DATE_MONDAY);
			if( range == FLT_RANGE_LAST_WEEK )
				g_date_subtract_days(tmpdate, 7);
			else
			if( range == FLT_RANGE_NEXT_WEEK )
				g_date_add_days(tmpdate, 7);

			flt->mindate = g_date_get_julian(tmpdate);
			g_date_add_days(tmpdate, 7);
			flt->maxdate = g_date_get_julian(tmpdate) - 1;
			break;

		case FLT_RANGE_LAST_FORTNIGHT:
		case FLT_RANGE_THIS_FORTNIGHT:
		case FLT_RANGE_NEXT_FORTNIGHT:
			//ISO 8601 from must be monday, to slice in correct week
			wday = g_date_get_weekday(tmpdate);
			g_date_subtract_days (tmpdate, wday - G_DATE_MONDAY);
			if( range == FLT_RANGE_LAST_FORTNIGHT )
				g_date_subtract_days(tmpdate, 14);
			else
			if( range == FLT_RANGE_NEXT_FORTNIGHT )
				g_date_add_days(tmpdate, 14);

			flt->mindate = g_date_get_julian(tmpdate);
			g_date_add_days(tmpdate, 14);
			flt->maxdate = g_date_get_julian(tmpdate) - 1;
			break;

		case FLT_RANGE_LAST_MONTH:
		case FLT_RANGE_THIS_MONTH:
		case FLT_RANGE_NEXT_MONTH:
			g_date_set_dmy(tmpdate, 1, month, year);
			if( range == FLT_RANGE_LAST_MONTH )
				g_date_subtract_months(tmpdate, 1);
			else
			if( range == FLT_RANGE_NEXT_MONTH )
				g_date_add_months(tmpdate, 1);
			flt->mindate = g_date_get_julian(tmpdate);
			month = g_date_get_month(tmpdate);
			year = g_date_get_year(tmpdate);
			g_date_add_days(tmpdate, g_date_get_days_in_month(month, year));
			flt->maxdate = g_date_get_julian(tmpdate) - 1;
			break;

		case FLT_RANGE_LAST_QUARTER:
		case FLT_RANGE_THIS_QUARTER:
		case FLT_RANGE_NEXT_QUARTER:
			g_date_set_dmy(tmpdate, PREFS->fisc_year_day, PREFS->fisc_year_month, yfiscal);
			if( range == FLT_RANGE_LAST_QUARTER )
				g_date_subtract_months(tmpdate, 3);
			else
			if( range == FLT_RANGE_NEXT_QUARTER )
				g_date_add_months(tmpdate, 3);
			g_date_add_months(tmpdate, 3 * (qnum-1) );
			flt->mindate = g_date_get_julian(tmpdate);
			g_date_add_months(tmpdate, 3);
			flt->maxdate = g_date_get_julian(tmpdate) - 1;
			break;

		case FLT_RANGE_LAST_YEAR:
		case FLT_RANGE_THIS_YEAR:
		case FLT_RANGE_NEXT_YEAR:
			g_date_set_dmy(tmpdate, PREFS->fisc_year_day, PREFS->fisc_year_month, yfiscal);
			if( range == FLT_RANGE_LAST_YEAR )
				g_date_subtract_years(tmpdate, 1);
			else
			if( range == FLT_RANGE_NEXT_YEAR )
				g_date_add_years(tmpdate, 1);
			flt->mindate = g_date_get_julian(tmpdate);
			g_date_add_years (tmpdate, 1);
			flt->maxdate = g_date_get_julian(tmpdate) - 1;
			break;

		case FLT_RANGE_LAST_30DAYS:
			flt->mindate = jtoday - 30;
			flt->maxdate = jtoday;
			break;

		case FLT_RANGE_LAST_60DAYS:
			flt->mindate = jtoday - 60;
			flt->maxdate = jtoday;
			break;

		case FLT_RANGE_LAST_90DAYS:
			flt->mindate = jtoday - 90;
			flt->maxdate = jtoday;
			break;

		case FLT_RANGE_LAST_12MONTHS:
			g_date_set_julian (tmpdate, jtoday);
			g_date_subtract_months(tmpdate, 12);
			flt->mindate = g_date_get_julian(tmpdate);
			flt->maxdate = jtoday;
			break;

		case FLT_RANGE_LAST_6MONTHS:
			g_date_set_julian (tmpdate, jtoday);
			g_date_subtract_months(tmpdate, 6);
			flt->mindate = g_date_get_julian(tmpdate);
			flt->maxdate = jtoday;
			break;

		// case FLT_RANGE_MISC_CUSTOM:
			//nothing to do
			
		case FLT_RANGE_MISC_ALLDATE:
			filter_set_date_bounds(flt, kacc);
			break;
		case FLT_RANGE_MISC_30DAYS:
			flt->mindate = jtoday - 30;
			flt->maxdate = jtoday + 30;		
			break;
	}
	g_date_free(tmpdate);
	
	DB( hb_print_date(flt->mindate , " min ") );
	DB( hb_print_date(flt->maxdate , " max ") );
}


void filter_preset_type_set(Filter *flt, gint type, gint mode)
{

	DB( g_print("\n[filter] preset type set\n") );

	flt->option[FLT_GRP_TYPE] = FLT_OFF;
	flt->type = type;
	flt->typ_exp = FALSE;
	flt->typ_inc = FALSE;
	flt->typ_xfr = FALSE;

	if( type != FLT_TYPE_ALL )
	{
		//FLT_INCLUDE / FLT_EXCLUDE
		flt->option[FLT_GRP_TYPE] = mode;
		switch(type)
		{
			case FLT_TYPE_EXPENSE:
				flt->typ_exp = TRUE;
				break;
			case FLT_TYPE_INCOME:
				flt->typ_inc = TRUE;
				break;
			case FLT_TYPE_INTXFER:
				flt->typ_xfr = TRUE;
				break;
		}
	}

}


/* = = = = = = = = = = = = = = = = */


static void filter_group_import_keys(Filter *flt, gint group, gchar *text)
{
gchar **str_array;
gint len, i;

	DB( g_print(" import keys '%s'\n", text) );

	str_array = g_strsplit (text, ",", -1);
	len = g_strv_length( str_array );
	for(i=0;i<len;i++)
	{
	guint32 key = atoi(str_array[i]);

		
		switch(group)
		{
			case FLT_GRP_ACCOUNT:
				if( da_acc_get(i) )
				{
					da_flt_status_acc_set(flt, key, TRUE);
					DB( g_print(" set acc='%d' to true\n", key) );
				}
				break;
			case FLT_GRP_PAYEE:
				if( da_pay_get(i) )
				{
					da_flt_status_pay_set(flt, key, TRUE);
					DB( g_print(" set pay='%d' to true\n", key) );
				}
				break;
			case FLT_GRP_CATEGORY:
				if( da_cat_get(i) )
				{
					da_flt_status_cat_set(flt, key, TRUE);
					DB( g_print(" set cat='%d' to true\n", key) );
				}
				break;
			/*case FLT_GRP_TAG:
				da_flt_status_tag_set(flt, key, TRUE);
				break;*/
		}
	}
	g_strfreev(str_array);
}


void filter_group_import(Filter *flt, gint group, const gchar *text)
{
gchar **str_array;
gint len;


	switch(group)
	{
		case FLT_GRP_DATE:
			str_array = g_strsplit (text, "|", 4);
			len = g_strv_length( str_array );

			if(len == 2)
			{
				flt->option[group] = atoi(str_array[0]);
				flt->range = atoi(str_array[1]);
			}
			if(len == 4 && flt->range == FLT_RANGE_MISC_CUSTOM)
			{
				flt->mindate = atoi(str_array[2]);
				flt->maxdate = atoi(str_array[3]);
			}
			g_strfreev(str_array);
			break;
		
		case FLT_GRP_ACCOUNT:
			str_array = g_strsplit (text, "|", 2);
			len = g_strv_length( str_array );

			if(len == 2)
			{
				flt->option[group] = atoi(str_array[0]);
				filter_group_import_keys(flt, FLT_GRP_ACCOUNT, str_array[1]);
			}
			g_strfreev(str_array);
			break;

		case FLT_GRP_PAYEE:
			str_array = g_strsplit (text, "|", 2);
			len = g_strv_length( str_array );

			if(len == 2)
			{
				flt->option[group] = atoi(str_array[0]);
				filter_group_import_keys(flt, FLT_GRP_PAYEE, str_array[1]);
			}
			g_strfreev(str_array);
			break;

		case FLT_GRP_CATEGORY:
			str_array = g_strsplit (text, "|", 2);
			len = g_strv_length( str_array );

			if(len == 2)
			{
				flt->option[group] = atoi(str_array[0]);
				filter_group_import_keys(flt, FLT_GRP_CATEGORY, str_array[1]);
			}
			g_strfreev(str_array);
			break;

			
			
	}
}



gchar *filter_group_export(Filter *flt, gint group)
{
gchar *retval = NULL;
GString *node;
guint i;

	switch(group)
	{
		case FLT_GRP_DATE:
			if(flt->option[group] > 0)
			{
				//TODO: maybe always keep 4 values here
				if(flt->range == FLT_RANGE_MISC_CUSTOM)
					retval = g_strdup_printf("%d|%d|%d|%d", flt->option[group], flt->range, flt->mindate, flt->maxdate);
				else
					retval = g_strdup_printf("%d|%d", flt->option[group], flt->range);
			}
			break;
		case FLT_GRP_ACCOUNT:
			if(flt->option[group] > 0)
			{
				node = g_string_sized_new(flt->gbacc->len);
				g_string_append_printf(node, "%d|", flt->option[group]);
				DB( g_printf("acc len:%d\n", flt->gbacc->len) );
				for(i=0;i<flt->gbacc->len;i++)
				{
					if( da_acc_get(i) )
						if( da_flt_status_acc_get(flt, i) == TRUE )
							g_string_append_printf(node, "%d,", i);
				}
				g_string_erase(node, node->len-1, 1);
				retval = node->str;
				g_string_free(node, FALSE);
				DB( g_printf(" '%s'\n", retval) );
				node = NULL;
			}
			break;
		case FLT_GRP_PAYEE:
			if(flt->option[group] > 0)
			{
				node = g_string_sized_new(flt->gbpay->len);
				g_string_append_printf(node, "%d|", flt->option[group]);
				DB( g_printf("pay len:%d\n", flt->gbpay->len) );
				for(i=0;i<flt->gbpay->len;i++)
				{
					if( da_pay_get(i) )
						if( da_flt_status_pay_get(flt, i) == TRUE )
							g_string_append_printf(node, "%d,", i);
				}
				g_string_erase(node, node->len-1, 1);
				retval = node->str;
				g_string_free(node, FALSE);
				DB( g_printf(" '%s'\n", retval) );
				node = NULL;
			}
			break;
		case FLT_GRP_CATEGORY:
			if(flt->option[group] > 0)
			{
				node = g_string_sized_new(flt->gbcat->len);
				g_string_append_printf(node, "%d|", flt->option[group]);
				DB( g_printf("cat len:%d\n", flt->gbcat->len) );
				for(i=0;i<flt->gbcat->len;i++)
				{
					if( da_cat_get(i) )
						if( da_flt_status_cat_get(flt, i) == TRUE )
							g_string_append_printf(node, "%d,", i);
				}
				g_string_erase(node, node->len-1, 1);
				retval = node->str;
				g_string_free(node, FALSE);
				DB( g_printf(" '%s'\n", retval) );
				node = NULL;
			}			break;
		case FLT_GRP_TAG:
			if(flt->option[group] > 0)
			{
		

			}
			break;
		case FLT_GRP_TEXT:
			if(flt->option[group] > 0)
			{
		

			}
			break;
		case FLT_GRP_AMOUNT:
			if(flt->option[group] > 0)
			{
		

			}
			break;
		case FLT_GRP_PAYMODE:
			if(flt->option[group] > 0)
			{
		

			}
			break;
		case FLT_GRP_STATUS:
			if(flt->option[group] > 0)
			{
		

			}
			break;
		case FLT_GRP_TYPE:
			if(flt->option[group] > 0)
			{
		

			}
			break;
	}
	return retval;
}



void filter_preset_status_set(Filter *flt, gint status)
{

	DB( g_print("\n[filter] preset status set\n") );
	
	/* any status */
	//#1991459 dont reset type
	//flt->option[FLT_GRP_TYPE] = 0;
	flt->option[FLT_GRP_STATUS] = 0;
	flt->option[FLT_GRP_CATEGORY] = 0;

	//#1991459 dont reset type
	//flt->type   = FLT_TYPE_ALL;
	flt->status = status;
	flt->sta_non = FALSE;
	flt->sta_clr = FALSE;
	flt->sta_rec = FALSE;
	//#1602835 fautly set
	//flt->forceadd = TRUE;
	//flt->forcechg = TRUE;

	//#1860356 keep widget active_id
	//#1873324 register status quick filter do not reset
	//note: status revert to UNRECONCILED here is normal if PREFS->hidereconciled=TRUE
	//flt->rawstatus = status;

	if( status != FLT_STATUS_ALL )
	{
		switch( status )
		{
			case FLT_STATUS_UNCATEGORIZED:
				//#1991459 dont reset type: here to hide xfer txn
				//flt->option[FLT_GRP_TYPE] = 2;
				//flt->type = FLT_TYPE_INTXFER;
				flt->option[FLT_GRP_CATEGORY] = 1;
				filter_status_cat_clear_except(flt, 0);
				
				DB( my_debug_garray("cat", flt->gbcat) );
				
				break;

			case FLT_STATUS_UNRECONCILED:
				flt->option[FLT_GRP_STATUS] = 1;
				flt->sta_non = TRUE;
				flt->sta_clr = TRUE;
				break;

			case FLT_STATUS_UNCLEARED:
				flt->option[FLT_GRP_STATUS] = 1;
				flt->sta_non = TRUE;
				break;

			case FLT_STATUS_RECONCILED:
				flt->option[FLT_GRP_STATUS] = 1;
				flt->sta_rec = TRUE;
				break;

			case FLT_STATUS_CLEARED:
				flt->option[FLT_GRP_STATUS] = 1;
				flt->sta_clr = TRUE;
				break;
		}
	}

}


gchar *filter_daterange_text_get(Filter *flt)
{
gchar buffer1[128];
gchar buffer2[128];
gchar buffer3[128];
GDate *date;
gchar *retval = NULL;

	DB( g_print("\n[filter] daterange text get\n") );
	
	date = g_date_new_julian(flt->mindate);
	g_date_strftime (buffer1, 128-1, PREFS->date_format, date);
	
	g_date_set_julian(date, flt->maxdate);
	g_date_strftime (buffer2, 128-1, PREFS->date_format, date);
	
	if( flt->nbdaysfuture > 0 )
	{
		g_date_set_julian(date, flt->maxdate + flt->nbdaysfuture);
		g_date_strftime (buffer3, 128-1, PREFS->date_format, date);
		retval = g_strdup_printf("%s — <s>%s</s> %s", buffer1, buffer2, buffer3);
	}
	else
		retval = g_strdup_printf("%s — %s", buffer1, buffer2);
	
	g_date_free(date);

	//return g_strdup_printf(_("<i>from</i> %s <i>to</i> %s — "), buffer1, buffer2);
	return retval;
}


/* = = = = = = = = = = = = = = = = */


/* used for quicksearch text into transaction */
gboolean filter_tpl_search_match(gchar *needle, Archive *arc)
{
gboolean retval = FALSE;
Payee *payitem;

	DB( g_print("\n[filter] tpl search match\n") );
	
	//#1668036 always try match on txn memo first
	if(arc->memo)
	{
		retval |= hb_string_utf8_strstr(arc->memo, needle, FALSE);
	}
	if(retval) goto end;
	
	//#1509485
	if(arc->flags & OF_SPLIT)
	{
	guint count, i;
	Split *split;

		count = da_splits_length(arc->splits);
		for(i=0;i<count;i++)
		{
		gint tmpinsert = 0;
	
			split = da_splits_get(arc->splits, i);
			tmpinsert = hb_string_utf8_strstr(split->memo, needle, FALSE);
			retval |= tmpinsert;
			if( tmpinsert )
				break;
		}
	}
	if(retval) goto end;
	
	if(arc->info)
	{
		retval |= hb_string_utf8_strstr(arc->info, needle, FALSE);
	}
	if(retval) goto end;

	payitem = da_pay_get(arc->kpay);
	if(payitem)
	{
		retval |= hb_string_utf8_strstr(payitem->name, needle, FALSE);
	}
	if(retval) goto end;

	//#1741339 add quicksearch for amount
	{
	gchar formatd_buf[G_ASCII_DTOSTR_BUF_SIZE];
	
		hb_strfnum(formatd_buf, G_ASCII_DTOSTR_BUF_SIZE-1, arc->amount, GLOBALS->kcur, FALSE);
		retval |= hb_string_utf8_strstr(formatd_buf, needle, FALSE);
	}

	
end:
	return retval;
}


static gboolean filter_txn_tag_match(Filter *flt, guint32 *tags)
{
guint count, i;
gboolean retval = FALSE;

	if(flt->gbtag == NULL)
		return FALSE;

	count = tags_count(tags);
	//if no tag in txn just check (no tag) is set
	if( count == 0 )
	{
		retval = da_flt_status_tag_get(flt, 0);
		goto end;
	}

	DB( g_print("\n[filter] tnx tag match\n") );

		//debug loop */
		#if MYDEBUG == 1
		g_print(" dbg gbtag %d elt\n", flt->gbtag->len);
		for(i=0;i<flt->gbtag->len;i++)
		{
			g_print("[%d]=%d ", i, flt->gbtag->data[i]);
		}	
		g_print("\n\n");
		#endif

	/* loop on any tags */
	DB( g_print(" loop txn tags %d\n", count) );
	for(i=0;i<count;i++)
	{
		retval = da_flt_status_tag_get(flt, tags[i]);

		DB( g_print(" %d ktag=%d, sel=%d\n", i, tags[i], retval) );

		if(retval == TRUE)	/* break once a tag match :: OR */
			break;
	}

end:
	return retval;
}


/* used for quicksearch text into transaction */
gboolean filter_txn_search_match(gchar *needle, Transaction *txn, gint flags)
{
gboolean retval = FALSE;
Payee *payitem;
Category *catitem;
gchar *tags;

	DB( g_print("\n[filter] tnx search match\n") );
	
	if(flags & FLT_QSEARCH_MEMO)
	{
		//#1668036 always try match on txn memo first
		if(txn->memo)
		{
			retval |= hb_string_utf8_strstr(txn->memo, needle, FALSE);
		}
		if(retval) goto end;
		
		//#1509485
		if(txn->flags & OF_SPLIT)
		{
		guint count, i;
		Split *split;

			count = da_splits_length(txn->splits);
			for(i=0;i<count;i++)
			{
			gint tmpinsert = 0;
		
				split = da_splits_get(txn->splits, i);
				tmpinsert = hb_string_utf8_strstr(split->memo, needle, FALSE);
				retval |= tmpinsert;
				if( tmpinsert )
					break;
			}
		}
		if(retval) goto end;
	}
	
	if(flags & FLT_QSEARCH_INFO)
	{
		if(txn->info)
		{
			retval |= hb_string_utf8_strstr(txn->info, needle, FALSE);
		}
		if(retval) goto end;
	}

	if(flags & FLT_QSEARCH_PAYEE)
	{
		payitem = da_pay_get(txn->kpay);
		if(payitem)
		{
			retval |= hb_string_utf8_strstr(payitem->name, needle, FALSE);
		}
		if(retval) goto end;
	}

	if(flags & FLT_QSEARCH_CATEGORY)
	{
		//#1509485
		if(txn->flags & OF_SPLIT)
		{
		guint count, i;
		Split *split;

			count = da_splits_length(txn->splits);
			for(i=0;i<count;i++)
			{
			gint tmpinsert = 0;
				
				split = da_splits_get(txn->splits, i);
				catitem = da_cat_get(split->kcat);
				if(catitem)
				{
					tmpinsert = hb_string_utf8_strstr(catitem->fullname, needle, FALSE);
					retval |= tmpinsert;
				}

				if( tmpinsert )
					break;
			}
		}
		else
		{
			catitem = da_cat_get(txn->kcat);
			if(catitem)
			{
				retval |= hb_string_utf8_strstr(catitem->fullname, needle, FALSE);
			}
		}
		if(retval) goto end;
	}
	
	if(flags & FLT_QSEARCH_TAGS)
	{
		//TODO: chnage this

		tags = tags_tostring(txn->tags);
		if(tags)
		{
			retval |= hb_string_utf8_strstr(tags, needle, FALSE);
		}
		g_free(tags);
		if(retval) goto end;
	}

	//#1741339 add quicksearch for amount
	if(flags & FLT_QSEARCH_AMOUNT)
	{
	gchar formatd_buf[G_ASCII_DTOSTR_BUF_SIZE];

		DB( g_print(" needle='%s' txnamt='%s'\n", needle, formatd_buf) );
		
		hb_strfnum(formatd_buf, G_ASCII_DTOSTR_BUF_SIZE-1, txn->amount, txn->kcur, FALSE);
		retval |= hb_string_utf8_strstr(formatd_buf, needle, FALSE);
	}

	
end:
	return retval;
}


gint filter_acc_match(Filter *flt, Account *acc)
{
gboolean status;
gint insert = 1;

/* account */
	if(flt->option[FLT_GRP_ACCOUNT])
	{
		status = da_flt_status_acc_get(flt, acc->key);
		insert = ( status == TRUE ) ? 1 : 0;
		if(flt->option[FLT_GRP_ACCOUNT] == 2) insert ^= 1;
	}
	
	return(insert);
}


gint filter_txn_match(Filter *flt, Transaction *txn)
{
gboolean status;
gint insert;

	//DB( g_print("\n[filter] txn match\n") );

	insert = 1;

/*** start filtering ***/

/* date */
	if(flt->option[FLT_GRP_DATE])
	{
		insert = ( (txn->date >= flt->mindate) && (txn->date <= (flt->maxdate + flt->nbdaysfuture) ) ) ? 1 : 0;
		if(flt->option[FLT_GRP_DATE] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* account */
	if(flt->option[FLT_GRP_ACCOUNT])
	{
		status = da_flt_status_acc_get(flt, txn->kacc);
		insert = ( status == TRUE ) ? 1 : 0;
		if(flt->option[FLT_GRP_ACCOUNT] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* payee */
	if(flt->option[FLT_GRP_PAYEE])
	{
		status = da_flt_status_pay_get(flt, txn->kpay);
		insert = ( status == TRUE ) ? 1 : 0;
		if(flt->option[FLT_GRP_PAYEE] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* category */
	if(flt->option[FLT_GRP_CATEGORY])
	{
		if(txn->flags & OF_SPLIT)
		{
		guint count, i;
		Split *split;

			insert = 0;	 //fix: 1151259
			count = da_splits_length(txn->splits);
			for(i=0;i<count;i++)
			{
			gint tmpinsert = 0;
				
				split = da_splits_get(txn->splits, i);
				status = da_flt_status_cat_get(flt, split->kcat);
				tmpinsert = ( status == TRUE ) ? 1 : 0;
				if(flt->option[FLT_GRP_CATEGORY] == 2) tmpinsert ^= 1;
				insert |= tmpinsert;
			}
		}
		else
		{
			status = da_flt_status_cat_get(flt, txn->kcat);
			insert = ( status == TRUE ) ? 1 : 0;
			if(flt->option[FLT_GRP_CATEGORY] == 2) insert ^= 1;
		}
	}
	if(!insert) goto end;

/* tag */
	if(flt->option[FLT_GRP_TAG]) {
		status = filter_txn_tag_match(flt, txn->tags);
		insert = ( status == TRUE ) ? 1 : 0;
		if(flt->option[FLT_GRP_TAG] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* type */
	if(flt->option[FLT_GRP_TYPE])
	{
	gint typ1 = ( (flt->typ_exp == TRUE) && !(txn->flags & (OF_INCOME|OF_INTXFER)) ) ? 1 : 0;
	gint typ2 = ( (flt->typ_inc == TRUE) && (txn->flags & (OF_INCOME)) ) ? 1 : 0;
	gint typ3 = ( (flt->typ_xfr == TRUE) && (txn->flags & (OF_INTXFER)) ) ? 1 : 0;

		insert = typ1 || typ2 || typ3;
		if(flt->option[FLT_GRP_TYPE] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* status */
	if(flt->option[FLT_GRP_STATUS])
	{
	gint sta1 = ( (flt->sta_non == TRUE) && (txn->status == TXN_STATUS_NONE) ) ? 1 : 0;
	gint sta2 = ( (flt->sta_clr == TRUE) && (txn->status == TXN_STATUS_CLEARED) ) ? 1 : 0;
	gint sta3 = ( (flt->sta_rec == TRUE) && (txn->status == TXN_STATUS_RECONCILED) ) ? 1 : 0;

		insert = sta1 || sta2 || sta3;
		if(flt->option[FLT_GRP_STATUS] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* paymode */
	if(flt->option[FLT_GRP_PAYMODE]) {
		insert = ( flt->paymode[txn->paymode] == TRUE) ? 1 : 0;
		if(flt->option[FLT_GRP_PAYMODE] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* amount */
	if(flt->option[FLT_GRP_AMOUNT])
	{
		insert = ( (txn->amount >= flt->minamount) && (txn->amount <= flt->maxamount) ) ? 1 : 0;

		if(flt->option[FLT_GRP_AMOUNT] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* info/memo */
	if(flt->option[FLT_GRP_TEXT])
	{
	gint insert1, insert2;

		insert1 = insert2 = 0;
		if(flt->info)
		{
			if(txn->info)
			{
				insert1 = hb_string_utf8_strstr(txn->info, flt->info, flt->exact);
			}
		}
		else
			insert1 = 1;

		if(flt->memo)
		{
			//#1668036 always try match on txn memo first
			if(txn->memo)
			{
				insert2 = hb_string_utf8_strstr(txn->memo, flt->memo, flt->exact);
			}

			if( (insert2 == 0) && (txn->flags & OF_SPLIT) )
			{
			guint count, i;
			Split *split;

				count = da_splits_length(txn->splits);
				for(i=0;i<count;i++)
				{
				gint tmpinsert = 0;
			
					split = da_splits_get(txn->splits, i);
					tmpinsert = hb_string_utf8_strstr(split->memo, flt->memo, flt->exact);
					insert2 |= tmpinsert;
					if( tmpinsert )
						break;
				}
			}
		}
		else
			insert2 = 1;

		insert = insert1 && insert2 ? 1 : 0;

		if(flt->option[FLT_GRP_TEXT] == 2) insert ^= 1;

	}
	//if(!insert) goto end;

end:
	/* force display */
	if(!insert)
	{
		if( ((flt->forceadd == TRUE) && (txn->flags & OF_ADDED))
		 || ((flt->forcechg == TRUE) && (txn->flags & OF_CHANGED))
		  )
		insert = 1;
	}

	//#1999186 pref void/remind to false do not work
	if( txn->status == TXN_STATUS_REMIND )
	{
		insert = flt->forceremind;
	}
	
	if( txn->status == TXN_STATUS_VOID )
	{
		insert = flt->forcevoid;
	}
	
//	DB( g_print(" %d :: %d :: %d\n", flt->mindate, txn->date, flt->maxdate) );
//	DB( g_print(" [%d] %s => %d (%d)\n", txn->account, txn->memo, insert, count) );
	return(insert);
}

