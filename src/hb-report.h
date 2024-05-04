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

#ifndef __HB_REPORT_H__
#define __HB_REPORT_H__


typedef enum
{
	REPORT_SRC_NONE,
	REPORT_SRC_CATEGORY,
	//REPORT_SRC_SUBCATEGORY,
	REPORT_SRC_PAYEE,
	REPORT_SRC_ACCOUNT,
	REPORT_SRC_TAG,
	REPORT_SRC_MONTH,
	REPORT_SRC_YEAR,
} HbReportSrc;


typedef enum {
	REPORT_TYPE_EXPENSE,
	REPORT_TYPE_INCOME,
	REPORT_TYPE_TOTAL
} HbReportType;


typedef enum
{
	REPORT_INTVL_NONE,
	REPORT_INTVL_DAY,
	REPORT_INTVL_WEEK,
	REPORT_INTVL_FORTNIGHT,
	REPORT_INTVL_MONTH,
	REPORT_INTVL_QUARTER,
	REPORT_INTVL_HALFYEAR,
	REPORT_INTVL_YEAR,
} HbReportIntvl;


typedef enum
{
	REPORT_RESULT_TOTAL,
	REPORT_RESULT_CUMUL,
	REPORT_RESULT_BALANCE,
} HbReportResult;


typedef struct _datatable DataTable;
typedef struct _datarow DataRow;
typedef struct _datacol DataCol;


typedef struct _carcost CarCost;

struct _carcost
{
	guint32		kparent;
	guint32		kcat;
	guint32		date;
	gchar		*memo;
	gdouble		amount;
	gboolean	partial;
	guint		meter;
	gdouble		fuel;
	guint		dist;
};


CarCost *da_vehiclecost_malloc(void);
void da_vehiclecost_free(CarCost *item);
void da_vehiclecost_destroy(GList *list);


enum {
	REPORT_MODE_TOTAL,
	REPORT_MODE_TREND
};


void da_datatable_free(DataTable *dt);

gdouble da_datarow_get_cell_sum(DataRow *dr, guint32 index);


DataTable *report_compute(gint src, gint intvl, Filter *flt, GQueue *txn_queue, gboolean do_forecast, gboolean do_balance);

DataCol *report_data_get_col(DataTable *dt, guint32 idx);
DataRow *report_data_get_row(DataTable *dt, guint32 row);
guint report_items_get_key(gint tmpsrc, guint jfrom, Transaction *ope);

gint report_interval_get_pos(gint intvl, guint jfrom, Transaction *ope);
gint report_interval_count(gint intvl, guint32 jfrom, guint32 jto);

guint32 report_interval_snprint_name(gchar *s, gint slen, gint intvl, guint32 jfrom, gint idx);

gdouble report_txn_amount_get(Filter *flt, Transaction *txn);


struct _datarow
{
	guint32		nbcols;
	guint32		pos;		//used for sort
	gchar		*label;		//row label
	gshort		flags;		//See below
	gshort	  	pad1;
	gchar		*xlabel;	//short label
	gchar		*misclabel;	//host top label: year, today, etc
	gdouble		*colexp;	//array for each row column
	gdouble		*colinc;	//array for each row column
	gdouble		rowexp;		//row total expense
	gdouble		rowinc;		//row total income
};


struct _datacol
{
	gchar		*label;		//long label
	gshort		flags;		//See below
	gshort  	pad1;
	gchar		*xlabel;	//short label
	gchar		*misclabel;	//host top label: year, today, etc
};

#define RF_NEWYEAR 		(1<<1)
#define RF_FORECAST		(1<<2)


struct _datatable
{
	guint32		nbkeys;		//maximum key value for items (row)
	guint32		nbrows;		//nb of items (length): cat/subcat/pay/acc/...
	guint32		nbcols;		//nb of intervals: d, w, m, q, hy, y
	guint32		maxpostdate;

	guint32		*keyindex;	//array of correspondance key => index in rows
	guint32		*keylist;
	DataRow		**rows;		//array of _datarow struct per key of item
	DataRow		*totrow;	//for trend
	DataCol		**cols;		//array of datacol

};


#endif

