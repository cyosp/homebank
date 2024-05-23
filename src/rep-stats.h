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

#ifndef __HOMEBANK_REPDIST_H__
#define __HOMEBANK_REPDIST_H__


enum {
	HID_REPDIST_MINDATE,
	HID_REPDIST_MAXDATE,
	HID_REPDIST_RANGE,
	HID_REPDIST_FORECAST,
	HID_REPDIST_VIEW,
	MAX_REPDIST_HID
};


struct repstats_data
{
	DataTable	*trend;
	gint		trendrows;
	gint		trendcols;

	GQueue		*txn_queue;
	Filter		*filter;

	gboolean	detail;
	gboolean	legend;
	gboolean	rate;
	gdouble		total_expense;
	gdouble		total_income;
	gint		charttype;

	GtkWidget	*window;
	GActionGroup *actions;
	gboolean	mapped_done;

	GtkWidget	*TB_bar;
	GtkWidget	*BT_list;
	GtkWidget	*BT_column;
	GtkWidget	*BT_donut;
	GtkWidget	*BT_stack;
	GtkWidget	*BT_stack100;
	GtkWidget	*BT_detail;
	GtkWidget	*BT_legend;
	GtkWidget	*BT_rate;
	GtkWidget	*BT_filter;
	GtkWidget	*BT_refresh;
	GtkWidget	*BT_print;
	GtkWidget	*BT_export;

	
	GtkWidget	*TX_info;
	GtkWidget	*CM_minor;
	GtkWidget	*RA_mode;
	GtkWidget	*CY_src;
	GtkWidget	*CY_type, *LB_type;
	GtkWidget	*CY_intvl, *LB_intvl;
	GtkWidget	*CM_forecast;

	GtkWidget	*GR_listbar;
	GtkWidget	*BT_expand;
	GtkWidget	*BT_collapse;

	//beta start
	GtkWidget	*PO_hubfilter;
	GtkWidget	*BT_reset;
	GtkWidget	*TX_fltactive, *TT_fltactive;
	//beat end
	

	GtkWidget	*RG_zoomx, *LB_zoomx;
	GtkWidget	*SW_total, *SW_trend;
	GtkWidget	*LV_report;
	GtkWidget	*LV_report2;
	
	GtkWidget	*CM_balance;
	GtkWidget	*CM_byamount;
	GtkWidget	*CM_compare;

	GtkWidget	*PO_mindate, *PO_maxdate;

	GtkWidget	*CY_range;
	GtkWidget	*GR_result;

	GtkWidget	*TX_daterange;
	GtkWidget	*TX_total[3];

	GtkWidget	*RE_chart;
	GtkWidget	*RE_chart2;

	GtkWidget	*GR_detail;
	GtkWidget	*LV_detail;

	gulong		handler_id[MAX_REPDIST_HID];
};



GtkWidget *repstats_window_new(void);

#endif
