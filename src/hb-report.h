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
	REPORT_SRC_CATEGORY,
	REPORT_SRC_SUBCATEGORY,
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

void da_datatable_free(DataTable *dt);
DataTable *da_datatable_malloc(gshort type, guint32 nbrows, guint32 nbcols);

gdouble da_datarow_get_cell_sum(DataRow *dr, guint32 index);

void datatable_init_items(DataTable *dt, gint src, guint32 jfrom);

void datatable_amount_add(DataTable *dt, guint32 idx, gdouble value);


DataTable *report_compute_total(gint tmpsrc, Filter *flt, GQueue *txn_queue);
DataTable *report_compute_trend(gint tmpsrc, gint tmpintvl, Filter *flt, GQueue *txn_queue);
DataTable *report_compute_trend_balance(gint tmpsrc, gint tmpintvl, Filter *flt);

gint report_items_count(gint src, guint32 jfrom, guint32 jto);
gint report_items_get_pos(gint tmpsrc, guint jfrom, Transaction *ope);

gint report_interval_get_pos(gint intvl, guint jfrom, Transaction *ope);
gint report_interval_count(gint intvl, guint32 jfrom, guint32 jto);
void report_interval_snprint_name(gchar *s, gint slen, gint intvl, guint32 jfrom, gint idx);

gdouble report_acc_initbalance_get(Filter *flt);
gdouble report_txn_amount_get(Filter *flt, Transaction *txn);


struct _datarow
{
	//guint32		key;
	guint32		pos;		//used for sort
	//gint		type;
	gchar		*label;		//row label

	//todo: for total mode: I should just keep some simple gdouble inc/exp here
	//and do not allocate extra memory for nothing
	guint32		nbcols;
	gdouble		*expense;	//array for each row column
	gdouble		*income;
};


struct _datatable
{
	gshort		mode;		//0=total / 1=time

	guint32		nbrows;		//nb of items: cat/subcat/pay/acc/...
	guint32		nbcols;		//nb of intervals: d, w, m, q, hy, y

	DataRow		**rows;		//array of _datarow struct per key of item
	//gchar		**coltitle;	//array of column title

	DataRow		*totrow;	//for trend

	gdouble		totexp;
	gdouble		totinc;
};


#endif

