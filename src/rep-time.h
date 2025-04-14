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

#ifndef __HOMEBANK_REPTIME_H__
#define __HOMEBANK_REPTIME_H__


enum
{
	LST_HUBREPTIME_POS,
	LST_HUBREPTIME_KEY,
	LST_HUBREPTIME_LABEL,
	LST_HUBREPTIME_EXPENSE,
	LST_HUBREPTIME_INCOME,
	LST_HUBREPTIME_TOTAL,
	LST_HUBREPTIME_FLAGS,
	NUM_LST_HUBREPTIME
};

/* for choose options */
enum
{
	FOR_REPTIME_ACCOUNT,
	FOR_REPTIME_CATEGORY,
	FOR_REPTIME_PAYEE,
	NUM_FOR_REPTIME
};


/* view by choose options */
enum
{
	GROUPBY_REPTIME_DAY,
	GROUPBY_REPTIME_WEEK,
	GROUPBY_REPTIME_MONTH,
	GROUPBY_REPTIME_QUARTER,
	GROUPBY_REPTIME_YEAR,
};


enum {
	HID_REPTIME_MINDATE,
	HID_REPTIME_MAXDATE,
	HID_REPTIME_RANGE,
	HID_REPTIME_VIEW,
	MAX_REPTIME_HID
};

struct reptime_data
{
	GQueue		*txn_queue;
	Filter		*filter;

	gdouble		average;

	gboolean	detail;
	guint32		accnum;

	gint		charttype;

	gdouble		*tmp_income;
	gdouble		*tmp_expense;

	GtkWidget	*window;
	GActionGroup *actions;
	gboolean	mapped_done;

	GtkWidget	*TB_bar;
	GtkWidget	*BT_list;
	GtkWidget	*BT_line;
	GtkWidget	*BT_column;
	GtkWidget	*BT_detail;
	GtkWidget	*BT_filter;
	GtkWidget	*BT_refresh;
	GtkWidget	*BT_reset;
	GtkWidget	*BT_print;
	GtkWidget	*BT_export;
	
	GtkWidget	*TX_info;
	GtkWidget	*TX_fltactive, *TT_fltactive;
	GtkWidget	*TX_daterange;
	GtkWidget	*CY_mode;
	GtkWidget	*CY_intvl;
	GtkWidget	*RG_zoomx, *LB_zoomx;
	GtkWidget	*CM_minor;
	GtkWidget	*CM_cumul;
	GtkWidget	*CM_balance;

	GtkWidget	*LV_report;

	GtkWidget	*GR_itemtype;
	GtkWidget	*CY_src;

	GtkWidget	*BT_all, *BT_non, *BT_inv;
	
	GtkWidget	*SW_acc, *LV_acc;
	GtkWidget	*SW_cat, *LV_cat;
	GtkWidget	*SW_pay, *LV_pay;
	GtkWidget	*SW_tag, *LV_tag;

	GtkWidget   *CM_showempty;

	GtkWidget	*PO_mindate, *PO_maxdate;

	GtkWidget	*CY_range;
	GtkWidget	*GR_result;

	GtkWidget	*RE_chart;

	GtkWidget	*GR_detail;
	GtkWidget	*LV_detail;


	gulong		hid[MAX_REPTIME_HID];

};


GtkWidget *reptime_window_new(guint32 accnum);

#endif
