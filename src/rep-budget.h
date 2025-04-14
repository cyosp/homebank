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

#ifndef __HOMEBANK_REPBUDGET_H__
#define __HOMEBANK_REPBUDGET_H__

enum {
	HID_REPBUDGET_MINDATE,
	HID_REPBUDGET_MAXDATE,
	HID_REPBUDGET_MINMONTHYEAR,
	HID_REPBUDGET_MAXMONTHYEAR,
	HID_REPBUDGET_RANGE,
	MAX_REPBUDGET_HID
};


/* list stat */
enum
{
	LST_BUDGET_POS,
	LST_BUDGET_KEY,
	LST_BUDGET_NAME,
	LST_BUDGET_SPENT,
	LST_BUDGET_BUDGET,
	LST_BUDGET_FULFILLED,
	LST_BUDGET_RESULT,
	LST_BUDGET_STATUS,
	NUM_LST_BUDGET
};

#define LST_BUDGET_POS_UNBUDGETED	G_MAXINT-2


typedef enum
{
	REP_BUD_MODE_TOTAL,
	REP_BUD_MODE_TIME
} HbRepBudMode;


struct repbudget_data
{
	GQueue		*txn_queue;
	Filter		*filter;

	gdouble		total_spent;
	gdouble		total_budget;

	gboolean	detail;
	gboolean	legend;

	GtkWidget	*window;
	GActionGroup *actions;
	gboolean	mapped_done;

	GtkWidget	*TB_bar;
	GtkWidget	*BT_list;
	GtkWidget	*BT_progress;
	GtkWidget	*BT_detail;
	GtkWidget	*BT_refresh;
	GtkWidget	*BT_print;
	GtkWidget	*BT_export;
	
	GtkWidget	*TX_info;
	GtkWidget	*TX_daterange;
	GtkWidget	*CM_untiltoday;
	GtkWidget	*CM_onlyout;
	GtkWidget	*CM_minor;
	GtkWidget	*RA_mode;
	GtkWidget	*CY_type;
	
	GtkWidget	*GR_listbar;
	GtkWidget	*BT_expand;
	GtkWidget	*BT_collapse;

	GtkWidget	*LV_report;

	//GtkWidget	*PO_mindate, *PO_maxdate;
	GtkWidget	*LB_maxdate;
	GtkWidget	*SB_mindate, *SB_maxdate;

	GtkWidget	*CY_range;
	GtkWidget	*GR_result;

	GtkWidget	*TX_total[3];

	GtkWidget	*RE_progress;

	GtkWidget	*GR_detail;
	GtkWidget	*LV_detail;

	gulong		handler_id[MAX_REPBUDGET_HID];
};


GtkWidget *repbudget_window_new(void);


#endif
