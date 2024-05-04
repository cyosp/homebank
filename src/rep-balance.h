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

#ifndef __HOMEBANK_REPBALANCE_H__
#define __HOMEBANK_REPBALANCE_H__

enum {
	HID_REPBALANCE_MINDATE,
	HID_REPBALANCE_MAXDATE,
	HID_REPBALANCE_RANGE,
	MAX_REPBALANCE_HID
};


/* list stat */
enum
{
	LST_OVER_OVER,
	LST_OVER_KEY,
	LST_OVER_DATESTR,
	LST_OVER_EXPENSE,
	LST_OVER_INCOME,
	LST_OVER_BALANCE,
	NUM_LST_OVER
};

struct repbalance_data
{
	//DataTable	*trend;

	GList		*ope_list;
	Filter		*filter;

	guint32		jbasedate;
	gdouble		minimum;

	gboolean	detail;
	guint32		accnum;

	guint32		*tmp_acckeys;
	gdouble		*tmp_income;
	gdouble		*tmp_expense;
	gdouble		firstbalance;
	guint		n_result;
	guint		nbovrdraft, nbope;
	guint32		usrkcur;

	GtkWidget	*window;
	gboolean	mapped_done;

	GtkWidget	*TB_bar;
	GtkWidget	*BT_list;
	GtkWidget	*BT_line;
	GtkWidget	*BT_detail;
	GtkWidget	*BT_refresh;
	GtkWidget	*BT_print;
	
	GtkWidget	*TX_info;
	GtkWidget	*TX_daterange;
	GtkWidget	*CM_minor;
	GtkWidget	*LV_report;

	GtkWidget	*BT_all, *BT_non, *BT_inv;
	GtkWidget	*SW_acc, *LV_acc;
	GtkWidget	*CY_intvl;
	GtkWidget	*CM_showempty;
	
	GtkWidget	*RG_zoomx, *LB_zoomx;

	GtkWidget	*PO_mindate, *PO_maxdate;

	GtkWidget	*CY_range;
	GtkWidget	*GR_result;

	GtkWidget	*RE_chart;

	GtkWidget	*GR_detail;
	GtkWidget	*LV_detail;


	gulong		handler_id[MAX_REPBALANCE_HID];


};


GtkWidget *repbalance_window_new(gint32 accnum);


#endif
