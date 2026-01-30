/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2025 Maxime DOYEN
 *
 *	This file is part of HomeBank.
 *
 *	HomeBank is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	HomeBank is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 */


#include "homebank.h"

#include "dsp-account.h"
#include "dsp-mainwindow.h"

#include "list-operation.h"
#include "hub-account.h"

#include "gtk-dateentry.h"
#include "ui-filter.h"
#include "ui-transaction.h"
#include "ui-txn-multi.h"
#include "ui-flt-widget.h"
#include "ui-dialogs.h"
#include "ui-widgets.h"



/****************************************************************************/
/* Debug macros											                    */
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

extern HbKvData CYA_FLT_TYPE[];
extern HbKvData CYA_FLT_STATUS[];


/* = = = = = = = = = = = = = = = = */


static void
_list_txn_selection_count_type(GtkTreeView *treeview, gint *nbrecon, gint *nbpending)
{
GtkTreeModel *model;
GList *lselection, *list;
gint tmprecon = 0;
gint tmppending = 0;

	DB( g_print("\n[hub-ledger] selection count type\n") );

	model = gtk_tree_view_get_model(treeview);
	lselection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(treeview), &model);

	list = g_list_last(lselection);
	while(list != NULL)
	{
	GtkTreeIter iter;
	Transaction *txn;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &txn, -1);
		if(txn->status == TXN_STATUS_RECONCILED)
			tmprecon++;
		if(txn->flags & (OF_ISIMPORT|OF_ISPAST))
			tmppending++;

		list = g_list_previous(list);
	}

	g_list_foreach(lselection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(lselection);

	if(nbrecon)
		*nbrecon = tmprecon;
	if(nbpending)
		*nbpending = tmppending;

}


static void
hub_ledger_balance_refresh(GtkWidget *view)
{
struct hub_ledger_data *data;
Transaction *minbalope;
GList *list;
gdouble balance;
GtkTreeModel *model;
gdouble lbalance = 0;
guint32 ldate = 0;
gushort lpos = 1;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(view, GTK_TYPE_WINDOW)), "inst_data");

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[hub-ledger] balance refresh kacc=%d\n", data->acc != NULL ? (gint)data->acc->key : -1) );

	balance = data->acc->initial;

	//#1270687: sort if date changed
	if(data->do_sort)
	{
		DB( g_print(" complete txn sort\n") );
		
		da_transaction_queue_sort(data->acc->txn_queue);
		data->do_sort = FALSE;
	}

	minbalope = NULL;
	list = g_queue_peek_head_link(data->acc->txn_queue);
	while (list != NULL)
	{
	Transaction *ope = list->data;
	gdouble value;

		//#1267344 maybe no remind in running balance
		if( transaction_is_balanceable(ope) )
			balance += ope->amount;

		ope->balance = balance;

		// clear mark flags
		ope->dspflags &= ~(FLAG_TMP_OVER|FLAG_TMP_LOWBAL);

		//#1661806 add show overdraft
		//#1672209 added round like for #400483
		value = hb_amount_round(balance, 2);
		if( (value != 0.0) && (value < data->acc->minimum) )
		{
			ope->dspflags |= FLAG_TMP_OVER;
		}
	
		//# mark lowest balance for future
		if ((ope->date > GLOBALS->today))
		{
			if( balance < lbalance )
				minbalope = ope;
		}
		
		if(ope->date == ldate)
		{
			ope->pos = ++lpos;	
		}
		else
		{
			ope->pos = lpos = 1;
		}

		ldate = ope->date;
		lbalance = balance;

		list = g_list_next(list);
	}

	if( minbalope != NULL )
		minbalope->dspflags |= FLAG_TMP_LOWBAL;

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	list_txn_sort_force(GTK_TREE_SORTABLE(model), NULL);
	
}


static void
hub_ledger_update(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;
GtkTreeSelection *selection;
gint flags = GPOINTER_TO_INT(user_data);
gboolean lockrecon, visible;
gint count = 0;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//data = INST_DATA(widget);

	DB( g_print("\n[hub-ledger] update kacc=%d\n", data->acc != NULL ? (gint)data->acc->key : -1) );

	
	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	/* set window title */
	if(flags & FLG_REG_TITLE)
	{
		DB( g_print("\n FLG_REG_TITLE\n") );

	}

	/* update toolbar & list */
	if(flags & FLG_REG_VISUAL)
	{
	//gboolean visible;
		
		DB( g_print("\n FLG_REG_VISUAL\n") );

		//minor ?
		hb_widget_visible (data->CM_minor, PREFS->euro_active);

		//TODO: balance to show/hide
		/*
		visible = (homebank_pref_list_column_get(PREFS->lst_acc_columns, COL_DSPACC_RECON, NUM_LST_COL_DSPACC) < 0) ? FALSE : TRUE;
		hb_widget_visible (data->LB_recon, visible);
		hb_widget_visible (data->TX_balance[0], visible);
		
		visible = (homebank_pref_list_column_get(PREFS->lst_acc_columns, COL_DSPACC_CLEAR, NUM_LST_COL_DSPACC) < 0) ? FALSE : TRUE;
		hb_widget_visible (data->LB_clear, visible);
		hb_widget_visible (data->TX_balance[1], visible);

		visible = (homebank_pref_list_column_get(PREFS->lst_acc_columns, COL_DSPACC_TODAY, NUM_LST_COL_DSPACC) < 0) ? FALSE : TRUE;
		hb_widget_visible (data->LB_today, visible);
		hb_widget_visible (data->TX_balance[2], visible);

		visible = (homebank_pref_list_column_get(PREFS->lst_acc_columns, COL_DSPACC_FUTURE, NUM_LST_COL_DSPACC) < 0) ? FALSE : TRUE;
		hb_widget_visible (data->LB_futur, visible);
		hb_widget_visible (data->TX_balance[3], visible);
		*/
	}

	/* update balances */
	if(flags & FLG_REG_BALANCE)
	{
		DB( g_print("\n FLG_REG_BALANCE\n") );

		if(data->showall == FALSE)
		{
		Account *acc = data->acc;

			hub_ledger_balance_refresh(widget);

			DB( g_print(" update 4 balances widget\n") );
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[0]), acc->bal_recon, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[1]), acc->bal_clear, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[2]), acc->bal_today, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[3]), acc->bal_future, acc->kcur, GLOBALS->minor);
		}
		else
		{
		GList *lst_acc, *lnk_acc;
		gdouble recon, clear, today, future;
	
			recon = clear = today = future = 0.0;
			lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
			lnk_acc = g_list_first(lst_acc);
			while (lnk_acc != NULL)
			{
			Account *acc = lnk_acc->data;

				recon += hb_amount_base(acc->bal_recon, acc->kcur);
				clear += hb_amount_base(acc->bal_clear, acc->kcur);
				today += hb_amount_base(acc->bal_today, acc->kcur);
				future += hb_amount_base(acc->bal_future, acc->kcur);
				
				lnk_acc = g_list_next(lnk_acc);
			}
			g_list_free(lst_acc);
		
			DB( g_print(" update 4 balances widget\n") );
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[0]), recon, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[1]), clear, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[2]), today, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[3]), future, GLOBALS->kcur, GLOBALS->minor);
		}
		ui_hub_account_compute(GLOBALS->mainwindow, NULL);
	}

	/* update disabled things */
	if(flags & FLG_REG_SENSITIVE)
	{
	gboolean sensitive, psensitive, nsensitive;
	GtkTreeModel *model;
	gint sort_column_id;
	GtkSortType order;
	Transaction *ope;

		DB( g_print(" FLG_REG_SENSITIVE\n") );

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
		count = gtk_tree_selection_count_selected_rows(selection);
		DB( g_print(" count = %d\n", count) );

		ope = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));

		//showall part
		sensitive = !data->showall;
		//hb_widget_visible(data->MI_exportqif, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "expqif")), sensitive);
		//hb_widget_visible(data->MI_browse, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "browse")), sensitive);
		//tools
		//hb_widget_visible(data->MI_markdup, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "mrksign")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "mrkdup")), sensitive);
		//hb_widget_visible(data->MI_chkintxfer, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "chkxfer")), sensitive);
		//#1873248 Auto. assignments faulty sensitive on 'All transactions' window
		//hb_widget_visible(data->MI_autoassign, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "runasg")), sensitive);

		//1909749 lock/unlock reconciled
		lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));
		if( ope != NULL )
		{
			if( (ope->status != TXN_STATUS_RECONCILED) )
				lockrecon = FALSE;

			DB( g_print(" lockrecon = %d (%d %d)\n", lockrecon, ope->status != TXN_STATUS_RECONCILED, gtk_switch_get_state (GTK_SWITCH(data->SW_lockreconciled)) ) );
		}


		//5.3.1 if closed account : disable any change
		sensitive =  TRUE;
		if( data->closed == TRUE )
			sensitive = FALSE;

		gtk_widget_set_sensitive(data->TB_bar, sensitive);

		//todo: subsititute ? or check carrefully
		//gtk_widget_set_sensitive(data->ME_menuedit, sensitive);
		//gtk_widget_set_sensitive(data->ME_menutxn, sensitive);
		//gtk_widget_set_sensitive(data->ME_menutools, sensitive);
		//gtk_widget_set_sensitive(data->ME_popmenu, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txncopy")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnpaste")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnpastet")), sensitive);

		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "mrksign")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "mrkdup")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "chkxfer")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "runasg")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "convert")), sensitive);

		//5.7 browse menu
		sensitive = account_has_website(data->acc);
		//gtk_widget_set_sensitive(data->MI_browse, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "browse")), sensitive);

		// multiple: disable inherit, edit
		sensitive = (count != 1 ) ? FALSE : TRUE;
		//gtk_widget_set_sensitive(data->MI_herit, sensitive);
		//gtk_widget_set_sensitive(data->MI_popherit, sensitive);					
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnherit")), sensitive);

		//gtk_widget_set_sensitive(data->MI_edit, lockrecon ? FALSE : sensitive);
		//gtk_widget_set_sensitive(data->MI_popedit, lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnedit")), lockrecon ? FALSE : sensitive);

		gtk_widget_set_sensitive(data->BT_herit, sensitive);
		gtk_widget_set_sensitive(data->BT_edit, lockrecon ? FALSE : sensitive);
		//gtk_widget_set_sensitive(data->MI_popcopyamount, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "copyamt")), sensitive);

		//txn have split
		sensitive = (count == 1) && (ope != NULL) && (ope->flags & OF_SPLIT) ? TRUE : FALSE;
		//gtk_widget_set_sensitive(data->MI_popviewsplit, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "viewsplit")), sensitive);
		
		// single: disable multiedit
		sensitive = (count <= 1 ) ? FALSE : TRUE;
		//1909749 lock/unlock reconciled
		gint nbrecon, nbpending;
		_list_txn_selection_count_type(GTK_TREE_VIEW(data->LV_ope), &nbrecon, &nbpending);
		if( (nbrecon > 0) && (gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled)) == TRUE) )
			sensitive = FALSE;
		//gtk_widget_set_sensitive(data->MI_multiedit   , sensitive);
		//gtk_widget_set_sensitive(data->MI_popmultiedit, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnmedit")), sensitive);
		gtk_widget_set_sensitive(data->BT_multiedit   , sensitive);

		//pending action
		sensitive = (count >=1 && nbpending > 0) ? TRUE : FALSE;
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnapprove")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnreject")), sensitive);

		// no selection: disable reconcile, delete
		sensitive = (count > 0 ) ? TRUE : FALSE;
		//gtk_widget_set_sensitive(data->MI_copy, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txncopy")), sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnclip")), sensitive);
		//gtk_widget_set_sensitive(data->ME_menustatus, lockrecon ? FALSE : sensitive);
		//gtk_widget_set_sensitive(data->ME_popmenustatus, lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "stanon")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "staclr")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "starec")), lockrecon ? FALSE : sensitive);

		//#1600356
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "flgn")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "flg1")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "flg2")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "flg3")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "flg4")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "flg5")), lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "flg6")), lockrecon ? FALSE : sensitive);

		//gtk_widget_set_sensitive(data->MI_delete, lockrecon ? FALSE : sensitive);
		//gtk_widget_set_sensitive(data->MI_popdelete, lockrecon ? FALSE : sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txndel")), lockrecon ? FALSE : sensitive);
		//gtk_widget_set_sensitive(data->MI_assign, sensitive);
		//gtk_widget_set_sensitive(data->MI_popassign, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "newasg")), sensitive);
		//gtk_widget_set_sensitive(data->MI_template, sensitive);
		//gtk_widget_set_sensitive(data->MI_poptemplate, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "newtpl")), sensitive);
		gtk_widget_set_sensitive(data->BT_delete, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->BT_clear, lockrecon ? FALSE : sensitive);
		gtk_widget_set_sensitive(data->BT_reconcile, lockrecon ? FALSE : sensitive);

		//edit menu
		sensitive = g_queue_get_length(data->q_txn_clip) > 0 ? TRUE : FALSE;
		//gtk_widget_set_sensitive(data->MI_pasten, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnpaste")), sensitive);
		//gtk_widget_set_sensitive(data->MI_pastet, sensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnpastet")), sensitive);

		// euro convert
		visible = (data->showall == TRUE) ? FALSE : PREFS->euro_active;
		if( (data->acc != NULL) && currency_is_euro(data->acc->kcur) )
			visible = FALSE;
		//hb_widget_visible(data->MI_conveuro, visible);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "convert")), visible);

		//move up/down button : not when showall and when sort is not date
		visible = FALSE;
		psensitive = FALSE;
		nsensitive = FALSE;
		if( count == 1 && data->showall == FALSE )
		{
		Transaction *prevtxn, *nexttxn;

			visible = TRUE;
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
			gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), &sort_column_id, &order);
			if( (data->showall == TRUE) || (sort_column_id != LST_DSPOPE_DATE) )
				visible = FALSE;

			prevtxn = NULL; nexttxn = NULL;
			Transaction *txn = list_txn_get_surround_transaction(GTK_TREE_VIEW(data->LV_ope), &prevtxn, &nexttxn);

			if( prevtxn && txn )
			{
				psensitive = (prevtxn->date == txn->date) ? TRUE : FALSE;
			}

			if( nexttxn && txn )
			{
				nsensitive = (nexttxn->date == txn->date) ? TRUE : FALSE;
			}
		}

		//hb_widget_visible(data->SP_updown, visible);
		//hb_widget_visible(data->BT_up, visible);
		//hb_widget_visible(data->BT_down, visible);
		//todo: why assign sesnitivity twice ??
		//hb_widget_visible(data->MI_poptxnup, visible);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnup")), sensitive);
		//hb_widget_visible(data->MI_poptxndown, visible);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txndw")), sensitive);
		//gtk_widget_set_sensitive(data->MI_poptxnup, psensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnup")), psensitive);
		gtk_widget_set_sensitive(data->BT_up, psensitive);
		//gtk_widget_set_sensitive(data->MI_poptxndown, nsensitive);
		g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txndw")), nsensitive);
		gtk_widget_set_sensitive(data->BT_down, nsensitive);
	}

	//#1875100 show infobar for pending
	visible = data->nb_pending > 0 ? TRUE : FALSE;
	hb_widget_visible(data->IB_accnotif, visible);
	if( visible == TRUE )
	{
	gchar *accmsg = g_strdup_printf(_("%d requires approval"), data->nb_pending);
		gtk_label_set_markup(GTK_LABEL(data->LB_accnotif), accmsg);
		g_free (accmsg);
	}

	//#1835588
	visible = PREFS->date_future_nbdays > 0 ? TRUE : FALSE;
	if( !(filter_preset_daterange_future_enable( data->filter, hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range)) )) )
		visible = FALSE;
	hb_widget_visible(data->CM_future, visible);
	DB( g_print(" show future=%d\n", visible) );

	
	/* update fltinfo */
	DB( g_print(" FLG_REG_INFOBAR\n") );


	DB( g_print(" statusbar\n") );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
	count = gtk_tree_selection_count_selected_rows(selection);
	DB( g_print(" nb selected = %d\n", count) );

	/* if more than one ope selected, we make a sum to display to the user */
	gdouble opeexp = 0.0;
	gdouble opeinc = 0.0;
	gchar buf1[64];
	gchar buf2[64];
	gchar buf3[64];
	gchar fbufavg[64];
	guint32 kcur;

	kcur = (data->showall == TRUE) ? GLOBALS->kcur : data->acc->kcur;


	if( count >= 1 )
	{
	GList *list, *tmplist;
	GtkTreeModel *model;
	GtkTreeIter iter;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
		list = gtk_tree_selection_get_selected_rows(selection, &model);
		tmplist = g_list_first(list);
		while (tmplist != NULL)
		{
		Transaction *item;

			gtk_tree_model_get_iter(model, &iter, tmplist->data);
			gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &item, -1);

			if( data->showall == FALSE )
			{
				if( item->flags & OF_INCOME )
					opeinc += item->amount;
				else
					opeexp += item->amount;
			}
			else
			{
				if( item->flags & OF_INCOME )
					opeinc += hb_amount_base(item->amount, item->kcur);
				else
					opeexp += hb_amount_base(item->amount, item->kcur);
			}

			DB( g_print(" memo='%s', %.2f\n", item->memo, item->amount ) );

			tmplist = g_list_next(tmplist);
		}
		g_list_free(list);

		DB( g_print(" %f - %f = %f\n", opeinc, opeexp, opeinc + opeexp) );


		hb_strfmon(buf1, 64-1, opeinc, kcur, GLOBALS->minor);
		hb_strfmon(buf2, 64-1, -opeexp, kcur, GLOBALS->minor);
		hb_strfmon(buf3, 64-1, opeinc + opeexp, kcur, GLOBALS->minor);
		hb_strfmon(fbufavg, 64-1, (opeinc + opeexp) / count, kcur, GLOBALS->minor);
	}

	DB( g_print(" update selection message\n") );
	gchar *msg;

	if( count <= 1 )
	{
		msg = g_strdup_printf(_("%d transactions"), data->total);
	}
	else
		msg = g_strdup_printf(_("%d transactions, %d selected, avg: %s, sum: %s (%s - %s)"), data->total, count, fbufavg, buf3, buf1, buf2);

	gtk_label_set_markup(GTK_LABEL(data->TX_selection), msg);
	g_free (msg);

	//5.6 update lock/unlock
	DB( g_print(" update lock/unlock\n") );
	lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));
	DB( g_print(" lockrecon=%d\n", lockrecon) );

	list_txn_set_lockreconciled(GTK_TREE_VIEW(data->LV_ope), lockrecon);
	gchar *iconname = lockrecon == TRUE ? ICONNAME_CHANGES_PREVENT : ICONNAME_CHANGES_ALLOW;
	g_object_set(data->IM_lockreconciled, "icon-name", iconname, NULL);
	
	gtk_widget_set_tooltip_text (data->SW_lockreconciled, 
		lockrecon == TRUE ? _("Locked. Click to unlock") : _("Unlocked. Click to lock"));
	
	DB( g_print(" redraw LV_ope\n") );
	gtk_widget_queue_draw (data->LV_ope);

}


/* these 5 functions are independant from account window */

/* account functions -------------------- */

static void
hub_ledger_edit_multiple(GtkWidget *widget, Transaction *txn, gint column_id, gpointer user_data)
{
struct hub_ledger_data *data;
GtkWidget *dialog;

	DB( g_print("\n[hub-ledger] edit multiple\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" txn:%p, column: %d\n", txn, column_id) );

	dialog = ui_multipleedit_dialog_new(GTK_WINDOW(data->window), GTK_TREE_VIEW(data->LV_ope));

	if(txn != NULL && column_id != 0)
	{
		ui_multipleedit_dialog_prefill(dialog, txn, column_id);
	}

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if( result == GTK_RESPONSE_ACCEPT )
	{
	gboolean do_sort;
	gint changes;

		//#1792808: sort if date changed 
		changes = ui_multipleedit_dialog_apply (dialog, &do_sort);
		data->do_sort = do_sort;
		if( changes > 0 )
		{
			//#1782749 update account status
			if( data->acc != NULL )
				data->acc->dspflags |= FLAG_ACC_TMP_EDITED;

			ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
		}
	}

	gtk_window_destroy (GTK_WINDOW(dialog));
	
	hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));
}

/*  ---end ---- */

static void
hub_ledger_action_move_up(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
Transaction *txn = NULL;
Transaction *prevtxn = NULL;
gint count = 0;
	
	DB( g_print("\n[hub-ledger] move up\n\n") );

	txn = list_txn_get_surround_transaction(GTK_TREE_VIEW(data->LV_ope), &prevtxn, NULL);
	if( txn && prevtxn )
	{
		if( txn->date == prevtxn->date )
		{
			DB( g_print(" swapping, as txn are same date\n") );
			//swap position
			gint savedpos = txn->pos;
			txn->pos = prevtxn->pos;
			prevtxn->pos = savedpos;
			GLOBALS->changes_count++;
			count++;
		}
	}

	if( count > 0 )
	{
		data->do_sort = TRUE;
		hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));
	}		
}


static void
hub_ledger_action_move_down(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
Transaction *txn = NULL;
Transaction *nexttxn = NULL;
gint count = 0;

	DB( g_print("\n[hub-ledger] move down\n\n") );

	txn = list_txn_get_surround_transaction(GTK_TREE_VIEW(data->LV_ope), NULL, &nexttxn);
	if( txn && nexttxn )
	{
		if( txn->date == nexttxn->date )
		{
			DB( g_print(" swapping, as txn are same date\n") );
			//swap position
			gint savedpos = txn->pos;
			txn->pos = nexttxn->pos;
			nexttxn->pos = savedpos;
			GLOBALS->changes_count++;
			count++;
		}
	}

	if( count > 0 )
	{
		data->do_sort = TRUE;
		hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));
	}		
}


static gboolean
hub_ledger_cb_recon_change (GtkWidget *widget, gboolean state, gpointer user_data)
{
struct hub_ledger_data *data;

	DB( g_print("\n[hub-ledger] cb recon change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	
	data->lockreconciled = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));
	
	DB( g_print(" state=%d switch=%d\n", state, data->lockreconciled ) );

	hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE));

	return FALSE;
}


static void
hub_ledger_collect_filtered_txn(GtkWidget *view, gboolean emptysearch)
{
struct hub_ledger_data *data;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
gint flag, status;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(view, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[hub-ledger] collect_filtered_txn - kacc=%d\n", data->acc != NULL ? (gint)data->acc->key : -1) );

	if(data->gpatxn != NULL)
		g_ptr_array_free (data->gpatxn, TRUE);

	//our txn storage to populate and quickfilter
	data->gpatxn = g_ptr_array_sized_new(64);

	flag   = kiv_combo_box_get_active(GTK_COMBO_BOX(data->CY_flag));
	status = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_status));
	DB( g_print(" flag=%d\n", flag) );

	data->nb_pending = 0;

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		// skip closed in showall mode
		//#1861337 users want them
		//if( data->showall == TRUE && (acc->flags & AF_CLOSED) )
		//	goto next_acc;

		// skip other than current in normal mode
		if( (data->showall == FALSE) && (data->acc != NULL) && (acc->key != data->acc->key) )
			goto next_acc;

		data->nb_pending += acc->nb_pending;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *ope = lnk_txn->data;
		gboolean insert = FALSE;

			//#1875100 skip any filter but pending
			//#2109854 really show all txn (void were hidden)
			if( status==FLT_STATUS_UNAPPROVED )
			{
				if( ope->flags & (OF_ISIMPORT|OF_ISPAST) )
					insert = TRUE;
			}
			else
			{
				if( filter_txn_match(data->filter, ope) == 1 )
				{
					//#1600356 filter flag
					if( (flag == GRPFLAG_ANY) || (ope->grpflg == flag) )
					{
						insert = TRUE;
					}
				}
			}

			//add to the list
			if( insert == TRUE )
			{
				g_ptr_array_add(data->gpatxn, (gpointer)ope);
			}

			lnk_txn = g_list_next(lnk_txn);
		}
	
	next_acc:
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	//#1789698 not always empty
	if( emptysearch == TRUE )
	{
		g_signal_handler_block(data->ST_search, data->handler_id[HID_SEARCH]);
		gtk_entry_set_text (GTK_ENTRY(data->ST_search), "");
		g_signal_handler_unblock(data->ST_search, data->handler_id[HID_SEARCH]);
	}	
}


static void
hub_ledger_selection(GtkTreeSelection *treeselection, gpointer user_data)
{

	DB( g_print("\n[hub-ledger] selection changed cb\n") );


	hub_ledger_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(FLG_REG_SENSITIVE));

}


static void
hub_ledger_listview_populate(GtkWidget *widget)
{
struct hub_ledger_data *data;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean hastext;
gchar *needle;
gint sort_column_id;
GtkSortType order;
guint i, qs_flag;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[hub-ledger] listview_populate - kacc=%d\n", data->acc != NULL ? (gint)data->acc->key : -1) );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));


	//block handler here
	g_signal_handlers_block_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope))), G_CALLBACK (hub_ledger_selection), NULL);

	gtk_tree_store_clear (GTK_TREE_STORE(model));

	// ref model to keep it
	DB( g_print(" unplug model\n") );
	g_object_ref(model);
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_ope), NULL);

	// perf: if you leave the sort, insert is damned slow
	gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), &sort_column_id, &order);
	
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, PREFS->lst_ope_sort_order);

	hastext = (gtk_entry_get_text_length (GTK_ENTRY(data->ST_search)) >= 2) ? TRUE : FALSE;
	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));

	//build the mask flag for quick search
	qs_flag = 0;
	if(hastext)
	{
		qs_flag = list_txn_get_quicksearch_column_mask(GTK_TREE_VIEW(data->LV_ope));
	}
	
	data->total = 0;
	data->totalsum = 0.0;

	for(i=0;i<data->gpatxn->len;i++)
	{
	Transaction *txn = g_ptr_array_index(data->gpatxn, i);
	gboolean insert = TRUE;
		
		if(hastext)
		{
			insert = filter_txn_search_match(needle, txn, qs_flag);
		}

		if(insert)
		{
			//gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	 		//gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			//5.7 optim: prepend and not append
	 		//gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, -1,
	 		gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, 0,
				MODEL_TXN_POINTER, txn,
				-1);

			if( data->showall == FALSE )
				data->totalsum += txn->amount;
			else
				data->totalsum += hb_amount_base (txn->amount, txn->kcur);

			data->total++;
		}
	}

	// push back the sort id
	DB( g_print(" sort model\n") );
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GTK_TREE_STORE(model)), sort_column_id, order);
	
	DB( g_print(" plug model\n") );
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_ope), model); /* Re-attach model to view */
	g_object_unref(model);

	g_signal_handlers_unblock_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope))), G_CALLBACK (hub_ledger_selection), NULL);

	/* update info range text */
	{
	gchar *daterange;
		
		daterange = filter_daterange_text_get(data->filter);
		gtk_widget_set_tooltip_markup(GTK_WIDGET(data->CY_range), daterange);
		//gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
		g_free(daterange);
	}

	DB( g_print(" call update\n") );
	hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));

}


static void
hub_ledger_cb_button_lifenergy(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;
gboolean lifnrgval;

	DB( g_print("\n[hub-ledger] toggle life energy\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	lifnrgval = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_lifnrg));
	if( lifnrgval == TRUE && GLOBALS->lifen_earnbyh <= 0.0 )
	{
		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
		_("Life Energy"),
		_("'Earn by hour' is not set into the current Wallet.")
		);
	}
	list_txn_set_life_energy(GTK_TREE_VIEW(data->LV_ope), lifnrgval);
}


static void
hub_ledger_info_cb_show_pending(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[hub-ledger] infobar show pending\n") );

	//TODO: change date to all date ?
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_status), FLT_STATUS_UNAPPROVED);
}


static void
hub_ledger_cb_filter_daterange(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;
gboolean future;
gint range;

	DB( g_print("\n[hub-ledger] filter_daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));
	future = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_future));

	data->filter->nbdaysfuture = 0;

	//in 5.6 no longer custom open the filter
	//if(range != FLT_RANGE_OTHER)
	//{
		filter_preset_daterange_set(data->filter, range, (data->showall == FALSE) ? data->acc->key : 0);
		// add eventual x days into future display
		if( future && (PREFS->date_future_nbdays > 0) )
			filter_preset_daterange_add_futuregap(data->filter, PREFS->date_future_nbdays);
		
		hub_ledger_collect_filtered_txn(data->LV_ope, FALSE);
		hub_ledger_listview_populate(data->LV_ope);
	/*}
	else
	{
		if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, data->showall, TRUE) != GTK_RESPONSE_REJECT)
		{
			hub_ledger_collect_filtered_txn(data->LV_ope, FALSE);
			hub_ledger_listview_populate(data->LV_ope);
			hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));
		}
	}*/
}


static void
hub_ledger_cb_refresh(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;

	DB( g_print("\n[hub-ledger] filterbar change\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//#2119051 add new txn needs to extend all date range
	if(data->filter->range == FLT_RANGE_MISC_ALLDATE)
	{
		hub_ledger_cb_filter_daterange(data->window, NULL);
	}

	hub_ledger_collect_filtered_txn(data->LV_ope, FALSE);
	hub_ledger_listview_populate(data->LV_ope);
}


static void
hub_ledger_cb_filterbar_change(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;
gint type, status;

	DB( g_print("\n[hub-ledger] filterbar change\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));
	status = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_status));

	filter_preset_type_set(data->filter, type, FLT_INCLUDE);
	filter_preset_status_set(data->filter, status);

	hub_ledger_collect_filtered_txn(data->LV_ope, FALSE);
	hub_ledger_listview_populate(data->LV_ope);
}


static void beta_hub_ledger_cb_preset_change(GtkWidget *widget, gpointer user_data);

static void
hub_ledger_cb_filter_reset(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;
gint dspstatus;
GtkWidget *combobox;

	DB( g_print("\n[hub-ledger] filter_reset\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	filter_reset(data->filter);
	
	//#1600356 grpflg
	g_signal_handlers_block_by_func(data->CY_flag, G_CALLBACK (beta_hub_ledger_cb_preset_change), NULL);
	kiv_combo_box_set_active(GTK_COMBO_BOX(data->CY_flag), GRPFLAG_ANY);
	g_signal_handlers_unblock_by_func(data->CY_flag, G_CALLBACK (beta_hub_ledger_cb_preset_change), NULL);
	

	filter_preset_daterange_set (data->filter, PREFS->date_range_txn, (data->showall == FALSE) ? data->acc->key : 0);

	if(PREFS->hidereconciled)
		filter_preset_status_set (data->filter, FLT_STATUS_UNRECONCILED);

	// add eventual x days into future display
	if( PREFS->date_future_nbdays > 0 )
		filter_preset_daterange_add_futuregap(data->filter, PREFS->date_future_nbdays);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_RANGE]);
	g_signal_handler_block(data->CY_type, data->handler_id[HID_TYPE]);
	g_signal_handler_block(data->CY_status, data->handler_id[HID_STATUS]);

	DB( g_print(" set range : %d\n", data->filter->range) );
	DB( g_print(" set type  : %d\n", data->filter->type) );
	DB( g_print(" set status: %d\n", data->filter->status) );

	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), data->filter->range);
	
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_type), data->filter->type);
	//#1873324 ledger status quick filter do not reset 
	//hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_status), data->filter->rawstatus);
	//#1878483 status with hidereconciled shows reconciled (due to filter !reconciled internal
	dspstatus = data->filter->status;
	if( (dspstatus == FLT_STATUS_RECONCILED) && (data->filter->option[FLT_GRP_STATUS] == 2) )
		dspstatus = FLT_STATUS_UNRECONCILED;
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_status), dspstatus);

	g_signal_handler_unblock(data->CY_status, data->handler_id[HID_STATUS]);
	g_signal_handler_unblock(data->CY_type, data->handler_id[HID_TYPE]);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_RANGE]);

	if( data->showall )
	{
		combobox = ui_flt_popover_hub_get_combobox(GTK_BOX(data->PO_hubfilter), NULL);
		g_signal_handlers_block_by_func (G_OBJECT (combobox), G_CALLBACK (beta_hub_ledger_cb_preset_change), NULL);
		gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);		
		g_signal_handlers_unblock_by_func (G_OBJECT (combobox), G_CALLBACK (beta_hub_ledger_cb_preset_change), NULL);
	}

	hub_ledger_collect_filtered_txn(data->LV_ope, TRUE);
	hub_ledger_listview_populate(data->LV_ope);

}


static void beta_hub_ledger_cb_preset_change(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;
Filter *newflt;


	DB( g_print("\n[repdist] filter preset change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( !data->showall )
		return;

	newflt = ui_flt_popover_hub_get(GTK_BOX(data->PO_hubfilter), NULL);
	if( newflt )
	{
		DB( g_print(" key:%d, copy filter\n", newflt->key) );
		da_flt_copy(newflt, data->filter);

		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), data->filter->range);
	}
	else
		hub_ledger_cb_filter_reset(widget, user_data);

	ui_flt_manage_header_sensitive(data->PO_hubfilter, NULL);

}


/* used to remove a intxfer child from a treeview */
static void
_list_txn_remove_by_value(GtkTreeView *treeview, Transaction *txn)
{
GtkTreeModel *model;
GtkTreeIter iter;
gboolean valid;

	if( txn == NULL )
		return;

	DB( g_print(" remove by value %p\n\n", txn) );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Transaction *tmp;

		gtk_tree_model_get (model, &iter,
			MODEL_TXN_POINTER, &tmp,
			-1);

		if( txn == tmp )
		{
			gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
			break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


// this func to some toggle
static void
_list_txn_status_selected_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
gint targetstatus = GPOINTER_TO_INT(userdata);
Transaction *txn;
gboolean saverecondate = FALSE;

	gtk_tree_model_get(model, iter, MODEL_TXN_POINTER, &txn, -1);

	account_balances_sub(txn);
	
	switch(targetstatus)
	{
		case TXN_STATUS_NONE:
			switch(txn->status)
			{
				case TXN_STATUS_CLEARED:
				case TXN_STATUS_RECONCILED:
					txn->status = TXN_STATUS_NONE;
					txn->dspflags |= FLAG_TMP_EDITED;
					break;
			}
			break;

		case TXN_STATUS_CLEARED:
			switch(txn->status)
			{
				case TXN_STATUS_NONE:
				case TXN_STATUS_RECONCILED:
					txn->status = TXN_STATUS_CLEARED;
					txn->dspflags |= FLAG_TMP_EDITED;
					break;
				case TXN_STATUS_CLEARED:
					txn->status = TXN_STATUS_NONE;
					txn->dspflags |= FLAG_TMP_EDITED;
					break;
			}
			break;
			
		case TXN_STATUS_RECONCILED:
			switch(txn->status)
			{
				case TXN_STATUS_NONE:
				case TXN_STATUS_CLEARED:
					txn->status = TXN_STATUS_RECONCILED;
					txn->dspflags |= FLAG_TMP_EDITED;
					saverecondate = TRUE;
					break;
				case TXN_STATUS_RECONCILED:
					txn->status = TXN_STATUS_CLEARED;
					txn->dspflags |= FLAG_TMP_EDITED;
					break;
			}
			break;

	}

	transaction_changed(txn, saverecondate);
	
	account_balances_add(txn);
	
	//#492755 removed 4.3 let the child transfer unchanged
	//#2019193 option the sync xfer status
	if( txn->flags & OF_INTXFER )
	{
		if( PREFS->xfer_syncstat == TRUE )
		{
		Transaction *child = transaction_xfer_child_strong_get(txn);

			if(child != NULL)
			{
			GtkWindow *accwin = homebank_app_find_window(txn->kxferacc);

				//#2080756 recompute bal
				account_balances_sub(child);
				child->status = txn->status;
				child->dspflags |= FLAG_TMP_EDITED;
				account_balances_add(child);

				//#2080756 if open refresh target account balances
				if(accwin != NULL)
				{
					DB( g_print(" xfer call refresh %d\n", txn->kxferacc));
					hub_ledger_update(GTK_WIDGET(accwin), GINT_TO_POINTER(FLG_REG_BALANCE));
				}
			}
		}
	}
}


static void
hub_ledger_remove_single_transaction(GtkWindow *window, Transaction *txn)
{
struct hub_ledger_data *data;

	if(txn == NULL)
		return;

	DB( g_print("\n[hub-ledger] remove single txn\n") );
	
	data = g_object_get_data(G_OBJECT(window), "inst_data");

	_list_txn_remove_by_value(GTK_TREE_VIEW(data->LV_ope), txn);
}


// future: refresh txn list of open ledger
void
beta_hub_ledger_refresh_txn_opens(void)
{
	DB( g_print("\n[hub-ledger] refresh txn list of opens\n") );

	GList *l = gtk_application_get_windows(GLOBALS->application);
	while (l != NULL)
	{
	GtkWindow *tmpwin = l->data;
	gint key = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(tmpwin), "key"));
		
		//TODO: when multiple wallet, we should check as well for that
		if( key == -1 || key > 0 )
		{
		gboolean refresh = FALSE;

			DB( g_print(" window: %p: key=%d '%s'\n", tmpwin, key, gtk_window_get_title(tmpwin)) );
			//showall always refresh
			if( key == -1 )
				refresh = TRUE;
			else
			{
			Account *acc = da_acc_get(key);

				if( acc && (acc->dspflags & FLAG_ACC_TMP_DIRTY) )
				{
					refresh = TRUE;
					account_set_dirty(acc, 0, FALSE);
				}
			}

			if( refresh )
			{
				DB( g_print(" >refresh\n") );
				hub_ledger_cb_refresh(GTK_WIDGET(tmpwin), NULL);
			}
		}
		l = g_list_next(l);
	}
}


static void
hub_ledger_add_after_propagate(struct hub_ledger_data *data, Transaction *add_txn)
{

	DB( g_print("\n[hub-ledger] add after propagate\n") );

	if((data->showall == TRUE) || ( (data->acc != NULL) && (add_txn->kacc == data->acc->key) ) )
	{
		account_set_dirty(data->acc, 0, TRUE);

		if( (add_txn->flags & OF_INTXFER) )
			account_set_dirty(NULL, add_txn->kxferacc, TRUE);

		beta_hub_ledger_refresh_txn_opens();
	}
}


static void
hub_ledger_cb_editfilter(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, data->showall, TRUE) != GTK_RESPONSE_REJECT)
	{
		hub_ledger_collect_filtered_txn(data->LV_ope, TRUE);
		hub_ledger_listview_populate(data->LV_ope);
		hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));

		g_signal_handler_block(data->CY_range, data->handler_id[HID_RANGE]);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
		g_signal_handler_unblock(data->CY_range, data->handler_id[HID_RANGE]);
		
		//ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
		if(data->showall)
			ui_flt_manage_header_sensitive(data->PO_hubfilter, NULL);
	}
}


static void
hub_ledger_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;

	DB( g_print("\n[hub-ledger] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_BALANCE));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_ope));
}


static gboolean
hub_ledger_cb_on_key_press(GtkWidget *source, GdkEvent *event, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GdkModifierType state;
guint keyval;

	gdk_event_get_state (event, &state);
	gdk_event_get_keyval(event, &keyval);

	// On Control-f enable search entry
	//already bind to menu Txn > Find ctrl+F
	/*if (state & GDK_CONTROL_MASK && keyval == GDK_KEY_f)
	{
		gtk_widget_grab_focus(data->ST_search);
	}
	else*/
	if (keyval == GDK_KEY_Escape && gtk_widget_has_focus(data->ST_search))
	{
		hbtk_entry_set_text(GTK_ENTRY(data->ST_search), NULL);
		gtk_widget_grab_focus(data->LV_ope);
		return TRUE;
	}

	return GDK_EVENT_PROPAGATE;
}


static void
hub_ledger_onRowActivated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
struct hub_ledger_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
gint col_id, count;
Transaction *ope;
gboolean lockrecon;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	//5.3.1 if closed account : disable any change
	if( data->closed == TRUE )
		return;

	col_id = gtk_tree_view_column_get_sort_column_id (col);
	count  = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(treeview));
	model  = gtk_tree_view_get_model(treeview);

	//get transaction double clicked to initiate the widget
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

	DB( g_print (" %d rows been double-clicked on column=%d! ope=%s\n", count, col_id, ope->memo) );

	if( count == 1)
	{
		//1909749 lock/unlock reconciled	
		lockrecon = FALSE;
		if( (ope->status == TXN_STATUS_RECONCILED) && (gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled)) == TRUE) )
			lockrecon = TRUE;

		if( lockrecon == FALSE )
		{
		GAction *action = g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnedit");

			//hub_ledger_action(GTK_WIDGET(treeview), GINT_TO_POINTER(ACTION_ACCOUNT_EDIT));
			g_action_activate(action, NULL);
		}
	}
	else
	{
		if( data->showall == FALSE )
		{
			if(col_id >= LST_DSPOPE_DATE && col_id != LST_DSPOPE_BALANCE)
			{
				hub_ledger_edit_multiple (data->window, ope, col_id, data);
			}
		}
	}
}


/*
** populate the account window
*/
void
hub_ledger_window_init(GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[hub-ledger] init window\n") );

	if( data->showall == TRUE )
	{
		//gtk_label_set_text (GTK_LABEL(data->LB_name), _("All transactions"));
		//hb_widget_visible (data->IM_closed, FALSE);
	}
	else
	{
		//gtk_label_set_text (GTK_LABEL(data->LB_name), data->acc->name);
		//hb_widget_visible (data->IM_closed, (data->acc->flags & AF_CLOSED) ? TRUE : FALSE);

		DB( g_print(" sort transactions\n") );
		da_transaction_queue_sort(data->acc->txn_queue);
	}

	list_txn_set_column_acc_visible(GTK_TREE_VIEW(data->LV_ope), data->showall);

	if( (data->showall == FALSE) && !(data->acc->flags & AF_NOBUDGET) )
		list_txn_set_warn_nocategory(GTK_TREE_VIEW(data->LV_ope), TRUE);

	//DB( g_print(" mindate=%d, maxdate=%d %x\n", data->filter->mindate,data->filter->maxdate) );

	DB( g_print(" set range or populate+update sensitive+balance\n") );
	
	hub_ledger_cb_filter_reset(widget, user_data);

	DB( g_print(" call update visual\n") );
	hub_ledger_update(widget, GINT_TO_POINTER(FLG_REG_VISUAL|FLG_REG_SENSITIVE));

}


static gboolean
hub_ledger_cb_search_focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct hub_ledger_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[hub-ledger] search focus-in event\n") );

	
	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));

	return FALSE;
}


static void
quick_search_text_changed_cb (GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	hub_ledger_listview_populate (data->window);
}


static gint
listview_context_cb (GtkWidget *widget, GdkEvent *event, GtkWidget *menu)
{
struct hub_ledger_data *data;
GdkEventType type;
guint button = 0;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//#1993088 ledger closed account popmenu should be disabled
	if( data->closed == TRUE )
		goto end;

	type = gdk_event_get_event_type(event);
	gdk_event_get_button(event, &button);

	if (type == GDK_BUTTON_PRESS && button == 3)
	{
		// check we are not in the header but in bin window
		if (gdk_event_get_window(event) == gtk_tree_view_get_bin_window (GTK_TREE_VIEW (widget)))
		{

			//test to enable
			//GAction *action;
			//action = g_action_map_lookup_action (G_ACTION_MAP (data->actions), "txnadd");

			//g_simple_action_set_enabled(G_SIMPLE_ACTION(action), TRUE);
			//g_action_activate(action, NULL);

	
			#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION >= 22) )
				gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent *)event);
			#else
				gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL, button, gdk_event_get_time(event));
			#endif
				// On indique à l'appelant que l'on a géré cet événement.

			return TRUE;
		}

		// On indique à l'appelant que l'on n'a pas géré cet événement.
	}
end:
	return FALSE;
}


/* account action functions -------------------- */


static void
hub_ledger_action_txnaddherit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkWidget *dialog;
gint result, kacc, type = 0;
guint changes = GLOBALS->changes_count;

	homebank_app_date_get_julian();

	if( !strcmp(g_action_get_name (G_ACTION (action)), "txnadd") )
	{
		DB( g_print(" (transaction) add multiple\n") );
		data->cur_ope = da_transaction_malloc();
		// miss from 5.2.8 ??
		//da_transaction_set_default_template(src_txn);
		type = TXN_DLG_ACTION_ADD;
		result = HB_RESPONSE_ADD;
	}
	else
	{
		DB( g_print(" (transaction) inherit multiple\n") );
		data->cur_ope = da_transaction_clone(list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope)));
		//#1873311 inherit+kepplastdate=OFF = today
		if( PREFS->heritdate == FALSE ) 
			data->cur_ope->date = GLOBALS->today;

		//#2083127 don't keep grpflags
		data->cur_ope->grpflg = 0;

		//#1432204 inherit => status none
		data->cur_ope->status = TXN_STATUS_NONE;
		type = TXN_DLG_ACTION_INHERIT;
		result = HB_RESPONSE_ADDKEEP;
	}

	kacc = (data->acc != NULL) ? data->acc->key : 0;
	dialog = create_deftransaction_window(GTK_WINDOW(data->window), type, TXN_DLG_TYPE_TXN, kacc );
	while(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP)
	{
		if( result == HB_RESPONSE_ADD )
		{
			da_transaction_init(data->cur_ope, kacc);
		}

		deftransaction_set_transaction(dialog, data->cur_ope);

		result = gtk_dialog_run (GTK_DIALOG (dialog));

		DB( g_print(" dialog result is %d\n", result) );

		if(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP || result == GTK_RESPONSE_ACCEPT)
		{
		Transaction *add_txn;
		
			deftransaction_get(dialog, NULL);

			add_txn = transaction_add(GTK_WINDOW(dialog), TRUE, data->cur_ope);
			//2044601 if NULL xfer may have beed aborted
			if( add_txn != NULL )
			{

				//#1831975
				if(PREFS->txn_showconfirm)
					deftransaction_external_confirm(dialog, add_txn);

				DB( g_print(" added 1 transaction to %d\n", add_txn->kacc) );

				hub_ledger_add_after_propagate(data, add_txn);

				hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_BALANCE));
				//#1667201 already done into add
				//data->acc->dspflags |= FLAG_ACC_TMP_ADDED;
				GLOBALS->changes_count++;
			}
			else
			{
				//2044601 keep actual txn
				result = HB_RESPONSE_ADDKEEP;
			}

		}
	}

	da_transaction_free (data->cur_ope);

	deftransaction_dispose(dialog, NULL);
	gtk_window_destroy (GTK_WINDOW(dialog));

	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}


static void
hub_ledger_action_txnedit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
Transaction *active_txn;
guint changes = GLOBALS->changes_count;
gint result;

	DB( g_print(" edit\n") );
		
	active_txn = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));
			
	if(active_txn)
	{
	Transaction *old_txn, *new_txn;
	guint32 oldkxferacc;

		old_txn = da_transaction_clone (active_txn);
		new_txn = active_txn;
		//5.8 if xfer break, this will be lost
		oldkxferacc = old_txn->kxferacc;
		
		result = deftransaction_external_edit(GTK_WINDOW(data->window), old_txn, new_txn);

		if(result == GTK_RESPONSE_ACCEPT)
		{
			DB( g_print(" edit accept\n") );
			//manage current window display stuff
			
			DB( g_print(" date changed: %d\n", old_txn->date != new_txn->date ? 1 : 0 ) );
			DB( g_print(" type changed: %d\n", transaction_get_type(old_txn) != transaction_get_type(new_txn) ? 1 : 0 ) );

			/*what to evaluate here
			1) txn remain in same account
				date chnaged > sort
			2) txn move
				a) was not a xfer and become
				b) was a xfer and removed
				c) normal txn & account changed
			*/

			//#1270687: sort if date changed
			//if(old_txn->date != new_txn->date)
			//	data->do_sort = TRUE;
			//#1931816: sort is already done in deftransaction_external_edit
			// but still to be done if showall
			if(data->showall == FALSE)
			{
			GtkTreeModel *model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
				DB( g_print(" >sort force\n") );
				list_txn_sort_force(GTK_TREE_SORTABLE(model), NULL);
			}

			//dirty and refresh open ledger

			// txn changed of account
			//TODO: maybe this should move to deftransaction_external_edit
			if( data->acc != NULL && (new_txn->kacc != data->acc->key) )
			{
				DB( g_print(" >account change\n") );
				account_set_dirty(data->acc, 0, TRUE);
				account_set_dirty(NULL, new_txn->kacc, TRUE);
			}

			//#1812470 txn is xfer update target account window if open
			if( (old_txn->flags & OF_INTXFER) && (old_txn->amount != new_txn->amount) )
			{
			GtkWindow *accwin;

				DB( g_print(" >xfer amt change\n") );
				accwin = homebank_app_find_window(new_txn->kxferacc);
				if(accwin)
				{
					DB( g_print(" update xfer dst win\n") );
					hub_ledger_update(GTK_WIDGET(accwin), GINT_TO_POINTER(FLG_REG_BALANCE));
				}
			}

			//5.7 txn was xfer but is not : refresh list
			if( ((old_txn->flags & OF_INTXFER) > 0) && ((new_txn->flags & OF_INTXFER)==0) )
			{
				DB( g_print("\n >break xfer - %d > %d\n", old_txn->kacc, oldkxferacc) );
				account_set_dirty(NULL, old_txn->kacc, TRUE);
				account_set_dirty(NULL, new_txn->kacc, TRUE);
			}
		
			beta_hub_ledger_refresh_txn_opens();

			//todo: probably move this to refresh as well
			hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_SENSITIVE|FLG_REG_BALANCE));

			//TODO: saverecondate is handled in external edit already
			transaction_changed(new_txn, FALSE);
			
			//#2065625
			GLOBALS->changes_count++;
		}

		da_transaction_free (old_txn);
	}

	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void
hub_ledger_action_txnmedit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	hub_ledger_edit_multiple(data->window, NULL, 0, user_data);
}


static void
_helper_pending_sub(struct hub_ledger_data *data, Transaction *txn)
{
Account *acc = da_acc_get(txn->kacc);

	txn->flags &= ~(OF_ISIMPORT|OF_ISPAST);

	g_return_if_fail(acc != NULL);

	if( acc->nb_pending > 0 )
		acc->nb_pending--;
	if( data->nb_pending > 0 )
		data->nb_pending--;

	account_flags_eval(acc);
}


static void
hub_ledger_action_txnapprove(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
guint changes = GLOBALS->changes_count;
//gint count;

	DB( g_print(" approve\n") );

	//#2042692
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);
	//count = g_list_length(selection);

	list = g_list_last(selection);
	while(list != NULL)
	{
	Transaction *txn;
	GtkTreeIter iter;

		if( gtk_tree_model_get_iter(model, &iter, list->data) == TRUE )
		{
			gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &txn, -1);
			if( txn->flags & (OF_ISIMPORT|OF_ISPAST))
			{
				DB( g_print(" approving txn '%s' %.2f\n", txn->memo, txn->amount) );
				account_balances_sub(txn);
				//#1875100
				_helper_pending_sub(data, txn);
				account_balances_add(txn);
				txn->dspflags |= FLAG_TMP_EDITED;
				GLOBALS->changes_count++;
			}
		}
		list = g_list_previous(list);
	}

	g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selection);


	hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_BALANCE));

	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void
hub_ledger_action_txndelete(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
gchar *title;
guint changes = GLOBALS->changes_count;
gint count, result;

	DB( g_print(" delete\n") );

	//#2042692
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);
	count = g_list_length(selection);

	title = g_strdup_printf (_("Are you sure you want to delete the %d selected transaction?"), count);

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->window),
			title,
			_("If you delete a transaction, it will be permanently lost."),
			_("_Delete"),
			TRUE
		);

	g_free(title);

	if(result == GTK_RESPONSE_OK)
	{
		//block selection change to avoid refresh and call to update
		g_signal_handlers_block_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope))), G_CALLBACK (hub_ledger_selection), NULL);
		
		list = g_list_last(selection);
		while(list != NULL)
		{
		Transaction *rem_txn;
		GtkTreeIter iter;

			//#1860232 crash here if no test when reach a txn already removed
			if( gtk_tree_model_get_iter(model, &iter, list->data) == TRUE )
			{
				gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &rem_txn, -1);

				if( !data->lockreconciled || (rem_txn->status != TXN_STATUS_RECONCILED) )
				{
					DB( g_print(" delete %s %.2f\n", rem_txn->memo, rem_txn->amount) );

					//#1716181 also remove from the ptr_array (quickfilter)
					g_ptr_array_remove(data->gpatxn, (gpointer)rem_txn);

					// 1) remove visible current and potential xfer
					gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);

					//manage target open window as well
					if((rem_txn->flags & OF_INTXFER))
					{
					Transaction *child = transaction_xfer_child_strong_get(rem_txn);
						if( child )
						{
							if( data->showall )
							{
								_list_txn_remove_by_value(GTK_TREE_VIEW(data->LV_ope), child);
								//#1716181 also remove from the ptr_array (quickfilter)				
								g_ptr_array_remove(data->gpatxn, (gpointer)child);
								data->total--;
								GLOBALS->changes_count++;
							}

							//5.8.4 if open remove target
							GtkWindow *accwin = homebank_app_find_window(rem_txn->kxferacc);
							if(accwin != NULL)
							{
								hub_ledger_remove_single_transaction(accwin, child);
								DB( g_print(" xfer call refresh %d\n", rem_txn->kxferacc));
								hub_ledger_update(GTK_WIDGET(accwin), GINT_TO_POINTER(FLG_REG_BALANCE));
							}
						}
					}

					// 2) manage pending
					if( rem_txn->flags & (OF_ISIMPORT|OF_ISPAST) )
					{
						_helper_pending_sub(data, rem_txn);
					}

					// 3) remove datamodel
					transaction_remove(rem_txn);
					data->total--;
					GLOBALS->changes_count++;
				}
			}
			list = g_list_previous(list);
		}

		g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(selection);

		g_signal_handlers_unblock_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope))), G_CALLBACK (hub_ledger_selection), NULL);
		
		hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_BALANCE));
	}

	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void
hub_ledger_action_grpflag(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
const gchar *name;
gint count, newflag = 0;
guint changes = GLOBALS->changes_count;
GtkTreeModel *model;
GList *selection, *list;

	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));

	if(count > 0 )
	{
		name = g_action_get_name (G_ACTION (action));
		if( strlen(name) < 3)
			return;

		switch(name[3])
		{
			case 'n': newflag=GRPFLAG_NONE; break;
			case '1': newflag=GRPFLAG_RED; break;
			case '2': newflag=GRPFLAG_ORANGE; break;
			case '3': newflag=GRPFLAG_YELLOW; break;
			case '4': newflag=GRPFLAG_GREEN; break;
			case '5': newflag=GRPFLAG_BLUE; break;
			case '6': newflag=GRPFLAG_PURPLE; break;
		}

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
		selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

		list = g_list_first(selection);
		while(list != NULL)
		{
		Transaction *ope;
		GtkTreeIter iter;

			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);
			if( !data->lockreconciled || (ope->status != TXN_STATUS_RECONCILED) )
			{
				DB( g_print(" change flag %d > %d\n", ope->grpflg, newflag) );
				if( ope->grpflg != newflag )
					GLOBALS->changes_count++;

				ope->grpflg = newflag;
			}
			list = g_list_next(list);
		}

		g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(selection);

		//refresh main
		if( GLOBALS->changes_count > changes )
			gtk_widget_queue_draw (data->LV_ope);
	}
}



static void
hub_ledger_action_status_none(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeSelection *selection;
gint count, result;
guint changes = GLOBALS->changes_count;
	
	_list_txn_selection_count_type(GTK_TREE_VIEW(data->LV_ope), &count, NULL);

	if(count > 0 )
	{
	
	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->window),
			_("Are you sure you want to change the status to None?"),
			_("Some transaction in your selection are already Reconciled."),
			_("_Change"),
			FALSE
		);
	}
	else
		result = GTK_RESPONSE_OK;
		
	if( result == GTK_RESPONSE_OK )
	{
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
		gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)_list_txn_status_selected_foreach_func, 
			GINT_TO_POINTER(TXN_STATUS_NONE));

		DB( g_print(" none\n") );
		DB( g_print(" redraw LV_ope\n") );
		gtk_widget_queue_draw (data->LV_ope);
		//gtk_widget_queue_resize (data->LV_acc);

		hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_BALANCE));

		GLOBALS->changes_count++;
	}
	
	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void
hub_ledger_action_status_clear (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeSelection *selection;
gint count, result;
guint changes = GLOBALS->changes_count;

	_list_txn_selection_count_type(GTK_TREE_VIEW(data->LV_ope), &count, NULL);

	if(count > 0 )
	{
	
	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->window),
			_("Are you sure you want to change the status to Cleared?"),
			_("Some transaction in your selection are already Reconciled."),
			_("_Change"),
			FALSE
		);
	}
	else
		result = GTK_RESPONSE_OK;

	if( result == GTK_RESPONSE_OK )
	{		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
		gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)_list_txn_status_selected_foreach_func, 
			GINT_TO_POINTER(TXN_STATUS_CLEARED));

		DB( g_print(" clear\n") );

		DB( g_print(" redraw LV_ope\n") );
		gtk_widget_queue_draw (data->LV_ope);
		//gtk_widget_queue_resize (data->LV_acc);

		hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_BALANCE));

		GLOBALS->changes_count++;
	}
	
	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void
hub_ledger_action_status_reconcile (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeSelection *selection;
gint count, result;
guint changes = GLOBALS->changes_count;
	
	_list_txn_selection_count_type(GTK_TREE_VIEW(data->LV_ope), &count, NULL);

	if(count > 0 )
	{
	
	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->window),
			_("Are you sure you want to toggle the status Reconciled?"),
			_("Some transaction in your selection are already Reconciled."),
			_("_Toggle"),
			FALSE
		);
	}
	else
		result = GTK_RESPONSE_OK;
		
	if( result == GTK_RESPONSE_OK )
	{
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
		gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)_list_txn_status_selected_foreach_func, 
			GINT_TO_POINTER(TXN_STATUS_RECONCILED));

		DB( g_print(" reconcile\n") );

		DB( g_print(" redraw LV_ope\n") );
		gtk_widget_queue_draw (data->LV_ope);
		//gtk_widget_queue_resize (data->LV_acc);

		hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_BALANCE));

		GLOBALS->changes_count++;
	}

	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void
hub_ledger_action_find (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	gtk_widget_grab_focus(data->ST_search);
}


static void
hub_ledger_action_createtemplate (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
gchar *title;
gint result, count;

	DB( g_print("\n[hub-ledger] make archive\n") );

	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));

	if( count > 0 )
	{
		title = g_strdup_printf (_("Are you sure you want to create template from the %d selected transaction?"), count);

		result = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->window),
				title,
				NULL,
				_("_Create"),
				FALSE
			);
		g_free(title);
	/*
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("%d archives will be created"),
			GLOBALS->changes_count
			);
	*/

		if(result == GTK_RESPONSE_OK)
		{
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
			selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

			list = g_list_first(selection);
			while(list != NULL)
			{
			Archive *item;
			Transaction *ope;
			GtkTreeIter iter;

				gtk_tree_model_get_iter(model, &iter, list->data);
				gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

				DB( g_print(" create archive %s %.2f\n", ope->memo, ope->amount) );

				item = da_archive_malloc();

				da_archive_init_from_transaction(item, ope, TRUE);

				//GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);
				da_archive_append_new(item);
				GLOBALS->changes_count++;

				list = g_list_next(list);
			}

			g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
			g_list_free(selection);

			//#2000809 add confirmation
			ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
				_("Create Template"),
				_("%d created with a prefilled icon"),
				count
				);

		}
	}
}


static void
hub_ledger_action_createassignment (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
gchar *title;
gint result, count;

	DB( g_print("\n[hub-ledger] make assignment\n") );

	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));
	if( count > 0 )
	{
		title = g_strdup_printf (_("Are you sure you want to create assignment from the %d selected transaction?"), count);
		result = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->window),
				title,
				NULL,
				_("_Create"),
				FALSE
			);

		g_free(title);
		if(result == GTK_RESPONSE_OK)
		{
			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
			selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

			list = g_list_first(selection);
			while(list != NULL)
			{
			Assign *item;
			Transaction *ope;
			GtkTreeIter iter;

				gtk_tree_model_get_iter(model, &iter, list->data);
				gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

				DB( g_print(" create assignment %s %.2f\n", ope->memo, ope->amount) );

				item = da_asg_malloc();
				da_asg_init_from_transaction(item, ope);
				
				//5.7.1 free if fail
				if( da_asg_append(item) == TRUE )
				{
					GLOBALS->changes_count++;
				}
				else
				{
					da_asg_free(item);
					count--;
				}
				list = g_list_next(list);
			}

			g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
			g_list_free(selection);
			
			//#2000809 add confirmation
			ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
				_("Create Assignment"),
				_("%d created with a prefilled icon"),
				count
				);
		}
	}
}


static void
hub_ledger_action_exportcsv (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
gchar *name, *filepath;
GString *node;
GIOChannel *io;
gboolean hassplit, hasstatus;
guint flags;

	DB( g_print("\n[hub-ledger] export csv\n") );

	name = g_strdup_printf("%s.csv", (data->showall == TRUE) ? GLOBALS->owner :data->acc->name);

	filepath = g_build_filename(PREFS->path_export, name, NULL);

	if( ui_dialog_export_csv(GTK_WINDOW(data->window), &filepath, &hassplit, &hasstatus, data->showall) == GTK_RESPONSE_ACCEPT )
	{
		DB( g_printf(" filename is '%s'\n", filepath) );

		io = g_io_channel_new_file(filepath, "w", NULL);
		if(io != NULL)
		{
		flags = LST_TXN_EXP_PMT | LST_TXN_EXP_CAT | LST_TXN_EXP_TAG;

			//#2037468
			if( data->showall )
				flags |= LST_TXN_EXP_ACC;

			if( hasstatus )
				flags |= LST_TXN_EXP_CLR;

			node = list_txn_to_string(GTK_TREE_VIEW(data->LV_ope), FALSE, hassplit, FALSE, flags);

			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
	}

	g_free(filepath);
	g_free(name);
}


static void
hub_ledger_action_print (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
gchar *title, *name, *filepath;
GString *node;
guint flags;
gboolean statement = FALSE;

	title = (data->showall == FALSE) ? data->acc->name : _("All transactions");
	name  = g_strdup_printf("hb-%s.pdf", title);
	filepath = g_build_filename(PREFS->path_export, name, NULL);
	g_free(name);
	
	gint8 leftcols[8] = { 0, 1, 2, 3, -1 };

	flags = LST_TXN_EXP_CLR;
	if( data->showall )
	{
		flags |= LST_TXN_EXP_ACC;
		//{ 0, 1, 2, 3, 4, -1 };
		leftcols[4] = 4;
		leftcols[5] = -1;
	}
	else
	{
		flags |= LST_TXN_EXP_BAL;
		//{ 0, 1, 2, 3, -1 };
		//2044850 revert fitwidth for statement
		statement = TRUE;
	}
	
	node = list_txn_to_string(GTK_TREE_VIEW(data->LV_ope), TRUE, FALSE, FALSE, flags);
	
	hb_print_listview(GTK_WINDOW(data->window), node->str, leftcols, title, filepath, statement);

	g_string_free(node, TRUE);
	g_free(filepath);

	/* old < 5.7
		if( ui_dialog_export_pdf(GTK_WINDOW(data->window), &filepath) == GTK_RESPONSE_ACCEPT )
		{
			DB( g_printf(" filename is'%s'\n", filepath) );
			hb_export_pdf_listview(GTK_TREE_VIEW(data->LV_ope), filepath, data->acc->name);
		}
	*/
}


static void
hub_ledger_action_check_catsign_mark (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	// noaction if show all/closed account
	if(data->showall || data->closed)
		return;

	DB( g_print("\n[hub-ledger] check category sign\n\n") );

	data->chkcatsign = transaction_check_chkcatsign_mark (data->acc);
	if( data->chkcatsign > 0 )
	{
	gchar *text = g_strdup_printf(_("%d category sign don't match"), data->chkcatsign);
		gtk_label_set_text(GTK_LABEL(data->LB_chkcatsign), text);
		g_free(text);
	}
	else
		gtk_label_set_text(GTK_LABEL(data->LB_chkcatsign), _("No category sign don't match were found !"));

	gtk_widget_show(data->IB_chkcatsign);
	//#GTK+710888: hack waiting a fix
	gtk_widget_queue_resize (data->IB_chkcatsign);

	DB( g_print(" redraw LV_ope\n") );
	gtk_widget_queue_draw (data->LV_ope);

}


static void
hub_ledger_action_check_catsign_unmark(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	// noaction if show all/closed account
	if(data->showall || data->closed)
		return;

	DB( g_print("\n[hub-ledger] uncheck category sign\n\n") );

	if(data->showall == FALSE)
	{   
		data->chkcatsign = 0;
		gtk_widget_hide(data->IB_chkcatsign);
		transaction_check_chkcatsign_unmark(data->acc);
		DB( g_print(" redraw LV_ope\n") );
		gtk_widget_queue_draw (data->LV_ope);
	}
}


static void
hub_ledger_cb_bar_chkcatsign_response(GtkWidget *info_bar, gint response_id, gpointer user_data)
{
struct hub_ledger_data *data;

	DB( g_print("\n[hub-ledger] bar_chlcatsign_response\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(info_bar, GTK_TYPE_WINDOW)), "inst_data");

	switch( response_id )
	{
		case GTK_RESPONSE_CLOSE:
			hub_ledger_action_check_catsign_unmark(NULL, NULL, data);
			gtk_widget_hide (GTK_WIDGET (info_bar));	
			break;
	}
}



static void
hub_ledger_action_duplicate_mark (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	// noaction if show all/closed account
	if(data->showall || data->closed)
		return;

	DB( g_print("\n[hub-ledger] check duplicate\n\n") );

	// open dialog to select date tolerance in days
	//  with info message
	//  with check/fix button and progress bar
	// parse listview txn, clear/mark duplicate
	// apply filter

	if(data->showall == FALSE)
	{
	gint daygap;

		daygap = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_txn_daygap));
		data->similar = transaction_similar_mark (data->acc, daygap);
		if( data->similar > 0 )
		{
		gchar *text = g_strdup_printf(_("There is %d group of similar transactions"), data->similar);
			gtk_label_set_text(GTK_LABEL(data->LB_duplicate), text);
			g_free(text);
		}
		else
			gtk_label_set_text(GTK_LABEL(data->LB_duplicate), _("No similar transaction were found !"));

		gtk_widget_show(data->IB_duplicate);
		//#GTK+710888: hack waiting a fix
		gtk_widget_queue_resize (data->IB_duplicate);

		DB( g_print(" redraw LV_ope\n") );
		gtk_widget_queue_draw (data->LV_ope);
	}


}


static void
hub_ledger_action_duplicate_unmark(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	// noaction if show all/closed account
	if(data->showall || data->closed)
		return;

	DB( g_print("\n[hub-ledger] uncheck duplicate\n\n") );

	if(data->showall == FALSE)
	{   
		data->similar = 0;
		gtk_widget_hide(data->IB_duplicate);
		transaction_similar_unmark(data->acc);
		DB( g_print(" redraw LV_ope\n") );
		gtk_widget_queue_draw (data->LV_ope);
	}
}


static void
hub_ledger_cb_bar_duplicate_response(GtkWidget *info_bar, gint response_id, gpointer user_data)
{
struct hub_ledger_data *data;

	DB( g_print("\n[hub-ledger] bar_duplicate_response\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(info_bar, GTK_TYPE_WINDOW)), "inst_data");

	switch( response_id )
	{
		case HB_RESPONSE_REFRESH:
			hub_ledger_action_duplicate_mark(NULL, NULL, data);
			break;
		case GTK_RESPONSE_CLOSE:
			hub_ledger_action_duplicate_unmark(NULL, NULL, data);
			gtk_widget_hide (GTK_WIDGET (info_bar));	
			break;
	}
}


static void
hub_ledger_action_check_internal_xfer(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeModel *model;
GtkTreeIter iter;
GList *badxferlist;
gboolean valid, lockrecon;
gint count;

	// noaction if closed account
	if(data->closed)
		return;

	DB( g_print("\n[hub-ledger] check intenal xfer\n") );

	lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));

	badxferlist = NULL;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Transaction *txn;

		gtk_tree_model_get (model, &iter,
			MODEL_TXN_POINTER, &txn,
			-1);

		//#1909749 skip reconciled if lock is ON
		if( lockrecon && txn->status == TXN_STATUS_RECONCILED )
			goto next;

		if( txn->flags & OF_INTXFER )
		{
			if( transaction_xfer_child_strong_get(txn) == NULL )
			{
				DB( g_print(" invalid xfer: '%s'\n", txn->memo) );
				
				//test unrecoverable (kxferacc = 0)
				if( txn->kxferacc <= 0 )
				{
					DB( g_print(" unrecoverable, revert to normal xfer\n") );
					txn->dspflags |= FLAG_TMP_EDITED;
					txn->paymode = PAYMODE_XFER;
					txn->kxfer = 0;
					txn->kxferacc = 0;
				}
				else
				{
					//perf must use preprend, see glib doc
					badxferlist = g_list_prepend(badxferlist, txn);
				}
			}
		}
next:
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	count = g_list_length (badxferlist);
	DB( g_print(" found %d invalid int xfer\n", count) );

	if(count <= 0)
	{
		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
		_("Check internal transfer result"),
		_("No inconsistency found !")
		);
	}
	else
	{
	gboolean do_fix;
	
		do_fix = ui_dialog_msg_question(
			GTK_WINDOW(data->window),
			_("Check internal transfer result"),
			_("Inconsistency were found: %d\n"
			  "do you want to review and fix?"),
			count
			);

		if(do_fix == GTK_RESPONSE_YES)
		{	
		GList *tmplist = g_list_first(badxferlist);

			while (tmplist != NULL)
			{
			Transaction *stxn = tmplist->data;

				//future (open dialog to select date tolerance in days)
				transaction_xfer_search_or_add_child(GTK_WINDOW(data->window), FALSE, stxn, 0);

				tmplist = g_list_next(tmplist);
			}	
		}
	}

	g_list_free (badxferlist);

}


//#1983995 copy raw amount
static void
hub_ledger_action_copyrawamount(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
Transaction *ope;

	ope = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));
	if(ope != NULL)
	{
	Currency *cur = da_cur_get(ope->kcur);

		if( cur != NULL )
		{
		GtkClipboard *clipboard = gtk_clipboard_get_default(gdk_display_get_default());
		gdouble monval = hb_amount_round(ABS(ope->amount), cur->frac_digits);
		gchar *text;

			text = g_strdup_printf ("%.*f", cur->frac_digits, monval);
			DB( g_print(" raw amount is '%s'\n", text) );
			gtk_clipboard_set_text(clipboard, text, strlen(text));
			g_free(text);
		}
	}
}


static void
hub_ledger_action_viewsplit(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
Transaction *ope;

	ope = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));
	if(ope != NULL)
		ui_split_view_dialog(data->window, ope);
}


static void
hub_ledger_action_exportqif(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
gchar *filename;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[hub-ledger] export QIF\n") );

	if( ui_file_chooser_qif(GTK_WINDOW(data->window), &filename) == TRUE )
	{
		hb_export_qif_account_single(filename, data->acc);
		g_free( filename );
	}
}


static void
hub_ledger_action_converttoeuro(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
gchar *msg;
gint result;

	// noaction if show all/closed account
	if(data->showall || data->closed)
		return;

	DB( g_print("\n[hub-ledger] convert euro\n") );

	msg = g_strdup_printf(_("Every transaction amount will be divided by %.6f."), PREFS->euro_value);

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->window),
			_("Are you sure you want to convert this account to Euro as Major currency?"),
			msg,
			_("_Convert"),
			FALSE
		);

	g_free(msg);

	if(result == GTK_RESPONSE_OK)
	{
		account_convert_euro(data->acc);
		hub_ledger_update(data->LV_ope, GINT_TO_POINTER(FLG_REG_BALANCE));
	}
}

static void
hub_ledger_action_edit_copy_clipboard(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkClipboard *clipboard;
GString *node;
guint flags;

	DB( g_print("\n[hub-ledger] copy clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	flags = LST_TXN_EXP_CLR | LST_TXN_EXP_PMT | LST_TXN_EXP_CAT | LST_TXN_EXP_TAG;
	if(data->showall == TRUE)
		flags |= LST_TXN_EXP_ACC;
	node = list_txn_to_string(GTK_TREE_VIEW(data->LV_ope), TRUE, FALSE, TRUE, flags);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


//#1818052 wish: copy/paste one/multiple transaction(s)
static void
hub_ledger_action_edit_copy(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
gint count;

	// noaction if closed account
	if(data->closed)
		return;

	DB( g_print("\n[hub-ledger] copy\n") );
	
	//struct hub_ledger_data *data2;
	//data2 = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(menuitem), GTK_TYPE_WINDOW)), "inst_data");
	//DB( g_print("%p = %p\n", data, data2) );

	g_queue_free_full(data->q_txn_clip, (GDestroyNotify)da_transaction_free);
	data->q_txn_clip = g_queue_new();
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

	count = 0;
	list = g_list_first(selection);
	while(list != NULL)
	{
	Transaction *ope, *newope;
	GtkTreeIter iter;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, MODEL_TXN_POINTER, &ope, -1);

		newope = da_transaction_clone(ope);
		DB( g_print(" copy txn %p - '%.2f' '%s'\n", newope, newope->amount, newope->memo) );
		
		g_queue_push_tail(data->q_txn_clip, newope);
		count++;
		list = g_list_next(list);
	}

	g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selection);

	if(count > 0 )
		hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_SENSITIVE));
	
}


static void
hub_ledger_action_edit_paste(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
GList *list;
gboolean istoday = FALSE;

	// noaction if closed account
	if(data->closed)
		return;

	DB( g_print("\n[hub-ledger] paste normal\n") );

	istoday = ( !strcmp(g_action_get_name (G_ACTION (action)), "txnpastet")) ? TRUE : FALSE;

	DB( g_print(" paste %d - as today=%d\n", (gint)g_queue_get_length(data->q_txn_clip), istoday) );
	
	list = g_queue_peek_head_link(data->q_txn_clip);
	while (list != NULL)
	{
	Transaction *item = list->data;
	Transaction *add_txn;
		
		DB( g_print(" paste txn %p - '%.2f' '%s'\n", item, item->amount, item->memo) );

		if( istoday == TRUE )
			item->date = GLOBALS->today;

		add_txn = transaction_add(GTK_WINDOW(data->window), FALSE, item);
		add_txn->dspflags |= FLAG_TMP_ADDED;

		hub_ledger_add_after_propagate(data, add_txn);

		//#2068634
		GLOBALS->changes_count++;
		
		list = g_list_next(list);
	}

	hub_ledger_update(data->window, GINT_TO_POINTER(FLG_REG_SENSITIVE+FLG_REG_BALANCE));
	
}


static void
hub_ledger_action_assign(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
gint count;
gboolean usermode = TRUE;
gboolean lockrecon;

	// noaction if show all/closed account
	if(data->showall || data->closed)
		return;

	DB( g_print("\n[hub-ledger] assign\n") );

	lockrecon = gtk_switch_get_active (GTK_SWITCH(data->SW_lockreconciled));

	count = transaction_auto_assign(g_queue_peek_head_link(data->acc->txn_queue), data->acc->key, lockrecon);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_ope));
	GLOBALS->changes_count += count;

	//inform the user
	if(usermode == TRUE)
	{
	gchar *txt;

		if(count == 0)
			txt = _("No transaction changed");
		else
			txt = _("transaction changed: %d");

		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
			_("Automatic assignment result"),
			txt,
			count);
	}

	//refresh main
	if( count > 0 )
		ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void
hub_ledger_action_browse(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[hub-ledger] browse\n") );

	if( account_has_website(data->acc) )
	{
		homebank_util_url_show(data->acc->website);
	}
}


static void
hub_ledger_action_close(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hub_ledger_data *data = user_data;

	DB( g_print("\n[hub-ledger] close\n") );

	DB( g_print(" window %p\n", data->window) );

	gtk_window_close(GTK_WINDOW(data->window));
}


/*static void not_implemented (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
char *text = g_strdup_printf ("Action “%s” not implemented", g_action_get_name (G_ACTION (action)));

	g_print("%s\n", text);
	g_free (text);
}*/


static GActionEntry win_entries[] = {
	{ "txnadd"		, hub_ledger_action_txnaddherit, NULL, NULL, NULL, {0,0,0} },
	{ "txnherit"	, hub_ledger_action_txnaddherit, NULL, NULL, NULL, {0,0,0} },
	{ "txnedit"		, hub_ledger_action_txnedit, NULL, NULL, NULL, {0,0,0} },
	{ "txnmedit"	, hub_ledger_action_txnmedit, NULL, NULL, NULL, {0,0,0} },
	{ "txnapprove"	, hub_ledger_action_txnapprove, NULL, NULL, NULL, {0,0,0} },
	{ "txnreject"	, hub_ledger_action_txndelete, NULL, NULL, NULL, {0,0,0} },

	{ "stanon"		, hub_ledger_action_status_none, NULL, NULL, NULL, {0,0,0} },
	{ "staclr"		, hub_ledger_action_status_clear, NULL, NULL, NULL, {0,0,0} },
	{ "starec"		, hub_ledger_action_status_reconcile, NULL, NULL, NULL, {0,0,0} },

	//func use direct name[3]
	{ "flgn"		, hub_ledger_action_grpflag, NULL, NULL, NULL, {0,0,0} },
	{ "flg1"		, hub_ledger_action_grpflag, NULL, NULL, NULL, {0,0,0} },
	{ "flg2"		, hub_ledger_action_grpflag, NULL, NULL, NULL, {0,0,0} },
	{ "flg3"		, hub_ledger_action_grpflag, NULL, NULL, NULL, {0,0,0} },
	{ "flg4"		, hub_ledger_action_grpflag, NULL, NULL, NULL, {0,0,0} },
	{ "flg5"		, hub_ledger_action_grpflag, NULL, NULL, NULL, {0,0,0} },
	{ "flg6"		, hub_ledger_action_grpflag, NULL, NULL, NULL, {0,0,0} },

	{ "viewsplit"	, hub_ledger_action_viewsplit, NULL, NULL, NULL, {0,0,0} },
	{ "copyamt"		, hub_ledger_action_copyrawamount, NULL, NULL, NULL, {0,0,0} },
	{ "newtpl"		, hub_ledger_action_createtemplate, NULL, NULL, NULL, {0,0,0} },
	{ "newasg"		, hub_ledger_action_createassignment, NULL, NULL, NULL, {0,0,0} },

	{ "txndel"		, hub_ledger_action_txndelete, NULL, NULL, NULL, {0,0,0} },
	{ "txnup"		, hub_ledger_action_move_up, NULL, NULL, NULL, {0,0,0} },
	{ "txndw"		, hub_ledger_action_move_down, NULL, NULL, NULL, {0,0,0} },


	{ "expqif"		, hub_ledger_action_exportqif, NULL, NULL, NULL, {0,0,0} },
	{ "expcsv"		, hub_ledger_action_exportcsv, NULL, NULL, NULL, {0,0,0} },
	{ "print"		, hub_ledger_action_print, NULL, NULL, NULL, {0,0,0} },
	{ "browse"		, hub_ledger_action_browse, NULL, NULL, NULL, {0,0,0} },
	{ "close"		, hub_ledger_action_close, NULL, NULL, NULL, {0,0,0} },

	{ "txncopy"		, hub_ledger_action_edit_copy, NULL, NULL, NULL, {0,0,0} },
	{ "txnpaste"	, hub_ledger_action_edit_paste, NULL, NULL, NULL, {0,0,0} },
	{ "txnpastet"	, hub_ledger_action_edit_paste, NULL, NULL, NULL, {0,0,0} },
	{ "txnclip"		, hub_ledger_action_edit_copy_clipboard, NULL, NULL, NULL, {0,0,0} },
	{ "txnfind"		, hub_ledger_action_find, NULL, NULL, NULL, {0,0,0} },

	{ "mrkdup"		, hub_ledger_action_duplicate_mark, NULL, NULL, NULL, {0,0,0} },
	{ "mrksign"		, hub_ledger_action_check_catsign_mark, NULL, NULL, NULL, {0,0,0} },
	{ "chkxfer"		, hub_ledger_action_check_internal_xfer, NULL, NULL, NULL, {0,0,0} },
	{ "runasg"		, hub_ledger_action_assign, NULL, NULL, NULL, {0,0,0} },
	{ "convert"		, hub_ledger_action_converttoeuro, NULL, NULL, NULL, {0,0,0} },


//	{ "actioname"	, not_implemented, NULL, NULL, NULL, {0,0,0} },
};


static void
_add_accel(gchar *action_name, gchar *accel)
{
const gchar *vaccels[] = { accel, NULL };

	gtk_application_set_accels_for_action(GLOBALS->application, action_name, vaccels);
	//g_menu_item_set_attribute(menuitem, "accel", "s", accel);
}

static void
_add_menuitem(GMenu *menu, gchar *label, gchar *action_name, gchar *accel)
{
GMenuItem *menuitem = g_menu_item_new(label, action_name);

	if( accel != NULL )
		_add_accel(action_name, accel);

	g_menu_append_item (menu, menuitem);
	g_object_unref (menuitem);
}


static GMenu *
hub_ledger_popmenu_create2(struct hub_ledger_data *data)
{
GMenu *menu, *submenu, *section;

	//g_menu_append (submenu, , "ldgr.");
	menu = g_menu_new();

	section = g_menu_new ();
	_add_menuitem (section, _("_Add...")				, "ldgr.txnadd"	, "<Ctrl>N");
	_add_menuitem (section, _("_Inherit...")			, "ldgr.txnherit", "<Ctrl>U");
	_add_menuitem (section, _("_Edit...")				, "ldgr.txnedit", "<Ctrl>E");
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_object_unref (section);

	section = g_menu_new ();
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_object_unref (section);

	submenu = g_menu_new ();
	_add_menuitem (submenu, _("_None")					, "ldgr.stanon", "<Ctrl><Shift>C");
	_add_menuitem (submenu, _("_Cleared")				, "ldgr.staclr", "<Ctrl><Shift>R");
	_add_menuitem (submenu, _("_Reconciled")			, "ldgr.starec", "<Ctrl>R");
	g_menu_append_submenu (section, _("_Status"), G_MENU_MODEL(submenu));
	g_object_unref (submenu);

	submenu = g_menu_new ();
	_add_menuitem (submenu, _("None")					, "ldgr.flgn", "<Ctrl>0");
	_add_menuitem (submenu, _("Red")					, "ldgr.flg1", "<Ctrl>1");
	_add_menuitem (submenu, _("Orange")					, "ldgr.flg2", "<Ctrl>2");
	_add_menuitem (submenu, _("Yellow")					, "ldgr.flg3", "<Ctrl>3");
	_add_menuitem (submenu, _("Green")					, "ldgr.flg4", "<Ctrl>4");
	_add_menuitem (submenu, _("Blue")					, "ldgr.flg5", "<Ctrl>5");
	_add_menuitem (submenu, _("Purple")					, "ldgr.flg6", "<Ctrl>6");
	g_menu_append_submenu (section, _("_Flag"), G_MENU_MODEL(submenu));
	g_object_unref (submenu);

	section = g_menu_new ();
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_object_unref (section);

	_add_menuitem (section, _("Approve")				, "ldgr.txnapprove", NULL);
	_add_menuitem (section, _("Reject (Delete)...")		, "ldgr.txnreject", NULL);

	section = g_menu_new ();
	_add_menuitem (section, _("_Multiple Edit...")		, "ldgr.txnmedit", NULL);
	_add_menuitem (section, _("View _Split")			, "ldgr.viewsplit", NULL);
	_add_menuitem (section, _("Copy raw amount")		, "ldgr.copyamt", NULL);
	_add_menuitem (section, _("Create template...")		, "ldgr.newtpl", NULL);
	_add_menuitem (section, _("Create assignment...")	, "ldgr.newasg", NULL);
	_add_menuitem (section, _("_Delete...")				, "ldgr.txndel", "Delete");
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_object_unref (section);

	section = g_menu_new ();
	_add_menuitem (section, _("_Up")				, "ldgr.txnup", NULL);
	_add_menuitem (section, _("_Down")				, "ldgr.txndw", NULL);
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_object_unref (section);

	return menu;
}


static GMenu *
hub_ledger_menubar_create2(struct hub_ledger_data *data)
{
GMenu *menubar, *menu;
GMenuItem *menuitem;
gboolean showall, closed;

	showall = data->showall;
	closed  = data->closed;

	//g_menu_append (submenu, , "ldgr.");
	menubar = g_menu_new();

	//menu Account
	menu = g_menu_new();
	if( showall == FALSE )
		_add_menuitem(menu, _("Export QIF...")	, "ldgr.expqif"	, NULL);
	_add_menuitem(menu, _("Export CSV...")	, "ldgr.expcsv"	, NULL);
	_add_menuitem(menu, _("Print...")		, "ldgr.print"	, "<Ctrl>P");
	if( showall == FALSE )
		_add_menuitem(menu, _("Browse Website")	, "ldgr.browse"	, NULL);
	_add_menuitem(menu, _("Close")			, "ldgr.close"	, "<Ctrl>W");
	//...
	menuitem = g_menu_item_new (_("A_ccount"), NULL);
	g_menu_item_set_submenu (menuitem, G_MENU_MODEL (menu));
	g_object_unref (menu);
	g_menu_append_item (menubar, menuitem);
	g_object_unref (menuitem);

	//menu Edit
	menu = g_menu_new();
	if( closed == FALSE )
	{
		_add_menuitem(menu, _("Copy")			, "ldgr.txncopy"		, "<Ctrl>C");
		_add_menuitem(menu, _("Paste")			, "ldgr.txnpaste"	, "<Ctrl>V");
		_add_menuitem(menu, _("Paste (today)")	, "ldgr.txnpastet"	, "<Ctrl><Shift>V");
	}
	_add_menuitem(menu, _("Copy clipboard")	, "ldgr.txnclip"		, "<Ctrl><Shift>C");
	_add_menuitem(menu, _("Find")			, "ldgr.txnfind" 	, "<Ctrl>F");
	//...
	menuitem = g_menu_item_new (_("_Edit"), NULL);
	g_menu_item_set_submenu (menuitem, G_MENU_MODEL (menu));
	g_object_unref (menu);
	g_menu_append_item (menubar, menuitem);
	g_object_unref (menuitem);

	//menu Tools
	if( closed == FALSE && showall == FALSE )
	{
		menu = g_menu_new();
		_add_menuitem(menu, _("Mark duplicate...")		, "ldgr.mrkdup"	, NULL);
		_add_menuitem(menu, _("Mark category sign...")	, "ldgr.mrksign"	, NULL);
		_add_menuitem(menu, _("Check internal transfer"), "ldgr.chkxfer"	, NULL);
		_add_menuitem(menu, _("Auto. assignments")		, "ldgr.runasg"	, NULL);
		_add_menuitem(menu, _("Convert to Euro...")		, "ldgr.convert"	, NULL);
		//...
		menuitem = g_menu_item_new (_("_Tools"), NULL);
		g_menu_item_set_submenu (menuitem, G_MENU_MODEL (menu));
		g_object_unref (menu);
		g_menu_append_item (menubar, menuitem);
		g_object_unref (menuitem);
	}
	
	return menubar;
}


static GtkWidget *
hub_ledger_toolbar_create(struct hub_ledger_data *data)
{
GtkWidget *toolbar, *button, *bbox, *hbox, *widget, *label;

	toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);

	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (toolbar), bbox);

		data->BT_up   = button = make_image_button2(ICONNAME_HB_OPE_MOVUP, _("Move transaction up"));
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.txnup");
		gtk_box_prepend (GTK_BOX (bbox), button);
		
		data->BT_down = button = make_image_button2(ICONNAME_HB_OPE_MOVDW, _("Move transaction down"));
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.txndw");
		gtk_box_prepend (GTK_BOX (bbox), button);

	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (toolbar), bbox);

		data->BT_add   = button = make_image_button2(ICONNAME_HB_OPE_ADD, _("Add a new transaction"));
		g_object_set(button, "label", _("Add"), "always-show-image", TRUE, NULL);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.txnadd");
		gtk_box_prepend (GTK_BOX (bbox), button);
		
		data->BT_herit = button = make_image_button2(ICONNAME_HB_OPE_HERIT, _("Inherit from the active transaction"));
		g_object_set(button, "label", _("Inherit"), "always-show-image", TRUE, NULL);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.txnherit");
		gtk_box_prepend (GTK_BOX (bbox), button);
		
		data->BT_edit  = button = make_image_button2(ICONNAME_HB_OPE_EDIT, _("Edit the active transaction"));
		g_object_set(button, "label", _("Edit"), "always-show-image", TRUE, NULL);
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.txnedit");
		gtk_box_prepend (GTK_BOX (bbox), button);

	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (toolbar), bbox);

		data->BT_clear = button     = make_image_button2(ICONNAME_HB_OPE_CLEARED, _("Toggle cleared for selected transaction(s)"));
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.staclr");
		gtk_box_prepend (GTK_BOX (bbox), button);
		
		data->BT_reconcile = button = make_image_button2(ICONNAME_HB_OPE_RECONCILED, _("Toggle reconciled for selected transaction(s)"));
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.starec");
		gtk_box_prepend (GTK_BOX (bbox), button);

		data->BT_multiedit = button = make_image_button2(ICONNAME_HB_OPE_MULTIEDIT, _("Edit multiple transaction"));
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.txnmedit");
		gtk_box_prepend (GTK_BOX (bbox), button);

		data->BT_delete    = button = make_image_button2(ICONNAME_HB_OPE_DELETE, _("Delete selected transaction(s)"));
		gtk_actionable_set_action_name (GTK_ACTIONABLE (button), "ldgr.txndel");
		gtk_box_prepend (GTK_BOX (bbox), button);

	//#1909749 lock/unlock reconciled
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_widget_set_margin_start (hbox, SPACING_LARGE);
	gtk_box_prepend (GTK_BOX (toolbar), hbox);

		label = gtk_label_new (_("Reconciled changes is"));
		gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
		data->LB_lockreconciled = label;
		gtk_box_prepend (GTK_BOX (hbox), label);

		//widget = hbtk_image_new_from_icon_name_16 (ICONNAME_CHANGES_PREVENT);
		widget = gtk_image_new();
		g_object_set(widget, "icon-name", ICONNAME_CHANGES_PREVENT, NULL);
		data->IM_lockreconciled = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = gtk_switch_new();
		data->SW_lockreconciled = widget;
		gtk_widget_set_valign (widget, GTK_ALIGN_CENTER);
		gtk_box_prepend (GTK_BOX (hbox), widget);

	return toolbar;
}


static gboolean 
hub_ledger_window_getgeometry(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
//struct hub_ledger_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[hub-ledger] get geometry\n") );

	//store position and size
	wg = &PREFS->acc_wg;
	wg->s = gtk_window_is_maximized(GTK_WINDOW(widget));
	if(!wg->s)
	{
		gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
		gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);
	}
	
	//DB( g_print(" window: l=%d, t=%d, w=%d, h=%d s=%d\n", wg->l, wg->t, wg->w, wg->h, wg->s) );

	return FALSE;
}


static gboolean
hub_ledger_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct hub_ledger_data *data = user_data;
struct WinGeometry *wg;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print("\n[hub-ledger] -- delete-event\n") );

	wg = &PREFS->acc_wg;
	wg->s = gtk_window_is_maximized(GTK_WINDOW(widget));
	//store columns
	list_txn_get_columns(GTK_TREE_VIEW(data->LV_ope));

	return FALSE;
}


static gboolean
hub_ledger_window_destroy (GtkWidget *widget, gpointer user_data)
{
struct hub_ledger_data *data;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");


	DB( g_print ("\n[hub-ledger] -- destroy event\n") );



	//enable define windows
	GLOBALS->define_off--;

	//5.8.6 unmark
	if( data->similar > 0 )
	{
		DB( g_print(" unmark similar\n") );
		hub_ledger_action_duplicate_unmark(NULL, NULL, data);
	}

	/* free title and filter */
	DB( g_print(" user_data=%p to be free\n", user_data) );
	g_free(data->wintitle);

	if(data->gpatxn != NULL)
		g_ptr_array_free (data->gpatxn, TRUE);

	g_queue_free_full(data->q_txn_clip, (GDestroyNotify)da_transaction_free);
	da_flt_free(data->filter);

	g_free(data);


	//our global list has changed, so update the treeview
	//TODO: find another way to signal
	//do it on mainwindow focus??
	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_TXNLIST+UF_REFRESHALL));

	return FALSE;
}


/*
 * if accnum = 0 or acc is null : show all account 
 */
GtkWidget *
hub_ledger_window_new(Account *acc)
{
struct hub_ledger_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainvbox, *intbox, *actionbox, *labelbox, *hbox;
GtkWidget *menubar, *bar, *scrollwin, *treeview, *label, *widget;
GActionGroup *actions;
GMenu *gmenumodel;

	DB( g_print("\n[hub-ledger] create_hub_ledger_window\n") );

	data = g_malloc0(sizeof(struct hub_ledger_data));
	if(!data) return NULL;

	//disable define windows
	GLOBALS->define_off++;
	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	/* setup TODO: to moove */
	data->filter = da_flt_malloc();
	data->q_txn_clip = g_queue_new ();

	
	/* create window, etc */
	window = gtk_application_window_new(GLOBALS->application);
	gtk_widget_set_name(GTK_WIDGET(window), "ledger");
	data->window = window;

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" new window=%p, inst_data=%p\n", window, data) );

	//init global vars
	data->acc = acc;
	data->showall = FALSE;
	data->closed  = FALSE;

	if( acc != NULL )
	{
		g_object_set_data(G_OBJECT(window), "key", GINT_TO_POINTER(acc->key));
		if( (data->acc->flags & AF_CLOSED) == FALSE )
		{
		//#2058696 add number & institutioname
		GString *node = g_string_sized_new(64);

			g_string_append(node, acc->name);
			//#2065758
			if( (acc->number != NULL) && (strlen(acc->number) > 0) )
			{
				g_string_append(node, " : ");
				g_string_append(node, acc->number);
			}
			//#2065758
			if( (acc->bankname != NULL) && (strlen(acc->bankname) > 0) )
			{
				g_string_append(node, ", ");
				g_string_append(node, acc->bankname);
			}
			g_string_append(node, " - HomeBank");
			data->wintitle = g_string_free(node, FALSE);
		}
		else
		{
			data->wintitle = g_strdup_printf("%s %s - HomeBank", data->acc->name, _("(closed)"));
			data->closed = TRUE;
		}
	}
	else
	{
		g_object_set_data(G_OBJECT(window), "key", GINT_TO_POINTER(-1));
		data->wintitle = g_strdup_printf(_("%s - HomeBank"), _("All transactions"));
		data->showall = TRUE;
	}

	gtk_window_set_title (GTK_WINDOW (window), data->wintitle);


	// action group
	actions = (GActionGroup*)g_simple_action_group_new ();
	data->actions = actions;
	
	g_action_map_add_action_entries (G_ACTION_MAP (actions),
		win_entries, G_N_ELEMENTS (win_entries), data);

	gtk_widget_insert_action_group (window, "ldgr", actions);


	/* connect signal */
	g_signal_connect (window, "destroy", G_CALLBACK (hub_ledger_window_destroy), (gpointer)data);
	g_signal_connect (window, "delete-event", G_CALLBACK (hub_ledger_window_dispose), (gpointer)data);


	g_signal_connect (window, "configure-event",	G_CALLBACK (hub_ledger_window_getgeometry), (gpointer)data);

	g_signal_connect (window, "key-press-event", G_CALLBACK (hub_ledger_cb_on_key_press), (gpointer)data);


	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_window_set_child(GTK_WINDOW(window), mainvbox);

	//1 - menubar
	gmenumodel = hub_ledger_menubar_create2(data);
	#if( (GTK_MAJOR_VERSION < 4)  )
		menubar = gtk_menu_bar_new_from_model(G_MENU_MODEL(gmenumodel));
	#else
		menubar = gtk_popover_menu_bar_new_from_model(G_MENU_MODEL(gmenumodel));
	#endif
	gtk_box_prepend (GTK_BOX (mainvbox), menubar);	

	//2 - account txn notification
	bar = gtk_info_bar_new ();
	data->IB_accnotif = bar;
	gtk_box_prepend (GTK_BOX (mainvbox), bar);

	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	//gtk_info_bar_set_show_close_button (GTK_INFO_BAR (bar), TRUE);

		label = gtk_label_new (NULL);
		data->LB_accnotif = label;
		//gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		//gtk_label_set_xalign (GTK_LABEL (label), 0);
		gtk_box_prepend (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label);

		widget = gtk_button_new_with_label(_("Show"));
		data->BT_info_showpending = widget;
		gtk_box_prepend (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), widget);

	//3 - info bar for duplicate
	bar = gtk_info_bar_new_with_buttons (_("_Refresh"), HB_RESPONSE_REFRESH, NULL);
	data->IB_duplicate = bar;
	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	gtk_info_bar_set_show_close_button (GTK_INFO_BAR (bar), TRUE);
	gtk_box_prepend (GTK_BOX (mainvbox), bar);

		hbox = gtk_info_bar_get_content_area (GTK_INFO_BAR (bar));

		label = gtk_label_new (NULL);
		data->LB_duplicate = label;
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_label_set_xalign (GTK_LABEL (label), 0);
		gtk_box_prepend (GTK_BOX (hbox), label);
		//5.8.6
		label = make_label(_("Date _gap:"), 0, 0.5);
		gtk_box_prepend (GTK_BOX (hbox), label);
		widget = make_numeric(label, 0, HB_DATE_MAX_GAP);
		data->NB_txn_daygap = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

	//3c - bar for category sign check
	bar = gtk_info_bar_new ();
	data->IB_chkcatsign = bar;
	gtk_box_prepend (GTK_BOX (mainvbox), bar);

	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	gtk_info_bar_set_show_close_button (GTK_INFO_BAR (bar), TRUE);

		label = gtk_label_new (NULL);
		data->LB_chkcatsign = label;
		//gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		//gtk_label_set_xalign (GTK_LABEL (label), 0);
		gtk_box_prepend (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label);

	//4 - windows interior
	intbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	hb_widget_set_margin(GTK_WIDGET(intbox), SPACING_SMALL);
	hbtk_box_prepend (GTK_BOX (mainvbox), intbox);

	//4a - actionbox
	actionbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_box_prepend (GTK_BOX (intbox), actionbox);

		hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
		gtk_widget_set_halign(hbox, GTK_ALIGN_START);
		scrollwin = make_scrolled_window_ns(GTK_POLICY_AUTOMATIC, GTK_POLICY_NEVER);
		hbtk_box_prepend (GTK_BOX (actionbox), scrollwin);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), hbox);

		widget = make_daterange(NULL, DATE_RANGE_FLAG_CUSTOM_DISABLE);
		data->CY_range = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = make_image_toggle_button2(ICONNAME_HB_OPE_FUTURE, NULL);
		data->CM_future = widget;
		//#2008521 set more accurate tooltip
		gchar *tt = g_strdup_printf(_("Toggle show %d days ahead"), PREFS->date_future_nbdays);
		gtk_widget_set_tooltip_text (widget, tt);
		g_free(tt);
		gtk_box_prepend (GTK_BOX (hbox), widget);

	//5.8 flag
		widget = make_fltgrpflag(NULL);
		data->CY_flag = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = hbtk_combo_box_new_with_data(label, CYA_FLT_TYPE);
		data->CY_type = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		widget = hbtk_combo_box_new_with_data(label, CYA_FLT_STATUS);
		data->CY_status = widget;
		gtk_box_prepend (GTK_BOX (hbox), widget);

		//5.8 beta test
		if( data->showall )
		{
			widget = create_popover_widget(GTK_WINDOW(data->window), data->filter);
			data->PO_hubfilter = widget;
			gtk_box_prepend (GTK_BOX (hbox), widget);
		}


	widget = make_image_button2(ICONNAME_HB_FILTER, _("Edit filter"));
	data->BT_filter = widget;
	gtk_box_prepend (GTK_BOX (actionbox), widget);

	//widget = gtk_button_new_with_mnemonic (_("Reset _filters"));
	//widget = gtk_button_new_with_mnemonic (_("_Reset"));
	widget = make_image_button2(ICONNAME_HB_CLEAR, _("Clear filter"));
	data->BT_reset = widget;
	gtk_box_prepend (GTK_BOX (actionbox), widget);


	widget = make_image_button2(ICONNAME_HB_REFRESH, _("Refresh results"));
	data->BT_refresh = widget;
	gtk_box_prepend (GTK_BOX (actionbox), widget);

	widget = make_image_toggle_button2(ICONNAME_HB_LIFEENERGY, _("Toggle Life Energy"));
	data->BT_lifnrg = widget;
	gtk_box_prepend (GTK_BOX (actionbox), widget);

	//TRANSLATORS: this is for Euro specific users, a toggle to display in 'Minor' currency
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_box_prepend (GTK_BOX (actionbox), widget);

	//quick search
	widget = make_search ();
	data->ST_search = widget;
	gtk_widget_set_size_request(widget, HB_MINWIDTH_SEARCH, -1);
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	gtk_box_prepend (GTK_BOX (actionbox), widget);


	/* grid line 2 */
	labelbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_margin_start(labelbox, SPACING_TINY);
	gtk_widget_set_margin_end(labelbox, SPACING_TINY);
	//gtk_widget_set_hexpand(actionbox, TRUE);
	gtk_box_prepend (GTK_BOX (intbox), labelbox);

	// text total/selection
	label = make_label(NULL, 0.0, 0.5);
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	//#1930395 text selectable for copy/paste 
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	//gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_halign (label, GTK_ALIGN_CENTER);
	gtk_widget_set_hexpand (label, TRUE);
	data->TX_selection = label;
	gtk_box_prepend (GTK_BOX (labelbox), label);

	label = gtk_label_new(_("Reconciled:"));
	gtk_widget_set_margin_start(label, SPACING_MEDIUM);
	gtk_box_prepend (GTK_BOX (labelbox), label);
	widget = gtk_label_new(NULL);
	gtk_widget_set_margin_start(widget, SPACING_TINY);
	data->TX_balance[0] = widget;
	gtk_box_prepend (GTK_BOX (labelbox), widget);

	label = gtk_label_new(_("Cleared:"));
	gtk_widget_set_margin_start(label, SPACING_MEDIUM);
	gtk_box_prepend (GTK_BOX (labelbox), label);
	widget = gtk_label_new(NULL);
	gtk_widget_set_margin_start(widget, SPACING_TINY);
	data->TX_balance[1] = widget;
	gtk_box_prepend (GTK_BOX (labelbox), widget);

	label = gtk_label_new(_("Today:"));
	gtk_widget_set_margin_start(label, SPACING_MEDIUM);
	gtk_box_prepend (GTK_BOX (labelbox), label);
	widget = gtk_label_new(NULL);
	gtk_widget_set_margin_start(widget, SPACING_TINY);
	data->TX_balance[2] = widget;
	gtk_box_prepend (GTK_BOX (labelbox), widget);

	label = gtk_label_new(_("Future:"));
	gtk_widget_set_margin_start(label, SPACING_MEDIUM);
	gtk_box_prepend (GTK_BOX (labelbox), label);
	widget = gtk_label_new(NULL);
	gtk_widget_set_margin_start(widget, SPACING_TINY);
	data->TX_balance[3] = widget;
	gtk_box_prepend (GTK_BOX (labelbox), widget);

	
	//list
	GtkWidget *lbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hbtk_box_prepend (GTK_BOX (intbox), lbox);
	
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	treeview = (GtkWidget *)create_list_transaction(LIST_TXN_TYPE_BOOK, PREFS->lst_ope_columns);
	data->LV_ope = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	hbtk_box_prepend (GTK_BOX (lbox), scrollwin);

	list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);
	

	
	/* toolbars */
	bar = hub_ledger_toolbar_create(data);
	data->TB_bar = bar;
	gtk_style_context_add_class (gtk_widget_get_style_context (bar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_prepend (GTK_BOX (lbox), bar);

	//TODO should move this
	//setup
	//TODO minor data seems no more used
	data->lockreconciled = PREFS->safe_lock_recon;
	gtk_switch_set_active(GTK_SWITCH(data->SW_lockreconciled), PREFS->safe_lock_recon);
	list_txn_set_lockreconciled(GTK_TREE_VIEW(data->LV_ope), PREFS->safe_lock_recon);
	
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope))), "minor", data->CM_minor);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_future), (PREFS->date_future_nbdays > 0) ? TRUE : FALSE );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor), GLOBALS->minor);
	gtk_widget_grab_focus(GTK_WIDGET(data->LV_ope));

	// connect signals
	g_signal_connect (data->BT_info_showpending, "clicked", G_CALLBACK (hub_ledger_info_cb_show_pending), NULL);



	g_signal_connect (data->IB_duplicate    , "response", G_CALLBACK (hub_ledger_cb_bar_duplicate_response), NULL);
	g_signal_connect (data->IB_chkcatsign   , "response", G_CALLBACK (hub_ledger_cb_bar_chkcatsign_response), NULL);

	g_signal_connect (data->SW_lockreconciled, "state-set", G_CALLBACK (hub_ledger_cb_recon_change), NULL);

	data->handler_id[HID_RANGE]	 = g_signal_connect (data->CY_range , "changed", G_CALLBACK (hub_ledger_cb_filter_daterange), NULL);
	g_signal_connect (data->CY_flag, "changed", G_CALLBACK (hub_ledger_cb_filterbar_change), NULL);
	data->handler_id[HID_TYPE]	 = g_signal_connect (data->CY_type	, "changed", G_CALLBACK (hub_ledger_cb_filterbar_change), NULL);
	data->handler_id[HID_STATUS] = g_signal_connect (data->CY_status, "changed", G_CALLBACK (hub_ledger_cb_filterbar_change), NULL);

	g_signal_connect (data->CM_future, "toggled", G_CALLBACK (hub_ledger_cb_filter_daterange), NULL);

	if( data->showall )
		g_signal_connect( ui_flt_popover_hub_get_combobox(GTK_BOX(data->PO_hubfilter), NULL), "changed", G_CALLBACK (beta_hub_ledger_cb_preset_change), NULL);

	g_signal_connect (data->BT_reset  , "clicked", G_CALLBACK (hub_ledger_cb_filter_reset), NULL);
	g_signal_connect (data->BT_refresh, "clicked", G_CALLBACK (hub_ledger_cb_refresh), NULL);

	g_signal_connect (data->BT_filter , "clicked", G_CALLBACK (hub_ledger_cb_editfilter), NULL);

	g_signal_connect (data->BT_lifnrg , "clicked", G_CALLBACK (hub_ledger_cb_button_lifenergy), NULL);
	
	g_signal_connect (data->CM_minor , "toggled", G_CALLBACK (hub_ledger_toggle_minor), NULL);
	
	data->handler_id[HID_SEARCH] = g_signal_connect (data->ST_search, "search-changed", G_CALLBACK (quick_search_text_changed_cb), data);
	//#1879451 deselect all when quicksearch has the focus (to prevent delete)
	g_signal_connect (data->ST_search, "focus-in-event", G_CALLBACK (hub_ledger_cb_search_focus_in_event), NULL);
	
	//g_signal_connect (GTK_TREE_VIEW(treeview), "cursor-changed", G_CALLBACK (hub_ledger_update), (gpointer)2);
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK (hub_ledger_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(treeview), "row-activated", G_CALLBACK (hub_ledger_onRowActivated), GINT_TO_POINTER(2));

	//new GMenu
	gmenumodel = hub_ledger_popmenu_create2(data);
	GtkWidget *gtkmenu = gtk_menu_new_from_model(G_MENU_MODEL(gmenumodel));
	//always attach to get sensitive gaction
	gtk_menu_attach_to_widget(GTK_MENU(gtkmenu), treeview, NULL);
	g_signal_connect (treeview, "button-press-event", G_CALLBACK (listview_context_cb), gtkmenu);


	//setup, init and show window
	wg = &PREFS->acc_wg;
	
	DB( g_print(" set default size w:%d h:%d m:%d\n", wg->w, wg->h, wg->s) );
	gtk_window_set_default_size(GTK_WINDOW(window), wg->w, wg->h);
	//gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);
	if(wg->s == 0)
	{
		if( wg->l && wg->t )
			gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
		DB( g_print(" move to %d %d\n", wg->l, wg->t) );
	}
	else
		gtk_window_maximize(GTK_WINDOW(window));

	DB( g_print(" show\n") );
	gtk_widget_show_all (window);
	
	gtk_widget_hide(data->IB_accnotif);
	gtk_widget_hide(data->IB_duplicate);
	gtk_widget_hide(data->IB_chkcatsign);

	return window;
}


