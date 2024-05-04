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
#include "hb-report.h"

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
extern struct Preferences *PREFS;


extern gchar *CYA_ABMONTHS[];

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

static void da_datarow_free(DataRow *row)
{
	if(row)
	{
		g_free(row->label);
		g_free(row->expense);
		g_free(row->income);
	}
}


static DataRow *da_datarow_malloc(guint32 nbcol)
{
DataRow *row;

	row = g_malloc0(sizeof(DataRow));
	if(!row)
		return NULL;

	row->nbcols = nbcol+2;
	row->expense = g_malloc0((nbcol+2) * sizeof(gdouble));
	row->income  = g_malloc0((nbcol+2) * sizeof(gdouble));
	return row;
}


void da_datatable_free(DataTable *dt)
{
guint i;

	DB( g_print("da_datatable_free\n") );
	
	if(dt != NULL)
	{
		//free each rows
		for(i=0;i<dt->nbrows;i++)
		{
		DataRow *dr = dt->rows[i];

			da_datarow_free(dr);
		}

		da_datarow_free(dt->totrow);

		g_free(dt->rows);

		g_free(dt);
	
	}
}


DataTable *da_datatable_malloc(gshort type, guint32 nbrows, guint32 nbcols)
{
DataTable *dt = g_malloc0(sizeof(DataTable));
guint i;

	DB( g_print("\nda_datatable_malloc\n") );
	
	if(!dt)
		return NULL;
	
	//allocate 2 more slot for total and average
	dt->nbrows = nbrows;
	dt->nbcols = nbcols;
	dt->rows = g_malloc0((nbrows) * sizeof(gpointer));
	for(i=0;i<dt->nbrows;i++)
	{
	DataRow *dr = da_datarow_malloc(dt->nbcols+2);

		//dr->label = ;
		//dr.pos = ;
		dt->rows[i] = dr;
	}

	dt->totrow = da_datarow_malloc(dt->nbcols+2);

	DB( g_print("- @%p, r=%d, c=%d\n", dt, nbrows, nbcols) );

	return dt;
}


gdouble da_datarow_get_cell_sum(DataRow *dr, guint32 index)
{
	if( index <= dr->nbcols )
	{
		return (dr->expense[index] + dr->income[index]);
	}

	g_warning("invalid datarow column");
	return 0;
}


/* = = = = = = = = = = = = = = = = = = = = */


// slide the date to monday of the week
static void hb_date_clamp_iso8601(GDate *date)
{
GDateWeekday wday;

	//ISO 8601 from must be monday, to slice in correct week
	wday = g_date_get_weekday(date);
	g_date_subtract_days (date, wday - G_DATE_MONDAY);
}


static guint date_in_week(guint32 from, guint32 opedate, guint days)
{
GDate *date1, *date2;
gint pos;

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	DB( g_print(" from=%d %02d-%02d-%04d ", 
		g_date_get_weekday(date1), g_date_get_day(date1), g_date_get_month(date1), g_date_get_year(date1)) );

	//#1915643 week iso 8601
	hb_date_clamp_iso8601(date1);
	pos = (opedate - g_date_get_julian(date1)) / days;

	DB( g_print(" shifted=%d %02d-%02d-%04d pos=%d\n", 
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
	return date_in_week(from, opedate, 7);
}	


/*
** return the FortNight list position correponding to the passed date
*/
static guint DateInFortNight(guint32 from, guint32 opedate)
{
	return date_in_week(from, opedate, 14);
}	


/*
** return the month list position correponding to the passed date
*/
static guint DateInMonth(guint32 from, guint32 opedate)
{
GDate *date1, *date2;
guint pos;

	//todo
	// this return sometimes -1, -2 which is wrong

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	pos = ((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1);

	DB( g_print(" from=%d-%d ope=%d-%d => %d\n", g_date_get_month(date1), g_date_get_year(date1), g_date_get_month(date2), g_date_get_year(date2), pos) );

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
guint quarter, pos;

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	//#1758532 shift to first quarter day of 'from date' 
	quarter = ((g_date_get_month(date1)-1)/3)+1;
	DB( g_print("-- from=%02d/%d :: Q%d\n", g_date_get_month(date1), g_date_get_year(date1), quarter) );
	g_date_set_day(date1, 1);
	g_date_set_month(date1, ((quarter-1)*3)+1);

	pos = (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/3;

	DB( g_print("-- from=%02d/%d ope=%02d/%d => pos=%d\n", g_date_get_month(date1), g_date_get_year(date1), g_date_get_month(date2), g_date_get_year(date2), pos) );

	g_date_free(date2);
	g_date_free(date1);

	return(pos);
}


static guint DateInHalfYear(guint32 from, guint32 opedate)
{
GDate *date1, *date2;
guint hyear, pos;

	date1 = g_date_new_julian(from);
	date2 = g_date_new_julian(opedate);

	// shift to first half year of 'from date'
	hyear = ((g_date_get_month(date1)-1)/6)+1;
	DB( g_print("-- from=%02d/%d :: Q%d\n", g_date_get_month(date1), g_date_get_year(date1), hyear) );
	g_date_set_day(date1, 1);
	g_date_set_month(date1, ((hyear-1)*6)+1);
	
	pos = (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/6;

	DB( g_print(" from=%d-%d ope=%d-%d => %d\n", g_date_get_month(date1), g_date_get_year(date1), g_date_get_month(date2), g_date_get_year(date2), pos) );

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

	DB( g_print(" from=%d ope=%d => %d\n", year_from, year_ope, pos) );

	return(pos);
}



void datatable_init_items(DataTable *dt, gint src, guint32 jfrom)
{
GList *list = NULL;
gint pos;
gchar *name;
gchar buffer[64];
guint i;

	DB( g_print("\ndatatable_init_items\n") );
	
	switch(src)
	{
		case REPORT_SRC_CATEGORY:
		case REPORT_SRC_SUBCATEGORY:
			list = category_glist_sorted(1);
			break;
		case REPORT_SRC_PAYEE:
			list = payee_glist_sorted(1);
			break;
		case REPORT_SRC_ACCOUNT:
			list = account_glist_sorted(1);
			break;
		case REPORT_SRC_TAG:
			list = tag_glist_sorted(1);
			break;
	}


	//todo: list_index has a cost
	//preferable to iter through the list
	for(i=0;i<dt->nbrows;i++)
	{
	DataRow *dr = dt->rows[i];
		
		pos = i;
		name = NULL;
		switch(src)
		{
			case REPORT_SRC_CATEGORY:
			case REPORT_SRC_SUBCATEGORY:
				{	
				Category *entry = da_cat_get(i);
					if(entry != NULL)
					{
						name = entry->key == 0 ? _("(no category)") : entry->fullname;
						//if( src == REPORT_SRC_SUBCATEGORY )
							//name = entry->name;
						pos = g_list_index(list, entry);
					}
				}	
				break;
			case REPORT_SRC_PAYEE:
				{
				Payee *entry = da_pay_get(i);
					if(entry != NULL)
					{
						name = entry->key == 0 ? _("(no payee)") : entry->name;
						pos = g_list_index(list, entry);
					}
				}
				break;

			case REPORT_SRC_ACCOUNT:
				{
				Account *entry = da_acc_get(i);
					if(entry != NULL)
					{
						name = entry->name;
						pos = g_list_index(list, entry);
					}
				}
				break;

			case REPORT_SRC_TAG:
				{
				Tag *entry = da_tag_get(i+1);
					if(entry != NULL)
					{
						name = entry->name;
						pos = g_list_index(list, entry);
					}
				}
				break;

			case REPORT_SRC_MONTH:
				report_interval_snprint_name(buffer, sizeof(buffer)-1, REPORT_INTVL_MONTH, jfrom, i);
				name = buffer;
				pos = i;
				break;

			case REPORT_SRC_YEAR:
				report_interval_snprint_name(buffer, sizeof(buffer)-1, REPORT_INTVL_YEAR, jfrom, i);
				name = buffer;
				pos = i;
				break;
		}

		DB( g_print("- init row:%d / pos:%d '%s'\n", i, pos, name) );

		dr->pos = pos;
		dr->label = g_strdup(name);
	}

	g_list_free(list);

}


// this is the add in total mode
void datatable_amount_add(DataTable *dt, guint32 idx, gdouble amount)
{
	if( idx <= (dt->nbrows) )
	{
	DataRow *dr = dt->rows[idx];

		if(amount > 0.0)
		{
			dr->income[1] += amount;
			dt->totinc += amount;
		}
		else
		{
			dr->expense[0] += amount;
			dt->totexp += amount;
		}
	}
	else
		g_warning("invalid datatable index");
}


DataTable *report_compute_total(gint tmpsrc, Filter *flt, GQueue *txn_queue)
{
DataTable *dt;
GList *list;
guint i;
gint n_result;

	DB( g_print("\n[report] compute_total\n") );
	
	//todo: remove this later on
	n_result = report_items_count(tmpsrc, flt->mindate, flt->maxdate);

	DB( g_print(" %d :: n_result=%d\n", tmpsrc, n_result) );
	
	dt = da_datatable_malloc (0, n_result, 3);
	if( dt != NULL )
	{
		datatable_init_items(dt, tmpsrc, flt->mindate);

		list = g_queue_peek_head_link(txn_queue);
		while (list != NULL)
		{
		Transaction *ope = list->data;
			
			DB( g_print("** testing '%s', cat=%d==> %d\n", ope->memo, ope->kcat, filter_txn_match(flt, ope)) );

			if( (filter_txn_match(flt, ope) == 1) )
			{
			guint32 pos = 0;
			gdouble trn_amount;

				DB( g_print(" - should insert\n") );

				trn_amount = report_txn_amount_get(flt, ope);
				trn_amount = hb_amount_base(trn_amount, ope->kcur);


				if( tmpsrc != REPORT_SRC_TAG )
				{
					//for split, affect the amount to the category
					if( (tmpsrc == REPORT_SRC_CATEGORY || tmpsrc == REPORT_SRC_SUBCATEGORY) && ope->flags & OF_SPLIT )
					{
					guint nbsplit = da_splits_length(ope->splits);
					Split *split;
					gboolean status;
					gint sinsert;

						for(i=0;i<nbsplit;i++)
						{
							split = da_splits_get(ope->splits, i);
							status = da_flt_status_cat_get(flt, split->kcat);
							sinsert = ( status == TRUE ) ? 1 : 0;
							if(flt->option[FLT_GRP_CATEGORY] == 2) sinsert ^= 1;

							DB( g_print(" split insert=%d, kcat=%d\n", sinsert, split->kcat) );

							if( (flt->option[FLT_GRP_CATEGORY] == 0) || sinsert)
							{
								pos = category_report_id(split->kcat, (tmpsrc == REPORT_SRC_SUBCATEGORY) ? TRUE : FALSE);
								trn_amount = hb_amount_base(split->amount, ope->kcur);
								datatable_amount_add(dt, pos, trn_amount);
							}
						}
					}
					else
					{
						pos = report_items_get_pos(tmpsrc, flt->mindate, ope);
						datatable_amount_add(dt, pos, trn_amount);
					}
				}
				else
				/* the TAG process is particular */
				{
					if(ope->tags != NULL)
					{
					guint32 *tptr = ope->tags;

						while(*tptr)
						{
							pos = *tptr - 1;
							datatable_amount_add(dt, pos, trn_amount);
							tptr++;
						}
					}
				}
			}
			list = g_list_next(list);
		}
	}
	
	return dt;
}


static void datatable_trend_add(DataTable *dt, guint32 row, guint32 col, gdouble amount, gboolean addtotal)
{
	DB( g_print("   add to r%d,c%d %.2f\n", row, col, amount) );

	if( row <= (dt->nbrows) )
	{
	DataRow *dr = dt->rows[row];

		if( col <= dt->nbcols )
		{
			if(amount > 0.0)
			{
				dr->income[col] += amount;
				dr->income[dt->nbcols] += (amount/dt->nbcols);
				dr->income[dt->nbcols+1] += amount;
				//dt->totinc += amount;
				if( addtotal == TRUE )
				{
					dt->totrow->income[col] += amount;
					dt->totrow->income[dt->nbcols] += (amount/dt->nbcols);
					dt->totrow->income[dt->nbcols+1] += amount;
				}
			}
			else
			{
				dr->expense[col] += amount;
				dr->expense[dt->nbcols] += (amount/dt->nbcols);
				dr->expense[dt->nbcols+1] += amount;
				//dt->totexp += amount;
				if( addtotal == TRUE )
				{
					dt->totrow->expense[col] += amount;
					dt->totrow->expense[dt->nbcols] += (amount/dt->nbcols);
					dt->totrow->expense[dt->nbcols+1] += amount;
				}
			}
		}
	}
	else
		g_warning("invalid datatable index");
}


DataTable *report_compute_trend(gint tmpsrc, gint tmpintvl, Filter *flt, GQueue *txn_queue)
{
DataTable *dt;
GList *list;
guint i;
gint n_result, n_cols;

	DB( g_print("\n[report] compute_trend\n") );
	
	//todo: remove this later on
	n_result = report_items_count(tmpsrc, flt->mindate, flt->maxdate);
	n_cols   = report_interval_count(tmpintvl, flt->mindate, flt->maxdate);

	DB( g_print(" %d :: rows=%d cols=%d\n", tmpsrc, n_result, n_cols) );
	
	dt = da_datatable_malloc (0, n_result, n_cols);
	if( dt != NULL )
	{
		datatable_init_items(dt, tmpsrc, flt->mindate);
		
		list = g_queue_peek_head_link(txn_queue);
		while (list != NULL)
		{
		Transaction *ope = list->data;
			
			if( (filter_txn_match(flt, ope) == 1) )
			{
			guint32 pos = 0;
			guint32 col = 0;
			gdouble trn_amount;

				DB( g_print("\n srctxn '%s' %.2f, cat=%d, pay=%d, acc=%d\n", ope->memo, ope->amount, ope->kcat, ope->kpay, ope->kacc) );
				
				trn_amount = report_txn_amount_get(flt, ope);
				trn_amount = hb_amount_base(trn_amount, ope->kcur);

				col = report_interval_get_pos(tmpintvl, flt->mindate, ope);

				switch( tmpsrc )
				{
					case REPORT_SRC_CATEGORY:
					case REPORT_SRC_SUBCATEGORY:
						//for split, affect the amount to the category
						if( ope->flags & OF_SPLIT )
						{
						guint nbsplit = da_splits_length(ope->splits);
						Split *split;
						gboolean status;
						gint sinsert;

							for(i=0;i<nbsplit;i++)
							{
								split = da_splits_get(ope->splits, i);
								status = da_flt_status_cat_get(flt, split->kcat);
								sinsert = ( status == TRUE ) ? 1 : 0;
								if(flt->option[FLT_GRP_CATEGORY] == 2) sinsert ^= 1;
								if( (flt->option[FLT_GRP_CATEGORY] == 0) || sinsert)
								{
									DB( g_print(" split insert=%d, kcat=%d\n", sinsert, split->kcat) );
									trn_amount = hb_amount_base(split->amount, ope->kcur);
									pos = category_report_id(split->kcat, (tmpsrc == REPORT_SRC_CATEGORY) ? FALSE : TRUE);
									datatable_trend_add(dt, pos, col, trn_amount, TRUE);
									//add to parent as well
									//#1955046 treeview with child was a test faulty released
									/*if( tmpsrc == REPORT_SRC_SUBCATEGORY )
									{
									//#1859279 test cat as subtype here to avoid count twice
									Category *cat = da_cat_get(split->kcat);

										if((cat != NULL) && (cat->flags & GF_SUB))
										{
											pos = cat->parent;
											datatable_trend_add(dt, pos, col, trn_amount, FALSE);
										}
									}*/
								}
							}
						}
						else
						{
							pos = category_report_id(ope->kcat, (tmpsrc == REPORT_SRC_CATEGORY) ? FALSE : TRUE);
							datatable_trend_add(dt, pos, col, trn_amount, TRUE);
							//add to parent as well
							//#1955046 treeview with child was a test faulty released
							/*if( tmpsrc == REPORT_SRC_SUBCATEGORY )
							{
							//#1859279 test cat as subtype here to avoid count twice
							Category *cat = da_cat_get(ope->kcat);

								if((cat != NULL) && (cat->flags & GF_SUB))
								{
									pos = cat->parent;
									datatable_trend_add(dt, pos, col, trn_amount, FALSE);
								}
							}*/
						}
						break;

					case REPORT_SRC_TAG:
						if(ope->tags != NULL)
						{
						guint32 *tptr = ope->tags;

							while(*tptr)
							{
								pos = *tptr - 1;
								datatable_trend_add(dt, pos, col, trn_amount, TRUE);
								tptr++;
							}
						}
						break;

					default:
						pos = report_items_get_pos(tmpsrc, flt->mindate, ope);
						datatable_trend_add(dt, pos, col, trn_amount, TRUE);
						break;

				}
			}
			list = g_list_next(list);
		}
	}

	return dt;
}


DataTable *report_compute_trend_balance(gint tmpsrc, gint tmpintvl, Filter *flt)
{
DataTable *dt;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
gint n_result, n_cols;
guint row, col;

	DB( g_print("\n[report] compute_trend_balance\n") );
	
	//todo: remove this later on
	n_result = report_items_count(tmpsrc, flt->mindate, flt->maxdate);
	n_cols   = report_interval_count(tmpintvl, flt->mindate, flt->maxdate);

	DB( g_print(" %d :: n_result=%d\n", tmpsrc, n_result) );
	
	dt = da_datatable_malloc (0, n_result, n_cols);
	if( dt != NULL )
	{
		datatable_init_items(dt, tmpsrc, flt->mindate);

		// account initial amount
		lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
		lnk_acc = g_list_first(lst_acc);
		while (lnk_acc != NULL)
		{
		Account *acc = lnk_acc->data;
		guint32 pos = 0;
		guint32 col = 0;
		gdouble trn_amount;

			//#1674045 ony rely on nosummary
			if( (acc->flags & (AF_NOREPORT)) )
				goto next_acc;
				
			//#2000294 enable account filtering
			if( filter_acc_match(flt, acc) == 0 )
				goto next_acc;

			//add initial amount
			trn_amount = hb_amount_base(acc->initial, acc->kcur);
			//col = column
			//pos = kacc
			DB( g_print("  ** add initbal %8.2f to row:%d, col:%d\n", trn_amount, acc->key, 0) );
			datatable_trend_add(dt, acc->key, 0, trn_amount, TRUE);

			lnk_txn = g_queue_peek_head_link(acc->txn_queue);
			while (lnk_txn != NULL)
			{
			Transaction *txn = lnk_txn->data;
			
				//5.5 forgot to filter...
				//#1886123 include remind based on user prefs
				if( (txn->status == TXN_STATUS_REMIND) && (PREFS->includeremind == FALSE) )
					goto next_txn;
					
				if( !( txn->status == TXN_STATUS_VOID ) )
				//TODO: enable filters : make no sense no ?
				//if( (filter_txn_match(flt, txn) == 1) )
				{
					pos = report_items_get_pos(tmpsrc, flt->mindate, txn);

					trn_amount = report_txn_amount_get(flt, txn);
					trn_amount = hb_amount_base(trn_amount, txn->kcur);

					DB( g_print("srctxn: %d - cat=%d, pay=%d, acc=%d | [%d - %d]\n", txn->date, txn->kcat, txn->kpay, txn->kacc, flt->mindate, flt->maxdate) );

					//if( (filter_txn_match(flt, txn) == 1) )
					if( (txn->date >= flt->mindate) && (txn->date <= flt->maxdate) )
					{
						col = report_interval_get_pos(tmpintvl, flt->mindate, txn);
						DB( g_print("  add %8.2f row:%d, col:%d\n", trn_amount, pos, col) );
						datatable_trend_add(dt, pos, col, trn_amount, TRUE);
					}
					else
					if( txn->date < flt->mindate)
					{
						DB( g_print("  **add %8.2f row:%d, col:%d\n", trn_amount, pos, 0) );
						datatable_trend_add(dt, pos, 0, trn_amount, TRUE);
					}
				}
			next_txn:
				lnk_txn = g_list_next(lnk_txn);
			}

		next_acc:
			lnk_acc = g_list_next(lnk_acc);
		}
		g_list_free(lst_acc);

	}
	
	//cumulate columns
	if( dt->nbcols > 1 )
	{
		for(row=0;row<dt->nbrows;row++)
		{
		DataRow *dr = dt->rows[row];

			for(col=1;col<dt->nbcols;col++)
			{
				dr->income[col]  += dr->income[col-1];
				dr->expense[col] += dr->expense[col-1];		
			
				dt->totrow->income[col] += dr->income[col-1];

				dt->totrow->expense[col] += dr->expense[col-1];
			}
		}
	}

	return dt;
}


gint report_items_count(gint src, guint32 jfrom, guint32 jto)
{
GDate *date1, *date2;
gint nbsrc = 0;

	switch(src)
	{
		case REPORT_SRC_CATEGORY:
		case REPORT_SRC_SUBCATEGORY:
			nbsrc = da_cat_get_max_key() + 1;
			break;
		case REPORT_SRC_PAYEE:
			nbsrc = da_pay_get_max_key() + 1;
			break;
		case REPORT_SRC_ACCOUNT:
			nbsrc = da_acc_get_max_key() + 1;
			break;
		case REPORT_SRC_TAG:
			nbsrc = da_tag_length();
			break;
		case REPORT_SRC_MONTH:
			date1 = g_date_new_julian(jfrom);
			date2 = g_date_new_julian(jto);
			nbsrc = ((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1) + 1;
			g_date_free(date2);
			g_date_free(date1);
			break;
		case REPORT_SRC_YEAR:
			date1 = g_date_new_julian(jfrom);
			date2 = g_date_new_julian(jto);
			nbsrc = g_date_get_year(date2) - g_date_get_year(date1) + 1;
			g_date_free(date2);
			g_date_free(date1);
			break;
	}

	return nbsrc;
}



gint report_items_get_pos(gint tmpsrc, guint jfrom, Transaction *ope)
{
gint pos = 0;

	switch(tmpsrc)
	{
		case REPORT_SRC_CATEGORY:
			pos = category_report_id(ope->kcat, FALSE);
			break;
		case REPORT_SRC_SUBCATEGORY:
			pos = ope->kcat;
			break;
		case REPORT_SRC_PAYEE:
			pos = ope->kpay;
			break;
		case REPORT_SRC_ACCOUNT:
			pos = ope->kacc;
			break;
		case REPORT_SRC_MONTH:
			pos = DateInMonth(jfrom, ope->date);
			break;
		case REPORT_SRC_YEAR:
			pos = DateInYear(jfrom, ope->date);
			break;
	}
	return pos;
}



gint report_interval_get_pos(gint intvl, guint jfrom, Transaction *ope)
{
gint pos = 0;

	switch(intvl)
	{
		case REPORT_INTVL_DAY:
			pos = ope->date - jfrom;
			break;
		case REPORT_INTVL_WEEK:
			//#1915643 week iso 8601
			//pos = (ope->date - jfrom)/7;
			pos = DateInWeek(jfrom, ope->date);
			break;
		//#2000290
		case REPORT_INTVL_FORTNIGHT:
			pos = DateInFortNight(jfrom, ope->date);
			break;
		case REPORT_INTVL_MONTH:
			pos = DateInMonth(jfrom, ope->date);
			break;
		case REPORT_INTVL_QUARTER:
			pos = DateInQuarter(jfrom, ope->date);
			break;
		case REPORT_INTVL_HALFYEAR:
			pos = DateInHalfYear(jfrom, ope->date);
			break;
		case REPORT_INTVL_YEAR:
			pos = DateInYear(jfrom, ope->date);
			break;
	}
	
	return pos;
}



gint report_interval_count(gint intvl, guint32 jfrom, guint32 jto)
{
GDate *date1, *date2;
gint nbintvl = 0;

	date1 = g_date_new_julian(jfrom);
	date2 = g_date_new_julian(jto);
	
	switch(intvl)
	{
		case REPORT_INTVL_DAY:
			nbintvl = 1 + (jto - jfrom);
			break;
		case REPORT_INTVL_WEEK:
			//#2000292 weeknum iso 8601 as well
			hb_date_clamp_iso8601(date1);
			nbintvl = 1 + (g_date_days_between(date1, date2) / 7);
			break;
		//#2000290
		case REPORT_INTVL_FORTNIGHT:
			hb_date_clamp_iso8601(date1);
			nbintvl = 1 + (g_date_days_between(date1, date2) / 14);		
			break;
		case REPORT_INTVL_MONTH:
			nbintvl = 1 + ((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1);
			break;
		case REPORT_INTVL_QUARTER:
			nbintvl = 1 + (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/3;
			break;
		case REPORT_INTVL_HALFYEAR:
			nbintvl = 1 + (((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1))/6;
			break;
		case REPORT_INTVL_YEAR:
			nbintvl = 1 + g_date_get_year(date2) - g_date_get_year(date1);
			break;
	}

	g_date_free(date2);
	g_date_free(date1);
	
	return nbintvl;
}


void report_interval_snprint_name(gchar *s, gint slen, gint intvl, guint32 jfrom, gint idx)
{
GDate *date = g_date_new_julian(jfrom);
GDateWeekday wday;

	switch(intvl)
	{
		case REPORT_INTVL_DAY:
			g_date_add_days(date, idx);
			g_date_strftime (s, slen, PREFS->date_format, date);
			break;

		case REPORT_INTVL_WEEK:
			g_date_add_days(date, idx*7);

			//#1915643 week iso 8601
			//ISO 8601 from must be monday, to slice in correct week
			wday = g_date_get_weekday(date);
			g_date_subtract_days (date, wday-G_DATE_MONDAY);
			g_date_add_days (date, G_DATE_WEDNESDAY);

			//g_snprintf(s, slen, _("%d-w%d"), g_date_get_year(date), g_date_get_monday_week_of_year(date));
			//TRANSLATORS: printf string for year of week W, ex. 2019-W52 for week 52 of 2019
			g_snprintf(s, slen, _("%d-w%02d"), g_date_get_year(date), g_date_get_iso8601_week_of_year(date));
			break;

		//#2000290
		case REPORT_INTVL_FORTNIGHT:
			hb_date_clamp_iso8601(date);
			g_date_add_days(date, idx*14);
			g_date_strftime (s, slen, PREFS->date_format, date);
			break;

		case REPORT_INTVL_MONTH:
			g_date_add_months(date, idx);
			//g_snprintf(buffer, 63, "%d-%02d", g_date_get_year(date), g_date_get_month(date));
			g_snprintf(s, slen, "%d-%s", g_date_get_year(date), _(CYA_ABMONTHS[g_date_get_month(date)]));
			break;

		case REPORT_INTVL_QUARTER:
			g_date_add_months(date, idx*3);
			//g_snprintf(buffer, 63, "%d-%02d", g_date_get_year(date), g_date_get_month(date));
			//todo: will be innacurrate here if fiscal year start not 1/jan
			//TRANSLATORS: printf string for year of quarter Q, ex. 2019-Q4 for quarter 4 of 2019
			g_snprintf(s, slen, _("%d-q%d"), g_date_get_year(date), ((g_date_get_month(date)-1)/3)+1);
			break;

		case REPORT_INTVL_HALFYEAR:
			g_date_add_months(date, idx*6);
			//#2007712
			//TRANSLATORS: printf string for half-year H, ex. 2019-H1 for 1st half-year of 2019
			g_snprintf(s, slen, _("%d-h%d"), g_date_get_year(date), g_date_get_month(date) < 7 ? 1 : 2);
			break;

		case REPORT_INTVL_YEAR:
			g_date_add_years(date, idx);
			g_snprintf(s, slen, "%d", g_date_get_year(date));
			break;
		default:
			*s ='\0';
			break;
	}

	g_date_free(date);
}


gdouble report_acc_initbalance_get(Filter *flt)
{
GList *lst_acc, *lnk_acc;
gdouble initbal = 0.0;
	
	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		//later, filter on acc
		if( !(acc->flags & AF_NOREPORT) )
			initbal += hb_amount_base(acc->initial, acc->kcur);

		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);
	return initbal;
}


//TODO: maybe migrate this to filter as well
//#1562372 in case of a split we need to take amount for filter categories only
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

				DB( g_print(" split insert=%d, kcat=%d\n", sinsert, split->kcat) );

				if( (flt->option[FLT_GRP_CATEGORY] == 0) || sinsert)
				{
					amount += split->amount;
				}
			}

		}
	}
	return amount;
}

