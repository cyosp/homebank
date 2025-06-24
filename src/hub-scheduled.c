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


#include "homebank.h"

#include "dsp-mainwindow.h"
#include "list-scheduled.h"
#include "hub-scheduled.h"

#include "ui-transaction.h"
#include "ui-dialogs.h"
#include "ui-widgets.h"


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


extern HbKvData CYA_FLT_SCHEDULED[];


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* scheduled */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void ui_hub_scheduled_onRowActivated (GtkTreeView *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
struct hbfile_data *data;
GtkTreeModel *model;
GList *selection, *list;
gint count;

	DB( g_print ("\n[hub-scheduled] row double-clicked\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc)));
	if( count == 1 )
	{
		//model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc));
		selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc)), &model);

		list = g_list_first(selection);
		if(list != NULL)
		{
		GtkTreeIter iter;
		Archive *arc;

			gtk_tree_model_get_iter(model, &iter, list->data);
			gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arc, -1);

			//fix: don't open for total line
			if( arc != NULL )
				ui_wallet_defarchive(arc);

		}

		g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
		g_list_free(selection);
	}
}


static void ui_hub_scheduled_do_post(Archive *arc, gboolean doedit, gpointer user_data)
{
struct hbfile_data *data = user_data;
GtkWidget *window;
gint result;
Transaction *txn;

	DB( g_print("\n[hub-scheduled] do post\n") );

	
	window =  create_deftransaction_window(GTK_WINDOW(data->window), TXN_DLG_ACTION_ADD, TXN_DLG_TYPE_SCH, 0);

	/* fill in the transaction */
	txn = da_transaction_malloc();
	da_transaction_init_from_template(txn, arc);
	txn->date = scheduled_get_txn_real_postdate(arc->nextdate, arc->weekend);

	deftransaction_set_transaction(window, txn);

	result = gtk_dialog_run (GTK_DIALOG (window));

	DB( g_print(" - dialog result is %d\n", result) );

	if(result == HB_RESPONSE_ADD || result == GTK_RESPONSE_ACCEPT)
	{
		deftransaction_get(window, NULL);
		transaction_add(GTK_WINDOW(GLOBALS->mainwindow), FALSE, txn);
		GLOBALS->changes_count++;

		scheduled_date_advance(arc);

		DB( g_print(" - added 1 transaction to %d\n", txn->kacc) );
	}

	da_transaction_free(txn);

	deftransaction_dispose(window, NULL);
	gtk_window_destroy (GTK_WINDOW(window));

}


static void ui_hub_scheduled_editpost_cb(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
	
	DB( g_print("\n[hub-scheduled] editpost\n") );
	
	selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc)), &model);

	list = g_list_first(selection);
	while(list != NULL)
	{
	GtkTreeIter iter;
	Archive *arc;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arc, -1);

		if( (arc != NULL) )
		{
			ui_hub_scheduled_do_post(arc, TRUE, data);
		}

		list = g_list_next(list);
	}

	g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selection);

	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE|UF_REFRESHALL));

}


static void ui_hub_scheduled_post_cb(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
	
	DB( g_print("\n[hub-scheduled] post\n") );

	selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc)), &model);

	list = g_list_first(selection);
	while(list != NULL)
	{
	GtkTreeIter iter;
	Archive *arc;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arc, -1);
	
		if( (arc != NULL) )
		{
			if( scheduled_is_postable(arc) )
			{
			Transaction *txn = da_transaction_malloc ();

				da_transaction_init_from_template(txn, arc);
				txn->date = scheduled_get_txn_real_postdate(arc->nextdate, arc->weekend);
				transaction_add(GTK_WINDOW(GLOBALS->mainwindow), FALSE, txn);

				GLOBALS->changes_count++;
				scheduled_date_advance(arc);

				da_transaction_free (txn);
			}
			else
			{
				ui_hub_scheduled_do_post(arc, FALSE, data);
			}

		}

		list = g_list_next(list);
	}

	g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selection);

	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE|UF_REFRESHALL));
	
}


static void ui_hub_scheduled_skip_cb(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data = user_data;
GtkTreeModel *model;
GList *selection, *list;
	
	DB( g_print("\n[hub-scheduled] skip\n") );

	selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc)), &model);

	list = g_list_first(selection);
	while(list != NULL)
	{
	GtkTreeIter iter;
	Archive *arc;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, LST_DSPUPC_DATAS, &arc, -1);

		DB( g_print(" %s %f\n", arc->memo, arc->amount) );

		if( (arc != NULL) && (arc->rec_flags & TF_RECUR) )
		{
			GLOBALS->changes_count++;
			scheduled_date_advance(arc);
			DB( g_print(" >skipping\n") );
			//ui_hub_scheduled_populate(GLOBALS->mainwindow, NULL);
		}
		
		list = g_list_next(list);
	}

	g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(selection);
	ui_hub_scheduled_populate(GLOBALS->mainwindow, NULL);
	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));
}


static void ui_hub_scheduled_update(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeSelection *selection;
gchar *msg;
gint count;
//gint filter;

	DB( g_print("\n[hub-scheduled] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//filter = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_sched_filter));

	// sensitive against selection
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc));
	count = gtk_tree_selection_count_selected_rows(selection);

	//Archive *arc = ui_hub_scheduled_get_selected_item(GTK_TREE_VIEW(data->LV_upc));

	if(count >= 1)
	{
		//DB( g_print("archive is %s\n", arc->memo) );
		
		gtk_widget_set_sensitive(GTK_WIDGET(data->BT_sched_skip), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(data->BT_sched_post), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(data->BT_sched_editpost), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(GTK_WIDGET(data->BT_sched_skip), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(data->BT_sched_post), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(data->BT_sched_editpost), FALSE);
	}

	//#1996505 add sum of selected
	if( count > 1 )
	{
	GList *list, *tmplist;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar buf1[64];
	gchar buf2[64];
	gchar buf3[64];
	gdouble sumexp, suminc;

		DB( g_print(" update list info\n") );

		sumexp = suminc = 0.0;
		//model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc));
		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc));
		list = gtk_tree_selection_get_selected_rows(selection, &model);
		tmplist = g_list_first(list);
		while (tmplist != NULL)
		{
		Archive *arc;
		Account *acc = NULL;

			gtk_tree_model_get_iter(model, &iter, tmplist->data);
			gtk_tree_model_get(model, &iter, 
				LST_DSPUPC_DATAS, &arc,
				-1);

			//DB( g_print(" collect %f - %f = %f %s\n", txninc, txnexp, txninc + txnexp, item->memo) );

			acc = da_acc_get(arc->kacc);
			if( acc != NULL )
			{
				if( arc->flags & OF_INCOME )
					suminc += hb_amount_base(arc->amount, acc->kcur);
				else
					sumexp += hb_amount_base(arc->amount, acc->kcur);
			}

			/* insert internal xfer txn : 1378836 */
			if( (arc->flags & OF_INTXFER) )
			{
			gdouble amount = -arc->amount;

				if( arc->flags & OF_ADVXFER )
				{
					amount = arc->xferamount;
					DB( g_print("  xfer amount is != curr %.17g\n", amount ) );
				}
				
				/* opposite here */
				acc = da_acc_get(arc->kxferacc);
				if( acc != NULL )
				{
					if( arc->flags & OF_INCOME )
						sumexp += hb_amount_base(amount, acc->kcur);
					else
						suminc += hb_amount_base(amount, acc->kcur);
				}
			}

			DB( g_print(" %.17g - %.17g = %.17g temp\n", suminc, sumexp, suminc + sumexp) );
		
			tmplist = g_list_next(tmplist);
		}
		g_list_free(list);

		DB( g_print(" %.17g - %.17g = %.17g final\n", suminc, sumexp, suminc + sumexp) );

		hb_strfmon(buf1, 64-1, suminc + sumexp, GLOBALS->kcur, GLOBALS->minor);
		hb_strfmon(buf2, 64-1, sumexp, GLOBALS->kcur, GLOBALS->minor);
		hb_strfmon(buf3, 64-1, suminc, GLOBALS->kcur, GLOBALS->minor);

		//TRANSLATORS: example 'sum: 3 (-1 + 4)'
		msg = g_strdup_printf(_("sum: %s (%s + %s)"), buf1, buf2, buf3);
		gtk_label_set_markup(GTK_LABEL(data->TX_selection), msg);
		g_free (msg);
	}
	else
		gtk_label_set_markup(GTK_LABEL(data->TX_selection), "");

}


static void ui_hub_scheduled_selection_cb(GtkTreeSelection *treeselection, gpointer user_data)
{
	DB( g_print("\n[hub-scheduled] selection\n") );

	ui_hub_scheduled_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(UF_SENSITIVE));
}



/*
** called after load, importamiga, on demand
*/
void ui_hub_scheduled_postall(GtkWidget *widget, gpointer user_data)
{
//struct hbfile_data *data;
gint count;
gint usermode = GPOINTER_TO_INT(user_data);

	DB( g_print("\n[hub-scheduled] post all\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	count = scheduled_post_all_pending();

	//inform the user
	if(usermode == TRUE)
	{
	gchar *txt;

		//#125534
		if( count > 0 )
		{
			ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_REFRESHALL));
		}
		
		if(count == 0)
			txt = _("No transaction to add");
		else
			txt = _("transaction added: %d");

		ui_dialog_msg_infoerror(GTK_WINDOW(GLOBALS->mainwindow), GTK_MESSAGE_INFO,
			_("Check scheduled transactions result"),
			txt,
			count);
	}

}


void ui_hub_scheduled_populate(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
GtkTreeIter  iter;
GtkTreeSelection *selection;
GList *list;
gdouble totexp = 0;
gdouble totinc = 0;
gint count = 0;
gchar buffer[256], *tooltip;
guint32 maxpostdate = 0;
guint32 fltmindate, fltmaxdate;
GDate *date;

	DB( g_print("\n[hub-scheduled] populate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc));

	g_signal_handlers_block_by_func (G_OBJECT (selection), G_CALLBACK (ui_hub_scheduled_selection_cb), NULL);


	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc));
	gtk_list_store_clear (GTK_LIST_STORE(model));

	//5.7.4 add
	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_upc), NULL); /* Detach model from view */

	ui_arc_listview_widget_columns_order_load(GTK_TREE_VIEW(data->LV_upc));

	homebank_app_date_get_julian();

	PREFS->pnl_upc_range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_sched_range));

	//set tooltip text
	maxpostdate = scheduled_date_get_post_max(GLOBALS->today, GLOBALS->auto_smode, GLOBALS->auto_nbdays, GLOBALS->auto_weekday, GLOBALS->auto_nbmonths);
	date = g_date_new_julian (maxpostdate);
	g_date_strftime (buffer, 256-1, PREFS->date_format, date);

	//post when program start: ON/OFF
	tooltip = g_strdup_printf("%s: %s\n%s: %s", 
		_("Post when program start"), PREFS->appendscheduled ? _("On") : _("Off"),
		_("maximum post date"),	buffer);
	//gtk_label_set_text(GTK_LABEL(data->LB_maxpostdate), buffer);
	gtk_widget_set_tooltip_text(data->IM_info, tooltip);

	g_free(tooltip);


	fltmindate = HB_MINDATE;
	fltmaxdate = HB_MAXDATE;
	scheduled_date_get_show_minmax(PREFS->pnl_upc_range, &fltmindate, &fltmaxdate);

	//#1909851 5.7 override if FLT_SCHEDULED_MAXPOSTDATE
	if( PREFS->pnl_upc_range == FLT_SCHEDULED_MAXPOSTDATE )
		fltmaxdate = maxpostdate;

	DB( hb_print_date(GLOBALS->today, "today" ) );
	DB( hb_print_date(maxpostdate, "maxpostdate" ) );
	DB( hb_print_date(fltmindate, "fltmindate" ) );
	DB( hb_print_date(fltmaxdate, "fltmaxdate" ) );

	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *arc = list->data;
	Account *acc;
	gdouble inc, exp;
	gint nbdays, nblate;

		if( (arc->rec_flags & TF_RECUR) ) //&& arc->kacc > 0)
		{
			count++;
			nbdays = arc->nextdate - maxpostdate;
			//nblate = scheduled_get_latepost_count(date, arc, GLOBALS->today);
			nblate = scheduled_get_latepost_count(date, arc, maxpostdate);

			DB( g_print(" eval %d in [%d-%d] ? days %d late %d, memo='%s'\n", arc->nextdate, fltmindate, fltmaxdate, nbdays, nblate, arc->memo) );

			if( nblate == 0 )
			{
				//#1857636 hide due date scheduled > custom user config if > 0
				if( (arc->nextdate < fltmindate) || (arc->nextdate > fltmaxdate)  )
				{
					DB( g_print("  skip '%s' : next %d >= maxshow %d\n", arc->memo, arc->nextdate, PREFS->pnl_upc_range) );
					//TODO: count hidden
					
					goto next;
				}
			}

			exp = inc = 0.0;
			if( arc->flags & OF_INCOME )
				inc = arc->amount;
			else
				exp = arc->amount;

			acc = da_acc_get(arc->kacc);
			if( acc != NULL )
			{
				DB( g_print("  add totals: %.17g %.17g\n", exp, inc) );
				if( arc->flags & OF_INCOME )
					totinc += hb_amount_base(arc->amount, acc->kcur);
				else
					totexp += hb_amount_base(arc->amount, acc->kcur);
			}

		/* good */

			/* insert internal xfer txn : 1378836 */
			if( (arc->flags & OF_INTXFER) )
			{
			gdouble amount = -arc->amount;

				if( arc->flags & OF_ADVXFER )
				{
					amount = arc->xferamount;
					DB( g_print("  xfer amount is != curr %.17g\n", amount ) );
				}
				
				/* opposite here */
				if( arc->flags & OF_INCOME )
					exp = amount;
				else
					inc = amount;

				acc = da_acc_get(arc->kxferacc);
				if( acc != NULL )
				{
					DB( g_print("  add totals: %.17g %.17g\n", exp, inc) );
					if( arc->flags & OF_INCOME )
						totexp += hb_amount_base(amount, acc->kcur);
					else
						totinc += hb_amount_base(amount, acc->kcur);
				}
			}

			gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				  LST_DSPUPC_DATAS, arc,
			      LST_DSPUPC_NEXT, nbdays,
				  //LST_DSPUPC_ACCOUNT, acc,
			      LST_DSPUPC_MEMO, arc->memo,
			      LST_DSPUPC_EXPENSE, exp,
			      LST_DSPUPC_INCOME , inc,
			      LST_DSPUPC_NB_LATE, nblate,
				  -1);

			DB( g_print("  totals: %.17g %.17g\n", totexp, totinc) );

		}
next:
		list = g_list_next(list);
	}

	g_date_free(date);

	// insert total
	if(count > 0 )
	{
		DB( g_print(" insert totals: %.17g %.17g\n", totexp, totinc) );

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			  LST_DSPUPC_DATAS, NULL,
			  LST_DSPUPC_MEMO, _("Total"),
			  LST_DSPUPC_EXPENSE, totexp,
		      LST_DSPUPC_INCOME, totinc,
		  -1);
	}

	//added 5.7.4
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_upc), model); /* Re-attach model to view */
	g_object_unref(model);
	g_signal_handlers_unblock_by_func (G_OBJECT (selection), G_CALLBACK (ui_hub_scheduled_selection_cb), NULL);

	ui_hub_scheduled_update(widget, NULL);
}



static void
ui_hub_scheduled_clipboard (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hbfile_data *data = user_data;
GtkClipboard *clipboard;
GString *node;

	//g_print ("Action %s activated\n", g_action_get_name (G_ACTION (action)));

	node = lst_sch_widget_to_string(GTK_TREE_VIEW(data->LV_upc), HB_STRING_PRINT);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void
ui_hub_scheduled_print (GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct hbfile_data *data = user_data;
GString *node;

	//g_print ("Action %s activated\n", g_action_get_name (G_ACTION (action)));

	gint8 leftcols[8] = { 0, 1, 2, 3, 6, 7, -1 };

	node = lst_sch_widget_to_string(GTK_TREE_VIEW(data->LV_upc), HB_STRING_PRINT);
	hb_print_listview(GTK_WINDOW(data->window), node->str, leftcols, _("Scheduled"), NULL, FALSE);

	g_string_free(node, TRUE);
}



static const GActionEntry actions[] = {
//	name, function(), type, state, 
	{ "clipboard"	, ui_hub_scheduled_clipboard			, NULL, NULL , NULL, {0,0,0} },
	{ "print"		, ui_hub_scheduled_print				, NULL, NULL , NULL, {0,0,0} },
//  { "paste", activate_action, NULL, NULL,      NULL, {0,0,0} },
};


GtkWidget *ui_hub_scheduled_create(struct hbfile_data *data)
{
GtkWidget *hub, *vbox, *bbox, *scrollwin, *treeview, *tbar;
GtkWidget *label, *widget;

	DB( g_print("\n[hub-scheduled] create\n") );
	
	hub = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hb_widget_set_margin(GTK_WIDGET(hub), SPACING_SMALL);

	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	hbtk_box_prepend (GTK_BOX (hub), vbox);

	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	hbtk_box_prepend (GTK_BOX (vbox), scrollwin);
	
	treeview = (GtkWidget *)lst_sch_widget_new(LIST_SCH_TYPE_DISPLAY);
	data->LV_upc = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_prepend (GTK_BOX (vbox), tbar);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(bbox), TRUE);
	gtk_box_prepend (GTK_BOX (tbar), bbox);

		widget = gtk_button_new_with_mnemonic (_("_Skip"));
		data->BT_sched_skip = widget;
		hbtk_box_prepend (GTK_BOX (bbox), widget);

		widget = gtk_button_new_with_mnemonic(_("Edit & P_ost"));
		data->BT_sched_editpost = widget;
		hbtk_box_prepend (GTK_BOX (bbox), widget);

		//TRANSLATORS: Posting a scheduled transaction is the action to materialize it into its target account.
		//TRANSLATORS: Before that action the automated transaction occurrence is pending and not yet really existing.
		widget = gtk_button_new_with_mnemonic (_("_Post"));
		data->BT_sched_post = widget;
		hbtk_box_prepend (GTK_BOX (bbox), widget);

	//info icon
	widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
	data->IM_info = widget;
	gtk_box_prepend (GTK_BOX (tbar), widget);

	//#1996505 add sum of selected
	label = make_label(NULL, 0.5, 0.5);
	gtk_widget_set_margin_end(GTK_WIDGET(label), SPACING_MEDIUM);
	data->TX_selection = label;
	hbtk_box_prepend (GTK_BOX (tbar), label);

	//#1857636 add setting to input max due date to show
	widget = hbtk_combo_box_new_with_data (NULL, CYA_FLT_SCHEDULED);
	data->CY_sched_range = widget;
	gtk_box_prepend (GTK_BOX (tbar), widget);

	//gmenu test (see test folder into gtk)
GMenu *menu, *section;
GtkWidget *image;

	menu = g_menu_new ();

	section = g_menu_new ();
	g_menu_append_section (menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("Copy to clipboard"), "actions.clipboard");
	g_menu_append (section, _("Print..."), "actions.print");
	g_object_unref (section);

	GSimpleActionGroup *group = g_simple_action_group_new ();
	data->action_group_acc = group;
	g_action_map_add_action_entries (G_ACTION_MAP (group), actions, G_N_ELEMENTS (actions), data);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(bbox), TRUE);
	gtk_box_prepend (GTK_BOX (tbar), bbox);

	widget = gtk_menu_button_new();
	gtk_box_prepend (GTK_BOX (bbox), widget);
	gtk_menu_button_set_direction (GTK_MENU_BUTTON(widget), GTK_ARROW_UP);
	gtk_widget_set_halign (widget, GTK_ALIGN_END);
	image = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_BUTTON_MENU);
	g_object_set (widget, "image", image,  NULL);

	gtk_widget_insert_action_group (widget, "actions", G_ACTION_GROUP(group));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (widget), G_MENU_MODEL (menu));


	//setup
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_sched_range), PREFS->pnl_upc_range);

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_upc)), "changed", G_CALLBACK (ui_hub_scheduled_selection_cb), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_upc), "row-activated", G_CALLBACK (ui_hub_scheduled_onRowActivated), NULL);
	
	g_signal_connect (G_OBJECT (data->BT_sched_skip), "clicked", G_CALLBACK (ui_hub_scheduled_skip_cb), data);
	g_signal_connect (G_OBJECT (data->BT_sched_editpost), "clicked", G_CALLBACK (ui_hub_scheduled_editpost_cb), data);
	g_signal_connect (G_OBJECT (data->BT_sched_post), "clicked", G_CALLBACK (ui_hub_scheduled_post_cb), data);

	g_signal_connect (data->CY_sched_range, "changed", G_CALLBACK (ui_hub_scheduled_populate), NULL);
	
	return hub;
}

