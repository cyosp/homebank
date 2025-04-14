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
#include "hb-report.h"

/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/

#define DB1(x);
//#define DB1(x) (x);

#define DB2(x);
//#define DB2(x) (x);

#define DB3(x);
//#define DB3(x) (x);

//date debug
#define DBD(x);
//#define DBD(x) (x);



/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


extern gchar *CYA_ABMONTHS[];


static void _datatable_init_count_key_row(DataTable *dt, gint grpby, Filter *flt);


/* = = = = = = = = = = = = = = = = = = = = */
/* CarCost */

void da_vehiclecost_free(CarCost *item)
{
	if(item != NULL)
	{
		g_free(item);
	}
}


CarCost *da_vehiclecost_malloc(void)
{
	return g_malloc0(sizeof(CarCost));
}

void da_vehiclecost_destroy(GList *list)
{
GList *tmplist = g_list_first(list);

	while (tmplist != NULL)
	{
	CarCost *item = tmplist->data;
		da_vehiclecost_free(item);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(list);
}



/* = = = = = = = = = = = = = = = = = = = = */

static void da_datacol_free(DataCol *col)
{
	if(col)
	{
		g_free(col->label);
		g_free(col->xlabel);
		g_free(col->misclabel);
		//5.9 leak
		g_free(col);
	}
}


static DataCol *da_datacol_malloc(void)
{
	return g_malloc0(sizeof(DataCol));
}


static void da_datarow_free(DataRow *row)
{
	if(row)
	{
		g_free(row->label);
		g_free(row->xlabel);
		g_free(row->misclabel);
		g_free(row->colexp);
		g_free(row->colinc);
		//5.9 leak
		g_free(row);
	}
}


static DataRow *da_datarow_malloc(guint32 nbcol)
{
DataRow *row;

	row = g_malloc0(sizeof(DataRow));
	if(!row)
		return NULL;

	row->colexp = g_malloc0((nbcol) * sizeof(gdouble));
	row->colinc = g_malloc0((nbcol) * sizeof(gdouble));
	row->nbcols = nbcol;
	//5.9 check
	if( !row->colexp || !row->colinc )
	{
		da_datarow_free(row);
		return NULL;
	}
	return row;
}


//hub-reptime/hub-reptotal/repstats
void da_datatable_free(DataTable *dt)
{
guint i;

	DB1( g_print("\n[report] da_datatable_free: %p\n", dt) );

	if(dt != NULL)
	{
		DB1( g_print(" free keylist\n") );
		g_free(dt->keylist);
		dt->keylist = NULL;

		DB1( g_print(" free datacol\n") );
		for(i=0;i<dt->nbcols;i++)
		{
			da_datacol_free(dt->cols[i]);
		}
		g_free(dt->cols);
		dt->cols = NULL;		

		DB1( g_print(" free datarow\n") );
		for(i=0;i<dt->nbrows;i++)
		{
			da_datarow_free(dt->rows[i]);
		}
		g_free(dt->rows);
		dt->rows = NULL;

		DB1( g_print(" free total datarow\n") );
		da_datarow_free(dt->totrow);
		dt->totrow = NULL;

		DB1( g_print(" free keyindex\n") );
		g_free(dt->keyindex);
		dt->keyindex = NULL;

		DB1( g_print(" free datatable\n") );
		g_free(dt);
		
		DB1( g_print(" end free\n") );
	}
}


static DataTable *da_datatable_malloc(gint grpby, gint intvl, Filter *flt)
{
DataTable *dt;
gboolean okcols, okrows;
guint i;

	DB1( g_print("\n[report] da_datatable_malloc: %p\n", dt) );
	
	dt = g_malloc0(sizeof(DataTable));
	if(!dt)
		return NULL;

	_datatable_init_count_key_row(dt, grpby, flt);
	dt->nbcols = report_interval_count(intvl, flt->mindate, flt->maxdate);
	//dt->nbkeys = maximum keys for acc/pay/cat/tag
	//dt->nbrows = nb of items
	//dt->nbcols = nb of cols

	DB1( g_print(" grpby:%d\n intvl:%d\n maxk:%d\n rows:%d\n cols:%d\n", grpby, intvl, dt->nbkeys, dt->nbrows, dt->nbcols) );

	DB1( g_print(" alloc %d keyindex\n", dt->nbkeys) );
	dt->keyindex = g_malloc0(dt->nbkeys * sizeof(gpointer));
	//ordered list to insert cat before subcat
	DB1( g_print(" alloc %d keylist\n", dt->nbrows) );
	dt->keylist = g_malloc0( dt->nbrows * sizeof(guint32) );

	DB1( g_print(" alloc %d row vector\n", dt->nbrows) );
	dt->rows = g_malloc0(dt->nbrows * sizeof(gpointer));

	DB1( g_print(" alloc %d col vector\n", dt->nbcols) );
	dt->cols = g_malloc0(dt->nbcols * sizeof(gpointer));

	DB1( g_print(" alloc 1 total row\n") );
	dt->totrow = da_datarow_malloc(dt->nbcols);

	//5.9 check
	if( !dt->keyindex || !dt->keylist || !dt->rows || !dt->cols || !dt->totrow )
	{
		da_datatable_free(dt);
		return NULL;
	}

	okcols = okrows = TRUE;
	DB1( g_print(" alloc %d rows\n", dt->nbrows) );
	for(i=0;i<dt->nbrows;i++)
	{
	DataRow *dr = da_datarow_malloc(dt->nbcols);

		//5.9 check
		if( !dr ) { okcols = FALSE; break; }
		dt->rows[i] = dr;
	}

	DB1( g_print(" alloc %d cols\n", dt->nbcols) );
	for(i=0;i<dt->nbcols;i++)
	{
	DataCol *dc = da_datacol_malloc();

		//5.9 check
		if( !dc ) { okrows = FALSE; break; }
		dt->cols[i] = dc;
	}

	if( !okcols || !okrows )
	{
		da_datatable_free(dt);
		return NULL;
	}

	return dt;
}


/* = = = = = = = = = = = = = = = = = = = = */


// slide the date to monday of the week
static void _hb_date_clamp_iso8601(GDate *date)
{
GDateWeekday wday;

	//ISO 8601 from must be monday, to slice in correct week
	wday = g_date_get_weekday(date);
	g_date_subtract_days (date, wday - G_DATE_MONDAY);
}


// slide the date to start quarter
static void _hb_date_clamp_quarter_start(GDate *date)
{
guint quarternum = ((g_date_get_month(date)-1)/3)+1;

	DBD( g_print("  init=%02d/%d > Q%d\n", g_date_get_month(date), g_date_get_year(date), quarternum) );
	g_date_set_day(date, 1);
	g_date_set_month(date, ((quarternum-1)*3)+1);
	DBD( g_print("  start=%02d/%d\n", g_date_get_month(date), g_date_get_year(date)) );
}


// slide the date to start halfyear
static void _hb_date_clamp_halfyear_start(GDate *date)
{
guint halfyearnum = ((g_date_get_month(date)-1)/6)+1;

	DBD( g_print("  init=%02d/%d > H%d\n", g_date_get_month(date), g_date_get_year(date), halfyearnum) );
	g_date_set_day(date, 1);
	g_date_set_month(date, ((halfyearnum-1)*6)+1);
	DBD( g_print("  start=%02d/%d\n", g_date_get_month(date), g_date_get_year(date)) );
}


static guint _date_in_week(guint32 from, guint32 opedate, guint days)
{
GDate *date1, *date2;
gint pos;

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	DBD( g_print(" from=%d %02d-%02d-%04d ", 
		g_date_get_weekday(date1), g_date_get_day(date1), g_date_get_month(date1), g_date_get_year(date1)) );

	//#1915643 week iso 8601
	_hb_date_clamp_iso8601(date1);
	pos = (opedate - g_date_get_julian(date1)) / days;

	DBD( g_print(" shifted=%d %02d-%02d-%04d pos=%d\n", 
		g_date_get_weekday(date1), g_date_get_day(date1), g_date_get_month(date1), g_date_get_year(date1), pos) );


	g_date_free(date2);
	g_date_free(date1);

	return(pos);
}


/*
** return the week list position correponding to the passed date
*/
static guint DateInWeek(guint32 from, guint32 opedate)
{
	return _date_in_week(from, opedate, 7);
}	


/*
** return the FortNight list position correponding to the passed date
*/
static guint DateInFortNight(guint32 from, guint32 opedate)
{
	return _date_in_week(from, opedate, 14);
}	


/*
** return the month list position correponding to the passed date
*/
static guint DateInMonth(guint32 from, guint32 opedate)
{
GDate *date1, *date2;
guint pos;

	DBD( g_print("DateInMonth %d ,%d\n", from, opedate) );

	//todo
	// this return sometimes -1, -2 which is wrong

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	pos = ((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1);

	DBD( g_print(" from=%d-%d ope=%d-%d => %d\n", g_date_get_month(date1), g_date_get_year(date1), g_date_get_month(date2), g_date_get_year(date2), pos) );

	g_date_free(date2);
	g_date_free(date1);

	return(pos);
}



//for fiscal sub gap between 1st fiscal and 1/1/year

//int quarterNumber = (date.Month-1)/3+1;
//DateTime firstDayOfQuarter = new DateTime(date.Year, (quarterNumber-1)*3+1,1);
//DateTime lastDayOfQuarter = firstDayOfQuarter.AddMonths(3).AddDays(-1);

static guint DateInQuarter(guint32 from, guint32 opedate)
{
GDate *date1, *date2;
guint pos;

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	//#1758532 slide quarter start
	_hb_date_clamp_quarter_start(date1);
	
	pos = (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/3;

	DBD( g_print("-- from=%02d/%d ope=%02d/%d => pos=%d\n", g_date_get_month(date1), g_date_get_year(date1), g_date_get_month(date2), g_date_get_year(date2), pos) );

	g_date_free(date2);
	g_date_free(date1);

	return(pos);
}


static guint DateInHalfYear(guint32 from, guint32 opedate)
{
GDate *date1, *date2;
guint pos;

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	//#2034618 slide halfyear start
	_hb_date_clamp_halfyear_start(date1);

	pos = (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/6;

	DBD( g_print(" from=%d-%d ope=%d-%d => %d\n", g_date_get_month(date1), g_date_get_year(date1), g_date_get_month(date2), g_date_get_year(date2), pos) );

	g_date_free(date2);
	g_date_free(date1);
	
	return(pos);
}


/*
** return the year list position correponding to the passed date
*/
static guint DateInYear(guint32 from, guint32 opedate)
{
GDate *date;
guint year_from, year_ope, pos;

	date = g_date_new_julian(from);
	year_from = g_date_get_year(date);

	g_date_set_julian(date, opedate);
	year_ope = g_date_get_year(date);
	g_date_free(date);

	pos = year_ope - year_from;

	DBD( g_print(" from=%d ope=%d => %d\n", year_from, year_ope, pos) );

	return(pos);
}


static guint32 _datatable_interval_get_jdate(gint intvl, guint32 jfrom, gint idx)
{
GDate *date = g_date_new_julian(jfrom);
GDateWeekday wday;
guint32 jdate;

	switch(intvl)
	{
		case REPORT_INTVL_DAY:
			g_date_add_days(date, idx);
			break;
		case REPORT_INTVL_WEEK:
			g_date_add_days(date, idx*7);
			//#1915643 week iso 8601
			//ISO 8601 from must be monday, to slice in correct week
			wday = g_date_get_weekday(date);
			g_date_subtract_days (date, wday-G_DATE_MONDAY);
			g_date_add_days (date, G_DATE_WEDNESDAY);
			break;
		//#2000290
		case REPORT_INTVL_FORTNIGHT:
			_hb_date_clamp_iso8601(date);
			g_date_add_days(date, idx*14);
			break;
		case REPORT_INTVL_MONTH:
			g_date_add_months(date, idx);
			break;
		case REPORT_INTVL_QUARTER:
			g_date_add_months(date, idx*3);
			break;
		case REPORT_INTVL_HALFYEAR:
			g_date_add_months(date, idx*6);
			break;
		case REPORT_INTVL_YEAR:
			g_date_add_years(date, idx);
			break;
	}

	jdate = g_date_get_julian(date);

	g_date_free(date);

	return jdate;
}


/* = = = = = = = = = = = = = = = = = = = = */

static void datatable_set_keylist(DataTable *dt, guint32 idx, guint32 key)
{
	if( idx < dt->nbrows )
		dt->keylist[idx] = key;
	else
		g_warning("datatable invalid set keylist %d of %d", idx , dt->nbrows);
}

static void datatable_set_keyindex(DataTable *dt, guint32 key, guint32 idx)
{
	if( key < dt->nbkeys )
		dt->keyindex[key] = idx;
	else
		g_warning("datatable invalid set keyindex %d of %d", key , dt->nbkeys);
}

static void _datatable_init_count_key_row(DataTable *dt, gint grpby, Filter *flt)
{
guint n_row, n_key;

	n_row = n_key = 0;
	if( grpby != REPORT_GRPBY_MONTH && grpby != REPORT_GRPBY_YEAR )
	{
		switch(grpby)
		{
			case REPORT_GRPBY_NONE:
				n_row = 1;
				n_key = 1;
				break;
			case REPORT_GRPBY_TYPE:
				n_row = 2;
				n_key = 2;
				break;
			case REPORT_GRPBY_CATEGORY:
				n_row = da_cat_length();
				n_key = da_cat_get_max_key();
				break;
			case REPORT_GRPBY_PAYEE:
				n_row = da_pay_length();
				n_key = da_pay_get_max_key();
				break;
			//5.7: todo check this +1
			case REPORT_GRPBY_ACCOUNT:
				n_row = da_acc_length();
				n_key = da_acc_get_max_key();
				break;
			case REPORT_GRPBY_ACCGROUP:
				n_row = da_grp_length();
				n_key = da_grp_get_max_key();
				break;
			case REPORT_GRPBY_TAG:
				n_row = da_tag_length();
				n_key = da_tag_get_max_key();
				break;
		}
	}
	else
	{
	GDate *date1 = g_date_new_julian(flt->mindate);
	GDate *date2 = g_date_new_julian(flt->maxdate);
	
		switch(grpby)
		{
			case REPORT_GRPBY_MONTH:
				n_row = ((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1) + 1;
				n_key = n_row;
				break;
			case REPORT_GRPBY_YEAR:
				n_row = g_date_get_year(date2) - g_date_get_year(date1) + 1;
				n_key = n_row;
			break;
		}

		g_date_free(date2);
		g_date_free(date1);
	}

	//we add 1 to every key as 0 is an item
	dt->nbkeys = n_key + 1;
	dt->nbrows = n_row;
}


static void _datatable_init_items_cat(DataTable *dt)
{
GList *lcat, *l;
guint idx = 0;

	lcat = l = category_glist_sorted(HB_GLIST_SORT_NAME);
	while (l != NULL )
	{
	Category *cat = l->data;
	DataRow *dr = dt->rows[idx];

		if( dr != NULL )
		{
			dr->pos   = idx;
			dr->label = g_strdup( (cat->key == 0) ? _("(no category)") : cat->name );
			//dt->keyindex[cat->key] = idx;
			datatable_set_keyindex(dt, cat->key, idx);
			
			DB2( g_print(" +cat k:%d, idx:%d %s'%s'\n", cat->key, idx, cat->parent==0 ? "" : " +", dr->label) );
		}
		l = g_list_next(l);
		idx++;
	}
	g_list_free(lcat);

	//init keylist
	DB2( g_print(" add cat keys") );
	idx = 0;
	lcat = l = category_glist_sorted(HB_GLIST_SORT_KEY);
	while (l != NULL)
	{
	Category *cat = l->data;
	
		//dt->keylist[idx] = cat->key;
		datatable_set_keylist(dt, idx, cat->key);
		DB2( g_print(" %d", cat->key) );
		l = g_list_next(l);
		idx++;
	}
	g_list_free(lcat);
	DB2( g_print("\n") );	
}

static void _datatable_init_items_pay(DataTable *dt)
{
GList *lpay, *l;
guint idx = 0;

	lpay = l = payee_glist_sorted(HB_GLIST_SORT_NAME);
	while (l != NULL )
	{
	Payee *pay = l->data;
	DataRow *dr = dt->rows[idx];

		if( dr != NULL )
		{
			dr->pos   = idx;
			dr->label = g_strdup( (pay->key == 0) ? _("(no payee)") : pay->name );
			DB2( g_print(" +pay k:%d, idx:%d '%s'\n", pay->key, idx, dr->label) );
			//store transpose
			//dt->keyindex[pay->key] = idx;
			datatable_set_keyindex(dt, pay->key, idx);
			//store keylist
			//dt->keylist[idx] = pay->key;
			datatable_set_keylist(dt, idx, pay->key);
		}
		l = g_list_next(l);
		idx++;
	}
	g_list_free(lpay);
}


static void _datatable_init_items_acc(DataTable *dt)
{
GList *lacc, *l;
guint idx = 0;

	lacc = l = account_glist_sorted(HB_GLIST_SORT_NAME);
	while (l != NULL )
	{
	Account *acc = l->data;
	DataRow *dr = dt->rows[idx];

		if( dr != NULL )
		{

			dr->pos   = idx;
			dr->label = g_strdup( acc->name );
			DB2( g_print(" +acc k:%d, idx:%d '%s'\n", acc->key, idx, dr->label) );
			//store transpose
			dt->keyindex[acc->key] = idx;
			//datatable_set_keyindex(dt, acc->key, idx);
			//store keylist
			dt->keylist[idx] = acc->key;
			//datatable_set_keylist(dt, idx, acc->key);
		}
		l = g_list_next(l);
		idx++;
	}
	g_list_free(lacc);
}


static void _datatable_init_items_accgrp(DataTable *dt)
{
GList *lgrp, *l;
guint idx = 0;

	lgrp = l = group_glist_sorted(HB_GLIST_SORT_NAME);
	while (l != NULL )
	{
	Group *grp = l->data;
	DataRow *dr = dt->rows[idx];

		if( dr != NULL )
		{
			dr->pos   = idx;
			dr->label = g_strdup( (grp->key == 0) ? _("(no group)") : grp->name );
			DB2( g_print(" +grp k:%d, idx:%d '%s'\n", grp->key, idx, dr->label) );
			//store transpose
			dt->keyindex[grp->key] = idx;
			//datatable_set_keyindex(dt, acc->key, idx);
			//store keylist
			dt->keylist[idx] = grp->key;
			//datatable_set_keylist(dt, idx, acc->key);
		}
		l = g_list_next(l);
		idx++;
	}
	g_list_free(lgrp);
}



static void _datatable_init_items_tag(DataTable *dt)
{
GList *ltag, *l;
guint idx = 0;

	ltag = l = tag_glist_sorted(HB_GLIST_SORT_NAME);
	while (l != NULL )
	{
	Tag *tag = l->data;
	DataRow *dr = dt->rows[idx];

		if( dr != NULL )
		{
			dr->pos   = idx;
			dr->label = g_strdup( (tag->key == 0) ? _("(no tag)") : tag->name );
			DB2( g_print(" +tag k:%d, idx:%d '%s'\n", tag->key, idx, dr->label) );
			//store transpose
			//dt->keyindex[tag->key] = idx;
			datatable_set_keyindex(dt, tag->key, idx);
			//store keylist
			//dt->keylist[idx] = tag->key;
			datatable_set_keylist(dt, idx, tag->key);
		}
		l = g_list_next(l);
		idx++;
	}
	g_list_free(ltag);
}


static void datatable_data_get_xlabel(GDate *date, guint intvl, gchar *intvlname, gsize slen)
{
	intvlname[0] ='\0';
	switch(intvl)
	{
		case REPORT_INTVL_DAY:
		case REPORT_INTVL_FORTNIGHT:
			g_date_strftime (intvlname, slen, "%b-%d", date);
			break;
		case REPORT_INTVL_WEEK:
			//TRANSLATORS: printf string for year of week W, ex. 2019-W52 for week 52 of 2019
			g_snprintf(intvlname, slen, _("w%02d"), g_date_get_iso8601_week_of_year(date));
			break;
		case REPORT_INTVL_MONTH:
			g_snprintf(intvlname, slen, "%s", _(CYA_ABMONTHS[g_date_get_month(date)]));
			break;
		case REPORT_INTVL_QUARTER:
			//todo: will be innacurrate here if fiscal year start not 1/jan
			//TRANSLATORS: printf string for year of quarter Q, ex. 2019-Q4 for quarter 4 of 2019
			g_snprintf(intvlname, slen, _("q%d"), ((g_date_get_month(date)-1)/3)+1);
			break;
		case REPORT_INTVL_HALFYEAR:
			//#2007712
			//TRANSLATORS: printf string for half-year H, ex. 2019-H1 for 1st half-year of 2019
			g_snprintf(intvlname, slen, _("h%d"), g_date_get_month(date) < 7 ? 1 : 2);
			break;
		case REPORT_INTVL_YEAR:
			g_snprintf(intvlname, slen, "%d", g_date_get_year(date));
			break;
	}
}


static void datatable_init_items(DataTable *dt, gint grpby, guint32 jfrom)
{
GDate *date;
gchar buffer[64], *name;
guint32 jdate;
guint idx, slen = 63;
GDateYear prevyear = 0;

	DB1( g_print("\n-- datatable int rows --\n") );

	if( grpby == REPORT_GRPBY_CATEGORY 
 	 || grpby == REPORT_GRPBY_PAYEE
	 || grpby == REPORT_GRPBY_ACCOUNT
	 || grpby == REPORT_GRPBY_ACCGROUP
	 || grpby == REPORT_GRPBY_TAG
	 )
	{
		switch(grpby)
		{
			case REPORT_GRPBY_CATEGORY:
				_datatable_init_items_cat(dt);
				break;
			case REPORT_GRPBY_PAYEE:
				_datatable_init_items_pay(dt);
				break;
			case REPORT_GRPBY_ACCOUNT:
				_datatable_init_items_acc(dt);
				break;
			case REPORT_GRPBY_ACCGROUP:
				_datatable_init_items_accgrp(dt);
				break;
			case REPORT_GRPBY_TAG:
				_datatable_init_items_tag(dt);
				break;
		}
		return;
	}


	date = g_date_new();
	jdate = GLOBALS->today;
	for(idx=0;idx<dt->nbrows;idx++)
	{
	DataRow *dr = dt->rows[idx];
	guint intvl = REPORT_INTVL_NONE;
		
		name = NULL;
		switch(grpby)
		{
			case REPORT_GRPBY_NONE:
				//technical label for sum
				name = "none";
				DB2( g_print(" +none k:%d, idx:%d '%s'\n", idx, idx, name) );
				break;
			case REPORT_GRPBY_TYPE:
				name = idx == 0 ? "exp" : "inc";
				DB2( g_print(" +type k:%d, idx:%d '%s'\n", idx, idx, name) );
				break;		
			case REPORT_GRPBY_MONTH:
				intvl = REPORT_INTVL_MONTH;
				jdate = report_interval_snprint_name(buffer, sizeof(buffer)-1, intvl, jfrom, idx);
				name = buffer;
				DB2( g_print(" +month k:%d, idx:%d '%s'\n", idx, idx, name) );
				break;
			case REPORT_GRPBY_YEAR:
				intvl = REPORT_INTVL_YEAR;
				jdate = report_interval_snprint_name(buffer, sizeof(buffer)-1, intvl, jfrom, idx);
				name = buffer;
				DB2( g_print(" +year k:%d, idx:%d '%s'\n", idx, idx, name) );
				break;
		}

		dr->pos = idx;
		if( name != NULL )
			dr->label = g_strdup(name);

		//store transpose
		//dt->keyindex[idx] = idx;
		datatable_set_keyindex(dt, idx, idx);
		//store keylist
		//dt->keylist[idx] = idx;
		datatable_set_keylist(dt, idx, idx);
		
		//manage xlabel
		g_date_set_julian(date, jdate);

		datatable_data_get_xlabel(date, intvl, buffer, slen);

		dr->xlabel = g_strdup(buffer);
		
		//set flags => moved into forecast
		//if( jdate > GLOBALS->today )
		//	dc->flags |= RF_FORECAST;
	
		if( (intvl != REPORT_INTVL_YEAR) && (g_date_get_year(date) > prevyear) && (g_date_get_month(date) == 1) )
		{
			g_snprintf(buffer, slen, "%d", g_date_get_year(date));
			dr->misclabel = g_strdup(buffer);
			dr->flags |= RF_NEWYEAR;
		}

		prevyear = g_date_get_year(date);		
		
	}
	
	g_date_free(date);

/*end:
	//debug keyindex
	g_print(" keyindex:");
	for(idx=0;idx<dt->nbkeys;idx++)
		g_print(" %d", dt->keyindex[idx]);
	g_print("\n");
	
	//debug keylist
	g_print(" keylist:");
	for(idx=0;idx<dt->nbrows;idx++)
		g_print(" %d", dt->keylist[idx]);
	g_print("\n");
*/
}


static void datatable_init_columns(DataTable *dt, gint intvl, guint32 jfrom)
{
GDate *date;
gchar intvlname[64];
guint32 jdate;
guint idx, slen = 63;
GDateYear prevyear = 0;

	date = g_date_new();

	for(idx=0;idx<dt->nbcols;idx++)
	{
	DataCol *dc = dt->cols[idx];

		//TODO: check
		report_interval_snprint_name(intvlname, sizeof(intvlname)-1, intvl, jfrom, idx);
		dc->label = g_strdup(intvlname);

		//new stuff
		jdate = _datatable_interval_get_jdate(intvl, jfrom, idx);
		g_date_set_julian(date, jdate);

		datatable_data_get_xlabel(date, intvl, intvlname, slen);

		dc->xlabel = g_strdup(intvlname);

		//set flags => moved into forecast
		if( jdate > GLOBALS->today )
			dc->flags |= RF_FORECAST;
	
		if( (intvl != REPORT_INTVL_YEAR) && (g_date_get_year(date) > prevyear) && (g_date_get_month(date) == 1) )
		{
			g_snprintf(intvlname, slen, "%d", g_date_get_year(date));
			dc->misclabel = g_strdup(intvlname);
			dc->flags |= RF_NEWYEAR;
		}

		prevyear = g_date_get_year(date);
	}

	g_date_free(date);

}

static gint datatable_data_get_txnsign(DataTable *dt, guint32 kcat, gdouble amount)
{
gint txnsign = 0;

	//raw expense/income mode
	if( (dt->flags & REPORT_COMP_FLG_CATSIGN) )
	{
		txnsign = category_root_type_get(kcat);
		//todo: fallback maybe ?
		//if( txnsign == 0 )
	}
	else
	{
		txnsign = (amount < 0.0) ? -1 : 1;
	}
	return txnsign;
}


static void datatable_add(DataTable *dt, guint32 key, guint32 col, gdouble amount, gint sign, gboolean addtotal)
{
DataRow *dr;

	if( hb_amount_cmp(amount, 0.0) == 0 )
		return;

	dr = report_data_get_row(dt, key); 
	if( dr == NULL )
		return;

	if( col < dt->nbcols )
	{
		//total mode must filter opposite sign categories
		if( dt->intvl == REPORT_INTVL_NONE )
		{		
			if( (dt->flags & REPORT_COMP_FLG_SPENDING) && sign == 1 )
				return;
//future
//			if( (dt->flags & REPORT_COMP_FLG_REVENUE) && sign == -1 )
//				return;
		}

		DB3( g_print(">>> add to k:%d c:%d %.2f sign=%d\n", key, col, amount, sign) );

		if( sign == -1 )
		{
			dr->colexp[col] += amount;
			dr->rowexp += amount;
			if( addtotal == TRUE )
			{
				dt->totrow->colexp[col] += amount;
				dt->totrow->rowexp += amount;
			}
		}
		else
		if( sign == 1 )
		{
			dr->colinc[col] += amount;
			dr->rowinc += amount;
			if( addtotal == TRUE )
			{
				dt->totrow->colinc[col] += amount;
				dt->totrow->rowinc += amount;
			}
		}
	}
	else
		g_warning("datatable add invalid col %d of %d", col, dt->nbcols);
}


static void datatable_data_cols_cumulate(DataTable *dt, gint intvl, Filter *flt)
{


	DB2( g_print("\n-- cumulate columns --\n") );
	DB2( g_print(" n_row=%d n_col=%d\n", dt->nbrows, dt->nbcols) );
	//cumulate columns
	if( dt->nbcols > 1 )
	{
	guint row, col;

		for(row=0;row<dt->nbrows;row++)
		{
		DataRow *dr = dt->rows[row];
			
			DB2( g_print(" row %d: ", row) );
			for(col=1;col<dr->nbcols;col++)
			{
			guint32 jdate = _datatable_interval_get_jdate(intvl, flt->mindate, col);
			
				if( jdate < dt->maxpostdate)
				{
					DB2( g_print("%d, ", col) );
					dr->colinc[col] += dr->colinc[col-1];
					dr->colexp[col] += dr->colexp[col-1];		

					dt->totrow->colinc[col] += dr->colinc[col-1];
					dt->totrow->colexp[col] += dr->colexp[col-1];
				}
			}
			DB2( g_print("\n") );
		}
	}
}


static void datatable_data_add_balance(DataTable *dt, gint grpby, gint intvl, Filter *flt)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;

	DB1( g_print(" -- try add balance\n") );

	if( (grpby != REPORT_GRPBY_NONE)
	 && (grpby != REPORT_GRPBY_ACCOUNT)
	 && (grpby != REPORT_GRPBY_ACCGROUP)
	)
		return;

	DB1( g_print(" -- add balance\n") );

	//TODO: we should rely on rows...
	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;
	guint32 pos = 0, txnsign;
	gdouble amount;

		//#1674045 ony rely on nosummary
		if( (acc->flags & (AF_NOREPORT)) )
			goto next_acc;
			
		//#2000294 enable account filtering
		if( filter_acc_match(flt, acc) == 0 )
			goto next_acc;

		//add initial amount to col[0]
		amount = hb_amount_base(acc->initial, acc->kcur);
		//we can't use report_items_get_key here
		//for none, we only allocate the row[0], so pos is 0
		pos = 0;
		switch(grpby)
		{
			case REPORT_GRPBY_ACCOUNT:
				pos = acc->key;
				break;
			case REPORT_GRPBY_ACCGROUP:
				pos = acc->kgrp;
				break;
		}		
		
		txnsign = (amount < 0.0) ? -1 : 1;
		datatable_add(dt, pos, 0, amount, txnsign, TRUE);
		DB2( g_print("  ** add initbal %8.2f to row:%d, col:%d\n", amount, acc->key, 0) );

		//add txn amount prior mindate to col[0]
		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;
		
			//5.5 forgot to filter...
			//#1886123 include remind based on user prefs
			if( !transaction_is_balanceable(txn) )
				goto next_txn;
				
			//enable filters : make no sense or not
			//if( (filter_txn_match(flt, txn) == 1) )
			//{
				pos = report_items_get_key(grpby, flt->mindate, txn);

				amount = report_txn_amount_get(flt, txn);
				amount = hb_amount_base(amount, txn->kcur);

				if( txn->date < flt->mindate )
				{
					DB2( g_print("  **add %8.2f row:%d, col:%d\n", amount, pos, 0) );
					txnsign = datatable_data_get_txnsign(dt, txn->kcat, amount);
					datatable_add(dt, pos, 0, amount, txnsign, TRUE);
				}
			//}
		next_txn:
			lnk_txn = g_list_next(lnk_txn);
		}

	next_acc:
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

}


static void datatable_data_add_txn(DataTable *dt, gint grpby, gint intvl, Transaction *txn, Filter *flt)
{
guint i;
guint key = 0;
guint col = 0;
gdouble amount;
gint txnsign;

	
	amount = report_txn_amount_get(flt, txn);
	amount = hb_amount_base(amount, txn->kcur);
	txnsign = datatable_data_get_txnsign(dt, txn->kcat, amount);

	DB2( g_print("\n srctxn %.2f, sign=%d, split=%d, cat=%d, pay=%d, acc=%d\n", amount, txnsign, 
		(txn->flags & OF_SPLIT), txn->kcat, txn->kpay, txn->kacc) );

	col = report_interval_get_pos(intvl, flt->mindate, txn);
	DB2( g_print("  col=%d (max is %d)\n", col, dt->nbcols) );

	switch( grpby )
	{
		case REPORT_GRPBY_CATEGORY:
		//case REPORT_GRPBY_SUBCATEGORY:
			//for split, affect the amount to the category
			if( txn->flags & OF_SPLIT )
			{
			guint nbsplit = da_splits_length(txn->splits);
			Split *split;
			gboolean status;
			gint sinsert;

				DB2( g_print("  %d splits\n", nbsplit) );
				for(i=0;i<nbsplit;i++)
				{
					split = da_splits_get(txn->splits, i);
					status = da_flt_status_cat_get(flt, split->kcat);
					sinsert = ( status == TRUE ) ? 1 : 0;
					if(flt->option[FLT_GRP_CATEGORY] == 2) sinsert ^= 1;
					if( (flt->option[FLT_GRP_CATEGORY] == 0) || sinsert)
					{
					Category *cat = da_cat_get(split->kcat);

						amount = hb_amount_base(split->amount, txn->kcur);
						if( cat != NULL )
						{
							txnsign = datatable_data_get_txnsign(dt, split->kcat, amount);
							DB2( g_print(" cat split insert=%d, kcat=%d %.2f %d\n", sinsert, split->kcat, amount, txnsign) );
							datatable_add(dt, cat->key, col, amount, txnsign, TRUE);
							if( cat->parent != 0 )
								datatable_add(dt, cat->parent, col, amount, txnsign, FALSE);
						}	
					}
					else
					{
						DB2( g_print("  >skipped, cat get=%d\n", sinsert) );
					}
				}
			}
			else
			{
			Category *cat = da_cat_get(txn->kcat);

				if( cat != NULL )
				{
					DB2( g_print(" cat normal kcat=%d %2.f\n", txn->kcat, amount) );
					datatable_add(dt, cat->key, col, amount, txnsign, TRUE);
					if( cat->parent != 0 )
						datatable_add(dt, cat->parent, col, amount, txnsign, FALSE);
				}
			}
			break;

		case REPORT_GRPBY_TAG:
			if(txn->tags != NULL)
			{
			guint32 *tptr = txn->tags;

				while(*tptr)
				{
					//#2031693 not -1 here
					key = *tptr;
					datatable_add(dt, key, col, amount, txnsign, TRUE);
					tptr++;
				}
			}
			//2031693 + add notags
			else
				datatable_add(dt, 0, col, amount, txnsign, TRUE);
			break;

		//5.8 define exp/inc by category type
		case REPORT_GRPBY_TYPE:
			if( txn->flags & OF_SPLIT )
			{
			guint nbsplit = da_splits_length(txn->splits);
			Split *split;

				DB2( g_print("  %d splits\n", nbsplit) );
				for(i=0;i<nbsplit;i++)
				{
					split = da_splits_get(txn->splits, i);
					//we don't care cat filter here
					amount = hb_amount_base(split->amount, txn->kcur);
					txnsign = datatable_data_get_txnsign(dt, split->kcat, amount);
					DB2( g_print(" by type cat split add kcat=%d %.2f %d\n", split->kcat, amount, txnsign) );
					if( (dt->flags & REPORT_COMP_FLG_SPENDING) && txnsign == -1 )
					{
						DB2( g_print(" add exp\n") );
						datatable_add(dt, 0, col, amount, txnsign, FALSE);	//expense
					}
					if( (dt->flags & REPORT_COMP_FLG_REVENUE) && txnsign == 1 )
					{
						DB2( g_print(" add inc\n") );
						datatable_add(dt, 1, col, amount, txnsign, FALSE);	//income
					}
				}
			}
			else
			{
				DB2( g_print(" by type: sign=%d\n", txnsign) );
				if( (dt->flags & REPORT_COMP_FLG_SPENDING) && txnsign == -1 )
				{
					DB2( g_print(" add exp\n") );
					datatable_add(dt, 0, col, amount, txnsign, FALSE);	//expense
				}
				if( (dt->flags & REPORT_COMP_FLG_REVENUE) && txnsign == 1 )
				{
					DB2( g_print(" add inc\n") );
					datatable_add(dt, 1, col, amount, txnsign, FALSE);	//income
				}
			}
			break;

		//all others
		default:
			key = report_items_get_key(grpby, flt->mindate, txn);
			datatable_add(dt, key, col, amount, txnsign, TRUE);
			break;
	}
}


//5.7 forecast attempt
static void report_compute_forecast(DataTable *dt, gint grpby, gint intvl, Filter *flt)
{
GList *list;
gint nbinsert;
guint32 curdate, maxpostdate;
GDate *post_date;

	DB1( g_print("\n[report] compute_forecast\n") );

	if( intvl == REPORT_INTVL_NONE )
	{
		DB1( g_print(" no forecast for total mode\n") );
		return;
	}

	post_date = g_date_new();

	//TODO: change this
	maxpostdate = dt->maxpostdate;
	

	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *arc = list->data;

		DB2( g_print("--------\n eval post of '%s' %.2f\n", arc->memo, arc->amount) );

		if(scheduled_is_postable(arc) == TRUE)
		{
		Account *acc;
		Transaction *txn, *child;

			DB2( g_print(" is postable\n") );

			nbinsert = 0;
			child = NULL;
			txn = da_transaction_malloc();
			da_transaction_init_from_template(txn, arc);
			
			acc = da_acc_get(txn->kacc);
			if(acc)
			{
				//#5.7.3 don't forecast...
				if( acc->flags & (AF_NOREPORT|AF_CLOSED) )
					goto nextarc;
				txn->kcur = acc->kcur;
			}

			txn->date = curdate = arc->nextdate;

			//#2048236 also add child xfer txn to computing
			if( txn->flags & OF_INTXFER )
			{
				child = transaction_xfer_child_new_from_txn(txn);
			} 

			//if arc->nexdate is prior flt->mindate, it will be filtered out
			if( (filter_txn_match(flt, txn) == 1) )
			{
				while(curdate <= maxpostdate)
				{
					txn->date = curdate; 
					DB3( hb_print_date(curdate, ">>> curdate=") );

					datatable_data_add_txn(dt, grpby, intvl, txn, flt);

					//#2048236 also add child xfer txn to computing
					if( child != NULL )
					{
						child->date = curdate;
						datatable_data_add_txn(dt, grpby, intvl, child, flt);
					}

					//mark column as forecast
					guint idx = report_interval_get_pos(intvl, flt->mindate, txn);
					DataCol *dc = report_data_get_col(dt, idx);
					if( dc != NULL )
						dc->flags |= RF_FORECAST;

					curdate = scheduled_date_get_next_post(post_date, arc, curdate);
					nbinsert++;
					// break if over limit
					if( (arc->rec_flags & TF_LIMIT) && (nbinsert >= arc->limit) )
						break;
				}
			}

			da_transaction_free (txn);
			da_transaction_free (child);
			
		}
nextarc:
		list = g_list_next(list);
	}

	g_date_free(post_date);

}

/* = = = = = = = = = = = = = = = = = = = = */

DataTable *report_compute(gint grpby, gint intvl, Filter *flt, GQueue *txn_queue, gint flags)
{
DataTable *dt;
GList *list;
guint32 maxpostdate = flt->maxdate;

	DB1( g_print("\n[report] == report_compute ==\n") );

	DB2( 
		{
		gchar *txt = "??";
			switch(grpby){
				case REPORT_GRPBY_NONE: txt="none"; break;
				case REPORT_GRPBY_CATEGORY: txt="cat"; break;
				case REPORT_GRPBY_PAYEE: txt="pay"; break;
				case REPORT_GRPBY_ACCOUNT: txt="acc"; break;
				case REPORT_GRPBY_TAG: txt="tag"; break;
				case REPORT_GRPBY_MONTH: txt="month"; break;
				case REPORT_GRPBY_YEAR: txt="year"; break;
				case REPORT_GRPBY_ACCGROUP: txt="accgrp"; break;
				case REPORT_GRPBY_TYPE: txt="type"; break;
			}
			g_print(" grpby: %d %s\n", grpby, txt);
		}
	);


	DB2( hb_print_date(maxpostdate, "maxdate") );

	//set maxpostdate
	/*if( do_forecast )
	{
		if( filter_preset_daterange_future_enable(flt, flt->range) )
		{
		GDate *post_date = g_date_new();

			g_date_set_time_t(post_date, time(NULL));
			g_date_add_months(post_date, PREFS->rep_forecat_nbmonth);
			g_date_set_day(post_date, g_date_get_days_in_month(g_date_get_month(post_date), g_date_get_year(post_date)));
			maxpostdate = g_date_get_julian(post_date);
			DB2( hb_print_date(maxpostdate, "max forecast date") );

			g_date_free(post_date);

			flt->maxdate = maxpostdate;
		}
	}*/

	dt = da_datatable_malloc(grpby, intvl, flt);

	if( dt == NULL )
		return dt;

	dt->flags       = flags;
	dt->grpby       = grpby;
	dt->intvl		= intvl;
	dt->maxpostdate = maxpostdate;


	datatable_init_items(dt, grpby, flt->mindate);
	datatable_init_columns(dt, intvl, flt->mindate);

	//balance must keep xfer
	/*if( do_balance == TRUE )
	{
		
	
	}*/

	//process txn
	DB2( g_print("\n-- ventilate txn amount to row/col --\n") );

	DB2( g_print(" option: signbycat=%d\n", flags & REPORT_COMP_FLG_CATSIGN) );
	DB2( g_print(" option: spending =%d\n", flags & REPORT_COMP_FLG_SPENDING) );
	DB2( g_print(" option: revenue  =%d\n", flags & REPORT_COMP_FLG_REVENUE) );

	list = g_queue_peek_head_link(txn_queue);
	while (list != NULL)
	{
	Transaction *txn = list->data;
		
		if( (filter_txn_match(flt, txn) == 1) )
		{
			datatable_data_add_txn(dt, grpby, intvl, txn, flt);
		}
		list = g_list_next(list);
	}

	//TODO: add prefs
	//if( PREFS->xxx )
	if( flags & REPORT_COMP_FLG_FORECAST )
	{
		DB2( g_print("\n-- compute forecast --\n") );
		report_compute_forecast(dt, grpby, intvl, flt);
	}

	//truncate in sign based category
	if( flags & REPORT_COMP_FLG_CATSIGN )
	{
		DB2( g_print("\n-- typebycat post compute --\n") );
		if( grpby == REPORT_GRPBY_TYPE )
		{
		guint col;
		DataRow *dr1 = dt->rows[0]; //exp
		DataRow *dr2 = dt->rows[1]; //inc

			for(col=0;col<dr1->nbcols;col++)
			{
				if( flags & REPORT_COMP_FLG_SPENDING && dr1->colexp[col] > 0.0 )
					dr1->colexp[col] = 0.0;
				if( flags & REPORT_COMP_FLG_REVENUE && dr2->colinc[col] < 0.0 )
					dr2->colinc[col] = 0.0;
			}
			DB2( g_print(" processed %d\n", col) );
		}
	}

	//process balance mode
	if( flags & REPORT_COMP_FLG_BALANCE )
	{
		DB2( g_print("\n-- compute balance --\n") );
		datatable_data_add_balance(dt, grpby, intvl, flt);
		datatable_data_cols_cumulate(dt, intvl, flt);
	}

	return dt;
}


DataCol *report_data_get_col(DataTable *dt, guint32 idx)
{
DataCol *retval = NULL;

	if( idx < dt->nbcols )
	{
		retval = dt->cols[idx];
	}
	else
		g_warning("datatable invalid get col %d of %d", idx, dt->nbcols);

	return retval;
}


//get a specific row by key
//hub-reptime/hub-reptotal/repstats
DataRow *report_data_get_row(DataTable *dt, guint32 key)
{
DataRow *retval = NULL;
guint32 idx;

	if( key < dt->nbkeys )
	{
		//we should transpose here
		idx = dt->keyindex[key];
		if( idx < dt->nbrows )
		{
			DB3( g_print(">>> get row=%d for key=%d\n", idx, key) ); 
			retval = dt->rows[idx];
		}
		else
			g_warning("datatable invalid get row %d of %d", idx, dt->nbrows);
	}
	else
		g_warning("datatable invalid get row key %d of %d", key, dt->nbkeys);

	return retval;
}


//gtk-chart/list-report
gdouble da_datarow_get_cell_sum(DataRow *dr, guint32 index)
{
	if( index < dr->nbcols )
	{
		return (dr->colexp[index] + dr->colinc[index]);
	}

	g_warning("invalid datarow column");
	return 0;
}


//rep-stat
guint report_items_get_key(gint grpby, guint jfrom, Transaction *txn)
{
Account *acc;
guint key = 0;

	switch(grpby)
	{
		case REPORT_GRPBY_NONE:
			key = 0;
			break;
		case REPORT_GRPBY_CATEGORY:
			key = category_report_id(txn->kcat, FALSE);
			break;
		//case REPORT_GRPBY_SUBCATEGORY:
		//	pos = txn->kcat;
		//	break;
		case REPORT_GRPBY_PAYEE:
			key = txn->kpay;
			break;
		case REPORT_GRPBY_ACCOUNT:
			key = txn->kacc;
			break;
		case REPORT_GRPBY_ACCGROUP:
			acc = da_acc_get(txn->kacc);
			if( acc != NULL )
				key = acc->kgrp;
			break;

		//TODO! miss TAG

		case REPORT_GRPBY_MONTH:
			key = DateInMonth(jfrom, txn->date);
			break;
		case REPORT_GRPBY_YEAR:
			key = DateInYear(jfrom, txn->date);
			break;
	}
	
	return key;
}


//rep- balance/budget/time
gint report_interval_get_pos(gint intvl, guint jfrom, Transaction *txn)
{
gint pos = 0;

	switch(intvl)
	{
		case REPORT_INTVL_DAY:
			pos = txn->date - jfrom;
			break;
		case REPORT_INTVL_WEEK:
			//#1915643 week iso 8601
			//pos = (txn->date - jfrom)/7;
			pos = DateInWeek(jfrom, txn->date);
			break;
		//#2000290
		case REPORT_INTVL_FORTNIGHT:
			pos = DateInFortNight(jfrom, txn->date);
			break;
		case REPORT_INTVL_MONTH:
			pos = DateInMonth(jfrom, txn->date);
			break;
		case REPORT_INTVL_QUARTER:
			pos = DateInQuarter(jfrom, txn->date);
			break;
		case REPORT_INTVL_HALFYEAR:
			pos = DateInHalfYear(jfrom, txn->date);
			break;
		case REPORT_INTVL_YEAR:
			pos = DateInYear(jfrom, txn->date);
			break;
		default: //REPORT_INTVL_NONE
			pos = 0;
			break;
	}
	
	return pos;
}


//rep-stats
//rep- balance/time
gint report_interval_count(gint intvl, guint32 jfrom, guint32 jto)
{
GDate *date1, *date2;
gint nbintvl = 0;

	date1 = g_date_new_julian(jfrom);
	date2 = g_date_new_julian(jto);
	
	switch(intvl)
	{
		case REPORT_INTVL_DAY:
			nbintvl = (jto - jfrom);
			break;
		case REPORT_INTVL_WEEK:
			//#2000292 weeknum iso 8601 as well
			_hb_date_clamp_iso8601(date1);
			nbintvl = (g_date_days_between(date1, date2) / 7);
			break;
		//#2000290
		case REPORT_INTVL_FORTNIGHT:
			_hb_date_clamp_iso8601(date1);
			nbintvl = (g_date_days_between(date1, date2) / 14);		
			break;
		case REPORT_INTVL_MONTH:
			nbintvl = ((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1);
			break;
		case REPORT_INTVL_QUARTER:
			//#1758532 slide quarter start
			_hb_date_clamp_quarter_start(date1);
			nbintvl = (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/3;
			break;
		case REPORT_INTVL_HALFYEAR:
			//#2034618 slide halfyear start
			_hb_date_clamp_halfyear_start(date1);
			nbintvl = (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/6;
			break;
		case REPORT_INTVL_YEAR:
			nbintvl = g_date_get_year(date2) - g_date_get_year(date1);
			break;
	}

	g_date_free(date2);
	g_date_free(date1);

	//5.9 check
	if( nbintvl < 0 )
	{
		g_warning("report: intvl<0");

		#if MYDEBUG
		hb_print_date(jfrom, NULL);
		hb_print_date(jto, NULL);
		#endif

		nbintvl = 0;
	}

	return 1 + nbintvl;
}


//used in list-report / rep- balance/budget/time 
guint32 report_interval_snprint_name(gchar *s, gint slen, gint intvl, guint32 jfrom, gint idx)
{
GDate *date;
guint32 jdate = _datatable_interval_get_jdate(intvl, jfrom, idx);

	date = g_date_new_julian(jdate);
	switch(intvl)
	{
		case REPORT_INTVL_DAY:
			g_date_strftime (s, slen, PREFS->date_format, date);
			break;
		case REPORT_INTVL_WEEK:
			//TRANSLATORS: printf string for year of week W, ex. 2019-W52 for week 52 of 2019
			g_snprintf(s, slen, _("%d-w%02d"), g_date_get_year(date), g_date_get_iso8601_week_of_year(date));
			break;
		//#2000290
		case REPORT_INTVL_FORTNIGHT:
			g_date_strftime (s, slen, PREFS->date_format, date);
			break;
		case REPORT_INTVL_MONTH:
			g_snprintf(s, slen, "%d-%s", g_date_get_year(date), _(CYA_ABMONTHS[g_date_get_month(date)]));
			break;
		case REPORT_INTVL_QUARTER:
			//todo: will be innacurrate here if fiscal year start not 1/jan
			//TRANSLATORS: printf string for year of quarter Q, ex. 2019-Q4 for quarter 4 of 2019
			g_snprintf(s, slen, _("%d-q%d"), g_date_get_year(date), ((g_date_get_month(date)-1)/3)+1);
			break;
		case REPORT_INTVL_HALFYEAR:
			//#2007712
			//TRANSLATORS: printf string for half-year H, ex. 2019-H1 for 1st half-year of 2019
			g_snprintf(s, slen, _("%d-h%d"), g_date_get_year(date), g_date_get_month(date) < 7 ? 1 : 2);
			break;
		case REPORT_INTVL_YEAR:
			g_snprintf(s, slen, "%d", g_date_get_year(date));
			break;
		default:
			*s ='\0';
			break;
	}

	g_date_free(date);

	return jdate;
}


//TODO: maybe migrate this to filter as well
//#1562372 in case of a split we need to take amount for filter categories only
//used in rep-time
gdouble report_txn_amount_get(Filter *flt, Transaction *txn)
{
gdouble amount;

	amount = txn->amount;

	if( flt->option[FLT_GRP_CATEGORY] > 0 )	//not inactive
	{
		if( txn->flags & OF_SPLIT )
		{
		guint i, nbsplit = da_splits_length(txn->splits);
		Split *split;
		gboolean status;
		gint sinsert;

			amount = 0.0;

			for(i=0;i<nbsplit;i++)
			{
				split = da_splits_get(txn->splits, i);
				status = da_flt_status_cat_get(flt, split->kcat);
				sinsert = ( status == TRUE ) ? 1 : 0;
				if(flt->option[FLT_GRP_CATEGORY] == 2) sinsert ^= 1;

				DB2( g_print(" split insert=%d, kcat=%d\n", sinsert, split->kcat) );

				if( (flt->option[FLT_GRP_CATEGORY] == 0) || sinsert)
				{
					amount += split->amount;
				}
			}

		}
	}
	return amount;
}

