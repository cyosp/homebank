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

#ifndef __HB_DSPACCOUNT_H__
#define __HB_DSPACCOUNT_H__


/* official GTK_RESPONSE are negative */
#define HB_RESPONSE_REFRESH	 1


enum
{
	ACTION_ACCOUNT_ADD,
	ACTION_ACCOUNT_INHERIT,
	ACTION_ACCOUNT_EDIT,
	ACTION_ACCOUNT_MULTIEDIT,
	ACTION_ACCOUNT_NONE,
	ACTION_ACCOUNT_CLEAR,
	ACTION_ACCOUNT_RECONCILE,
	ACTION_ACCOUNT_DELETE,
	ACTION_ACCOUNT_FILTER,
	ACTION_ACCOUNT_CLOSE,
	MAX_ACTION_ACCOUNT
};


enum {
	FLG_REG_TITLE     	= 1 << 0,	//1
	FLG_REG_SENSITIVE 	= 1 << 1,	//2
	FLG_REG_BALANCE   	= 1 << 2,	//4
	FLG_REG_VISUAL    	= 1 << 3,	//8
	FLG_REG_REFRESHALL	= 1 << 4	//16
};


enum {
	HID_RANGE,
	HID_TYPE,
	HID_STATUS,
	HID_SEARCH,
	MAX_HID
};

struct hub_ledger_data
{
	GtkWidget	*window;
	GActionGroup *actions;

	GtkWidget	*IB_accnotif, *LB_accnotif, *BT_info_showpending;
	GtkWidget	*IB_duplicate, *LB_duplicate, *NB_txn_daygap;
	GtkWidget	*IB_chkcatsign, *LB_chkcatsign;

	GtkWidget	*TB_bar;
	GtkWidget   *BT_add, *BT_herit, *BT_edit;
	GtkWidget   *BT_clear, *BT_reconcile;
	GtkWidget   *BT_multiedit, *BT_delete;
	GtkWidget	*BT_up, *BT_down;
	GtkWidget	*SW_lockreconciled, *IM_lockreconciled, *LB_lockreconciled;

	GtkWidget	*CY_range;
	GtkWidget	*CM_future;
	GtkWidget	*CY_flag;
	GtkWidget	*CY_type;
	GtkWidget	*CY_status;
//	GtkWidget	*CY_month, *NB_year;
	GtkWidget	*PO_hubfilter;
	GtkWidget	*BT_reset, *BT_refresh, *BT_filter;
	GtkWidget	*BT_lifnrg;
	GtkWidget	*TX_selection;

	GtkWidget   *ST_search;
	
	GtkWidget	*TX_daterange;
	
	//GtkWidget	*IM_closed;
	//GtkWidget   *LB_name;
	GtkWidget	*CM_minor;
	//GtkWidget	*LB_recon, *LB_clear, *LB_today, *LB_futur;
	GtkWidget	*TX_balance[4];

	GtkWidget	*LV_ope;

	gint	busy;
	gchar	*wintitle;

	Account		*acc;
	Transaction *cur_ope;

	GQueue		*q_txn_clip;	//txn clipboard copy/paste	
	GPtrArray   *gpatxn;		//quickfilter
	
	gboolean	showall;
	gboolean	closed;
	gboolean	lockreconciled;
	gboolean	do_sort;
	
	/* status counters */
	guint		nb_pending;
	gint		hidden, total, similar, chkcatsign;
	gdouble		totalsum;

	Filter		*filter;

	guint		timer_tag;
	
	gulong		handler_id[MAX_HID];

	//gint change;	/* change shouldbe done directly */

};

#define DEFAULT_DELAY 750           /* Default delay in ms */

GtkWidget *hub_ledger_window_new(Account *acc);
void hub_ledger_window_init(GtkWidget *widget, gpointer user_data);


#endif /* __HOMEBANK_DSPACCOUNT_H__ */
