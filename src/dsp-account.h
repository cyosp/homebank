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

struct register_panel_data
{
	GtkWidget	*window;

	GtkWidget	*IB_duplicate;
	GtkWidget	*LB_duplicate;
	GtkWidget	*NB_txn_daygap;
	
	GtkWidget	*ME_menuacc, *ME_menuedit, *ME_menutxn, *ME_menutools, *ME_menustatus;
	GtkWidget	*MI_exportqif, *MI_exportcsv, *MI_exportpdf, *MI_print, *MI_close;
	GtkWidget	*MI_copy, *MI_pasten, *MI_pastet;
	GtkWidget	*MI_add, *MI_herit, *MI_edit;
	GtkWidget	*MI_statnone, *MI_statclear, *MI_statrecon;
	GtkWidget	*MI_multiedit, *MI_assign, *MI_template, *MI_delete;
	GtkWidget	*MI_find;
	GtkWidget	*MI_markdup, *MI_chkintxfer, *MI_autoassign, *MI_autopayee, *MI_filter, *MI_conveuro;

	GtkWidget	*TB_bar;
	GtkWidget   *BT_add, *BT_herit, *BT_edit;
	GtkWidget   *BT_clear, *BT_reconcile;
	GtkWidget   *BT_multiedit, *BT_template, *BT_delete;
	GtkWidget	*SP_updown, *BT_up, *BT_down;
	GtkWidget	*SW_lockreconciled, *IM_lockreconciled, *LB_lockreconciled;

	GtkWidget	*CY_range;
	GtkWidget	*CM_future;
	GtkWidget	*CY_type;
	GtkWidget	*CY_status;
//	GtkWidget	*CY_month, *NB_year;
	GtkWidget	*BT_reset, *BT_refresh, *BT_filter;
	GtkWidget	*TX_selection;

	GtkWidget   *ST_search;
	
	GtkWidget	*TX_daterange;
	
	//GtkWidget	*IM_closed;
	//GtkWidget   *LB_name;
	GtkWidget	*CM_minor;
	//GtkWidget	*LB_recon, *LB_clear, *LB_today, *LB_futur;
	GtkWidget	*TX_balance[4];

	GtkWidget	*LV_ope;

	GtkWidget	*ME_popmenu, *ME_popmenustatus;
	GtkWidget	*MI_popadd, *MI_popherit, *MI_popedit;
	GtkWidget	*MI_popstatnone, *MI_popstatclear, *MI_popstatrecon;
	GtkWidget	*MI_popmultiedit, *MI_poptemplate, *MI_popassign, *MI_popdelete;
	GtkWidget	*MI_popcopyamount, *MI_popviewsplit, *MI_poptxnup, *MI_poptxndown;
	

	gint	busy;
	gchar	*wintitle;

	Account		*acc;
	Transaction *cur_ope;

	GQueue		*q_txn_clip;	//txn clipboard copy/paste	
	GPtrArray   *gpatxn;		//quickfilter
	
	gboolean	showall;
	gboolean	do_sort;
	
	/* status counters */
	gint		hidden, total, similar;
	gdouble		totalsum;

	Filter		*filter;

	guint		timer_tag;
	
	gulong		handler_id[MAX_HID];

	//gint change;	/* change shouldbe done directly */

};

#define DEFAULT_DELAY 750           /* Default delay in ms */

GtkWidget *register_panel_window_new(Account *acc);
void register_panel_window_init(GtkWidget *widget, gpointer user_data);


#endif /* __HOMEBANK_DSPACCOUNT_H__ */