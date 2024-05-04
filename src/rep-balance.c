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


#include "homebank.h"

#include "rep-balance.h"

#include "list-operation.h"
#include "gtk-chart.h"
#include "gtk-dateentry.h"

#include "ui-account.h"
#include "dsp-mainwindow.h"
#include "ui-transaction.h"


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


/* prototypes */
static void repbalance_update_daterange(GtkWidget *widget, gpointer user_data);
static void repbalance_detail(GtkWidget *widget, gpointer user_data);
static void repbalance_sensitive(GtkWidget *widget, gpointer user_data);
static void repbalance_compute(GtkWidget *widget, gpointer user_data);

static GtkWidget *lst_repbal_create(void);
static void lst_repbal_set_cur(GtkTreeView *treeview, guint32 kcur);

//todo amiga/linux
//prev
//next


//extern gchar *CYA_REPORT_MODE[];
extern HbKvData CYA_REPORT_INTVL[];


/* action functions -------------------- */
static void repbalance_action_viewlist(GtkToolButton *toolbutton, gpointer user_data)
{
struct repbalance_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 0);
	repbalance_sensitive(data->window, NULL);
}

/*static void repbalance_action_mode_changed(GtkToolButton *toolbutton, gpointer user_data)
{
struct repbalance_data *data = user_data;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->BT_list), TRUE);
	repbalance_action_viewlist(toolbutton, data);
}
*/

static void repbalance_action_viewline(GtkToolButton *toolbutton, gpointer user_data)
{
struct repbalance_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	repbalance_sensitive(data->window, NULL);
}

static void repbalance_action_print(GtkToolButton *toolbutton, gpointer user_data)
{
struct repbalance_data *data = user_data;



	gtk_chart_print(GTK_CHART(data->RE_chart), GTK_WINDOW(data->window), NULL, NULL);

}


/* ======================== */


static void repbalance_date_change(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	DB( g_print(" \n[repbalance] date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	// set min/max date for both widget
	gtk_date_entry_set_maxdate(GTK_DATE_ENTRY(data->PO_mindate), data->filter->maxdate);
	gtk_date_entry_set_mindate(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->mindate);
	
	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPBALANCE_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPBALANCE_RANGE]);


	repbalance_compute(widget, NULL);
	repbalance_update_daterange(widget, NULL);

}


static void repbalance_update_quickdate(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	DB( g_print(" \n[repbalance] update quickdate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	g_signal_handler_block(data->PO_mindate, data->handler_id[HID_REPBALANCE_MINDATE]);
	g_signal_handler_block(data->PO_maxdate, data->handler_id[HID_REPBALANCE_MAXDATE]);
	
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);
	
	g_signal_handler_unblock(data->PO_mindate, data->handler_id[HID_REPBALANCE_MINDATE]);
	g_signal_handler_unblock(data->PO_maxdate, data->handler_id[HID_REPBALANCE_MAXDATE]);

}


static void repbalance_range_change(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gint range;

	DB( g_print(" \n[repbalance] range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_range));

	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, 0);

		repbalance_update_quickdate(widget, NULL);

		repbalance_compute(widget, NULL);
		repbalance_update_daterange(widget, NULL);
	}
}


static void repbalance_update_daterange(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gchar *daterange;

	DB( g_print(" \n[repbalance] update daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	daterange = filter_daterange_text_get(data->filter);
	gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
	g_free(daterange);
}


static void repbalance_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter iter;
guint key = -1;

	DB( g_print(" \n[repbalance] selection\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_OVER_KEY, &key, -1);
	}

	DB( g_print(" - active is %d\n", key) );

	repbalance_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
	repbalance_sensitive(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


/*
** update sensitivity
*/
static void repbalance_sensitive(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gboolean visible;
gint page;

	DB( g_print(" \n[repbalance] sensitive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	visible = page == 0 ? TRUE : FALSE;
	hb_widget_visible (data->BT_detail, visible);
	//sensitive = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), NULL, NULL);
	//gtk_action_set_sensitive(action, sensitive);

	visible = page == 0 ? FALSE : TRUE;
	hb_widget_visible (data->BT_print, visible);
	hb_widget_visible(data->LB_zoomx, visible);
	hb_widget_visible(data->RG_zoomx, visible);

}


static void repbalance_update_info(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gchar *info;
gchar   buf[128];

	DB( g_print(" \n[repbalance] update info\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	gboolean minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	hb_strfmon(buf, 127, data->minimum, data->usrkcur, minor);

	////TRANSLATORS: count of transaction in balancedrawn / count of total transaction under abalancedrawn amount threshold
	info = g_strdup_printf(_("%d/%d under %s"), data->nbovrdraft, data->nbope, buf);
	gtk_label_set_text(GTK_LABEL(data->TX_info), info);
	g_free(info);
	
}


static void repbalance_detail(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
guint active = GPOINTER_TO_INT(user_data);
guint tmpintvl;
GList *list;
GtkTreeModel *model;
GtkTreeIter  iter, child;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" \n[repbalance] detail\n") );

	tmpintvl = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_intvl));
	
	/* clear and detach our model */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

	if(data->detail)
	{
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), NULL); /* Detach model from view */

		/* fill in the model */
		list = g_list_first(data->ope_list);
		while (list != NULL)
		{
		Transaction *ope = list->data;
		guint i, pos;

			if( (ope->date >= data->filter->mindate) && (ope->date <= data->filter->maxdate) )
			{
				//#1907699 date is wrong
				//pos = report_interval_get_pos(tmpintvl, data->jbasedate, ope);
				//#1970020 but with #1958001 fix, it is
				pos = report_interval_get_pos(tmpintvl, data->filter->mindate, ope);

				DB( g_print(" get '%s', pos=%d act=%d\n", ope->memo, pos, active) );

				//filter here
				//if( pos == active && (ope->kacc == acckey || selectall) )
				if( pos == active )
				{
					DB( g_print(" insert\n") );
					
					gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), 
						&iter, NULL, -1,
						MODEL_TXN_POINTER, ope,
						MODEL_TXN_SPLITAMT, ope->amount,
						-1);

					//#1875801 show split detail
					if( ope->flags & OF_SPLIT )
					{
					guint nbsplit = da_splits_length(ope->splits);

						for(i=0;i<nbsplit;i++)
						{
						Split *split = da_splits_get(ope->splits, i);
						
							gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &child, &iter, -1,
								MODEL_TXN_POINTER, ope,
								MODEL_TXN_SPLITPTR, split,
								-1);
						}
					}

				}
			}
			list = g_list_next(list);
		}

		/* Re-attach model to view */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), model);
		g_object_unref(model);

		gtk_tree_view_columns_autosize( GTK_TREE_VIEW(data->LV_detail) );

	}
}


static gboolean
repbalance_cb_acc_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
struct repbalance_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(label), GTK_TYPE_WINDOW)), "inst_data");

	ui_acc_listview_quick_select(GTK_TREE_VIEW(data->LV_acc), uri);

	repbalance_compute (GTK_WIDGET(data->LV_acc), NULL);

    return TRUE;
}



static void
repbalance_cb_acc_changed(GtkCellRendererToggle *cell, gchar *path_str, gpointer user_data)
{
struct repbalance_data *data = user_data;

	repbalance_compute (GTK_WIDGET(data->LV_acc), NULL);
}


static void repbalance_detail_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
struct repbalance_data *data;
Transaction *active_txn;
gboolean result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print ("\n[repbalance] A detail row has been double-clicked!\n") );

	active_txn = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_detail));
	if(active_txn)
	{
	Transaction *old_txn, *new_txn;

		//#1909749 skip reconciled if lock is ON
		if( PREFS->lockreconciled == TRUE && active_txn->status == TXN_STATUS_RECONCILED )
			return;

		old_txn = da_transaction_clone (active_txn);
		new_txn = active_txn;
		result = deftransaction_external_edit(GTK_WINDOW(data->window), old_txn, new_txn);

		if(result == GTK_RESPONSE_ACCEPT)
		{
		GtkTreeSelection *treeselection;
		GtkTreeModel *model;
		GtkTreeIter iter;
		GtkTreePath *path = NULL;

			//1936806 keep the selection
			treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->LV_report));
			if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
			{
				path = gtk_tree_model_get_path(model, &iter);
			}

			//#1640885
			GLOBALS->changes_count++;
			repbalance_compute (data->window, NULL);

			if( path != NULL )
			{
				gtk_tree_selection_select_path(treeselection, path);
				gtk_tree_path_free(path);
			}

		}

		da_transaction_free (old_txn);
	}
}



static void repbalance_update_detail(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if(data->detail)
	{
	GtkTreeSelection *treeselection;
	GtkTreeModel *model;
	GtkTreeIter iter;
	guint key;

		treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->LV_report));

		if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
		{
			gtk_tree_model_get(model, &iter, LST_OVER_KEY, &key, -1);

			DB( g_print(" - active is %d\n", key) );

			repbalance_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
		}



		gtk_widget_show(data->GR_detail);
	}
	else
		gtk_widget_hide(data->GR_detail);
}


static void repbalance_toggle_detail(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->detail ^= 1;

	DB( g_print(" \n[repbalance] toggledetail to %d\n", (int)data->detail) );

	repbalance_update_detail(widget, user_data);

}


static void repbalance_zoomx_callback(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gdouble value;

	DB( g_print("(statistic) zoomx\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	value = gtk_range_get_value(GTK_RANGE(data->RG_zoomx));

	DB( g_print(" + scale is %f\n", value) );

	gtk_chart_set_barw(GTK_CHART(data->RE_chart), value);

}


static void repbalance_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	DB( g_print(" \n[repbalance] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	repbalance_update_info(widget,NULL);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	gtk_chart_show_minor(GTK_CHART(data->RE_chart), GLOBALS->minor);
	gtk_chart_queue_redraw(GTK_CHART(data->RE_chart));
}


static void repbalance_compute(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gint tmpintvl;
gboolean showempty;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
guint32 i, jmindate, jmaxdate, maxkacc;
gdouble trn_amount;
GtkTreeModel *model;
GtkTreeIter  iter;
//Account *acc;
gint usrnbacc, usrrange;

	DB( g_print(" \n[repbalance] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//get kacc array
	maxkacc = da_acc_get_max_key();
	data->tmp_acckeys = g_malloc0((maxkacc+2) * sizeof(guint32));

	//reset our data
	data->nbope = 0;
	data->nbovrdraft = 0;
	data->firstbalance = 0.0;
	
	//TODO: use a Queue here
	g_list_free(data->ope_list);
	data->ope_list = NULL;

	jmindate = 0;
	jmaxdate = 0;

	//grab user selection
	tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_intvl));
	usrrange  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_range));
	showempty = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_showempty));
	usrnbacc  = ui_acc_listview_fill_keys(GTK_TREE_VIEW(data->LV_acc), data->tmp_acckeys, &data->usrkcur);

	DB( g_print(" usr: nbacc:%d, kcur:%d, empty:%d\n\n", usrnbacc, data->usrkcur, showempty) );

	// clear everything
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	gtk_list_store_clear (GTK_LIST_STORE(model));
	gtk_chart_set_datas_none(GTK_CHART(data->RE_chart));
	
	if( (usrnbacc == 0) )
		goto end;

	//grab data from user selected account
	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		// avoid unselected and noreport (maybe already filtered into list)
		if( (acc->flags & (AF_NOREPORT)) || (data->tmp_acckeys[acc->key] == 0) )
			goto next_acc;

		// set minimum (only used when 1 account is selected)
		data->minimum = acc->minimum;

		// if more than 1 cur get amount as base currency
		trn_amount = 0.0;
		if( (data->usrkcur != acc->kcur) )
			trn_amount = hb_amount_base(acc->initial, acc->kcur);
		else
			trn_amount = acc->initial;

		data->firstbalance += trn_amount;
		DB( g_print(" - stored initial %.2f for account %d:%s\n", trn_amount, acc->key, acc->name) );

		// get mindate
		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		if(lnk_txn)
		{
		Transaction *txn = lnk_txn->data;
			if(jmindate == 0)
				jmindate = txn->date;
			else
				jmindate = MIN(jmindate, txn->date);
		}

		// get maxdate
		lnk_txn = g_queue_peek_tail_link(acc->txn_queue);
		if(lnk_txn)
		{
		Transaction *txn = lnk_txn->data;
			if(jmaxdate == 0)
				jmaxdate = txn->date;
			else
				jmaxdate = MAX(jmaxdate, txn->date);
		}
		DB( g_print(" mindate: %d maxdate: %d\n", jmindate, jmaxdate) );
		
		//collect every txn for account
		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			//5.5 forgot to filter...
			if( !((txn->status == TXN_STATUS_REMIND) || (txn->status == TXN_STATUS_VOID)) )
			{
				//5.3 append is damn slow
				//list = g_list_append(list, txn);
				data->ope_list = g_list_prepend(data->ope_list, txn);
			}
			lnk_txn = g_list_next(lnk_txn);
		}

	next_acc:
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);


	if(g_list_length(data->ope_list) == 0)
		goto end;

	// update the date range if alldate is selected
	if( (usrrange == FLT_RANGE_MISC_ALLDATE) )
	{
		data->filter->mindate = jmindate;
		data->filter->maxdate = jmaxdate;
		repbalance_update_quickdate(widget, NULL);
	}
	
	// grab nb result and allocate memory
	data->jbasedate = jmindate;
	//#1958001
	data->n_result = report_interval_count(tmpintvl, data->filter->mindate, data->filter->maxdate);
	data->tmp_income  = g_malloc0((data->n_result+2) * sizeof(gdouble));
	data->tmp_expense = g_malloc0((data->n_result+2) * sizeof(gdouble));

	DB( g_print(" %d days in selection\n", data->filter->maxdate - data->filter->mindate));

	DB( hb_print_date(data->filter->mindate, "min") );
	DB( hb_print_date(data->filter->maxdate, "max") );

	if(data->tmp_income && data->tmp_expense)
	{
	GList *list;
	gdouble trn_amount;

		/* sort by date & compute the balance */
		data->ope_list = da_transaction_sort (data->ope_list);
		list = g_list_first(data->ope_list);
		while (list != NULL)
		{
		Transaction *ope = list->data;

			//if(selkey == ope->kacc || selectall == TRUE)
			if( (data->tmp_acckeys[ope->kacc] == TRUE) )
			{
				// if more than 1 cur get amount as base currency
				if( (data->usrkcur != ope->kcur) )
					trn_amount = hb_amount_base(ope->amount, ope->kcur);
				else
					trn_amount = ope->amount;

				// cumulate pre-date range balance
				if( (ope->date < data->filter->mindate) )
					data->firstbalance += trn_amount;

				if( (ope->date >= data->filter->mindate) && (ope->date <= data->filter->maxdate) )
				{
				gint pos = report_interval_get_pos(tmpintvl, data->filter->mindate, ope);

					//deal with transactions
					if(trn_amount < 0)
						data->tmp_expense[pos] += trn_amount;
					else
						data->tmp_income[pos] += trn_amount;
				}
			}

			list = g_list_next(list);
		}
	}


	// set currency
	lst_repbal_set_cur(GTK_TREE_VIEW(data->LV_report), data->usrkcur);
	gtk_chart_set_currency(GTK_CHART(data->RE_chart), data->usrkcur);

	// ref and detach model for speed
	g_object_ref(model); 
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), NULL);


	DB( g_print(" inserting %d results\n", data->n_result) );

	gdouble balance = data->firstbalance;
	//#1958001
	for(i=0;i<data->n_result;i++)
	{
	gboolean is_ovrdraft = FALSE;
	gchar intvlname[64];

		balance += data->tmp_expense[i];
		balance += data->tmp_income[i];

		if( !showempty && (data->tmp_expense[i] == 0 && data->tmp_income[i] == 0) )
			continue;

		if(usrnbacc == 1)
			is_ovrdraft = (balance < data->minimum) ? TRUE : FALSE;

		//#1907699 date is wrong
		report_interval_snprint_name(intvlname, sizeof(intvlname)-1, tmpintvl, data->filter->mindate, i);
		//report_interval_snprint_name(intvlname, sizeof(intvlname)-1, tmpintvl, jmindate, i);

		DB( g_print(" %3d: %s %f %f\n", i, intvlname, data->tmp_expense[i], data->tmp_income[i]) );
		
		/* column 0: pos (gint) */
		/* not used: column 1: key (gint) */
		/* column 2: name (gchar) */
		/* column x: values (double) */

		gtk_list_store_insert_with_values (GTK_LIST_STORE(model), 
			&iter, -1,
			LST_OVER_OVER, is_ovrdraft,
			//LST_OVER_DATE, posdate,
			LST_OVER_KEY, i,
			LST_OVER_DATESTR, intvlname,
			LST_OVER_EXPENSE, data->tmp_expense[i],
			LST_OVER_INCOME , data->tmp_income[i],
			LST_OVER_BALANCE, balance,
			-1);

		data->nbope++;
		if(is_ovrdraft == TRUE)
			data->nbovrdraft++;
	}

	gboolean visible = (usrnbacc > 1) ? FALSE : TRUE;
	gtk_widget_set_visible (GTK_WIDGET(data->TX_info), visible);
	gtk_chart_show_overdrawn(GTK_CHART(data->RE_chart), visible);

	if( visible == TRUE )
	{
		repbalance_update_info(widget, NULL);
		gtk_chart_set_overdrawn(GTK_CHART(data->RE_chart), data->minimum);
	}

	/* Re-attach model to view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
	g_object_unref(model);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	/* update bar chart */
	gtk_chart_set_datas(GTK_CHART(data->RE_chart), model, LST_OVER_BALANCE, NULL, NULL);
	//gtk_chart_set_line_datas(GTK_CHART(data->RE_chart), model, LST_OVER_BALANCE, LST_OVER_DATE);


end:
	g_free(data->tmp_expense);
	g_free(data->tmp_income);
	g_free(data->tmp_acckeys);

	data->tmp_expense = NULL;
	data->tmp_income  = NULL;
	data->tmp_acckeys = NULL;
}


static GtkWidget *
repbalance_toolbar_create(struct repbalance_data *data)
{
GtkWidget *toolbar, *button;

	toolbar = gtk_toolbar_new();

	button = (GtkWidget *)gtk_radio_tool_button_new(NULL);
	data->BT_list = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_LIST, "label", _("List"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as list"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_line = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_LINE, "label", _("Line"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as lines"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	button = gtk_widget_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
		"icon-name", ICONNAME_HB_OPE_SHOW,
		"label", _("Detail"),
		"tooltip-text", _("Toggle detail"),
		NULL);
	data->BT_detail = button;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REFRESH, _("Refresh"), _("Refresh results"));
	data->BT_refresh = button;

	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_PRINT, _("Print"), _("Print"));
	data->BT_print = button;
	
	return toolbar;
}


//reset the filter
static void repbalance_filter_setup(struct repbalance_data *data)
{
	DB( g_print("\n[repbalance] reset filter\n") );

	filter_reset(data->filter);
	filter_preset_daterange_set(data->filter, PREFS->date_range_rep, 0);
	
}


static void repbalance_window_setup(struct repbalance_data *data, guint32 accnum)
{
	DB( g_print(" \n[repbalance] setup\n") );

	DB( g_print(" init data\n") );

	repbalance_filter_setup(data);


	DB( g_print(" populate\n") );
	
	ui_acc_listview_populate(data->LV_acc, ACC_LST_INSERT_REPORT);

	DB( g_print(" set widgets default\n") );

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor),GLOBALS->minor);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), PREFS->date_range_rep);

	//5.6 set default account
	if(!accnum)
	{
		accnum = da_acc_get_first_key();
	}
	ui_acc_listview_set_active(GTK_TREE_VIEW(data->LV_acc), accnum);

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);

	gtk_chart_show_legend(GTK_CHART(data->RE_chart), FALSE, FALSE);
	gtk_chart_show_xval(GTK_CHART(data->RE_chart), TRUE);
	gtk_chart_set_smallfont (GTK_CHART(data->RE_chart), PREFS->rep_smallfont);

	DB( g_print(" connect widgets signals\n") );

	g_signal_connect (data->CM_minor, "toggled", G_CALLBACK (repbalance_toggle_minor), NULL);

	g_signal_connect (data->CY_intvl, "changed", G_CALLBACK (repbalance_compute), (gpointer)data);

    data->handler_id[HID_REPBALANCE_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (repbalance_date_change), (gpointer)data);
    data->handler_id[HID_REPBALANCE_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (repbalance_date_change), (gpointer)data);

	data->handler_id[HID_REPBALANCE_RANGE] = g_signal_connect (data->CY_range, "changed", G_CALLBACK (repbalance_range_change), NULL);


	//hbtk_radio_button_connect (GTK_CONTAINER(data->RA_mode), "toggled", G_CALLBACK (repbalance_action_mode_changed), (gpointer)data);

	// acc treeview
	g_signal_connect (data->BT_all, "activate-link", G_CALLBACK (repbalance_cb_acc_activate_link), NULL);
	g_signal_connect (data->BT_non, "activate-link", G_CALLBACK (repbalance_cb_acc_activate_link), NULL);
	g_signal_connect (data->BT_inv, "activate-link", G_CALLBACK (repbalance_cb_acc_activate_link), NULL);


	//item filter
	GtkCellRendererToggle *renderer;

	renderer = g_object_get_data(G_OBJECT(data->LV_acc), "togrdr_data");
	g_signal_connect_after (G_OBJECT(renderer), "toggled", G_CALLBACK (repbalance_cb_acc_changed), (gpointer)data);

	//g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), "changed", G_CALLBACK (repbalance_cb_acc_changed), NULL);


	g_signal_connect (data->CM_showempty, "toggled", G_CALLBACK (repbalance_compute), NULL);

	g_signal_connect (data->RG_zoomx, "value-changed", G_CALLBACK (repbalance_zoomx_callback), NULL);

	g_signal_connect (G_OBJECT (data->BT_list), "clicked", G_CALLBACK (repbalance_action_viewlist), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_line), "clicked", G_CALLBACK (repbalance_action_viewline), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_detail), "clicked", G_CALLBACK (repbalance_toggle_detail), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_refresh), "clicked", G_CALLBACK (repbalance_compute), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_print), "clicked", G_CALLBACK (repbalance_action_print), (gpointer)data);
	
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), "changed", G_CALLBACK (repbalance_selection), NULL);
	
	g_signal_connect (GTK_TREE_VIEW(data->LV_detail), "row-activated", G_CALLBACK (repbalance_detail_onRowActivated), NULL);

}


static gboolean repbalance_window_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbalance_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbalance] window mapped\n") );

	//setup, init and show window
	repbalance_window_setup(data, data->accnum);
	repbalance_update_quickdate(data->window, NULL);
	repbalance_compute(data->window, NULL);

	return FALSE;
}


static gboolean repbalance_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbalance_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print(" \n[repbalance] dispose\n") );

	g_list_free (data->ope_list);

	da_flt_free(data->filter);

	g_free(data);

	//store position and size
	wg = &PREFS->ove_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);

	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d\n", wg->l, wg->t, wg->w, wg->h) );

	//enable define windows
	GLOBALS->define_off--;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	//unref window to our open window list
	GLOBALS->openwindows = g_slist_remove(GLOBALS->openwindows, widget);

	return FALSE;
}


//allocate our object/memory
static void repbalance_window_acquire(struct repbalance_data *data)
{
	DB( g_print("\n[repbalance] acquire\n") );

	data->ope_list = NULL;
	data->filter = da_flt_malloc();
	data->detail = 0;
	
}


// the window creation
GtkWidget *repbalance_window_new(gint accnum)
{
struct repbalance_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainvbox, *hbox, *vbox, *notebook, *sw;
GtkWidget *label, *widget, *table, *treebox, *treeview;
gint row;

	DB( g_print(" \n[repbalance] new\n") );

	data = g_malloc0(sizeof(struct repbalance_data));
	if(!data) return NULL;

	data->accnum = accnum;
	repbalance_window_acquire (data);

	//disable define windows
	GLOBALS->define_off++;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

    /* create window, etc */
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	data->window = window;
	//ref window to our open window list
	GLOBALS->openwindows = g_slist_prepend(GLOBALS->openwindows, window);


	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" - new window=%p, inst_data=%p\n", window, data) );

	gtk_window_set_title (GTK_WINDOW (window), _("Balance report"));

	//window contents
	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add (GTK_CONTAINER (window), mainvbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_box_pack_start (GTK_BOX (mainvbox), hbox, TRUE, TRUE, 0);

	//control part
	table = gtk_grid_new ();
	gtk_widget_set_hexpand (GTK_WIDGET(table), FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), table, FALSE, FALSE, 0);

	gtk_container_set_border_width (GTK_CONTAINER (table), SPACING_SMALL);
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);


	row = 0;
	label = make_label_group(_("Display"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	/*row++;
	label = make_label_widget(_("Mode:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_radio_button_new(GTK_ORIENTATION_HORIZONTAL, CYA_REPORT_MODE, TRUE);
	data->RA_mode = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);
	*/

	//5.5
	row++;
	label = make_label_widget(_("Inter_val:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	//widget = make_cycle(label, CYA_REPORT_INTVL);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_INTVL);
	data->CY_intvl = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Show empty line"));
	data->CM_showempty = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_Zoom X:"));
	data->LB_zoomx = label;
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = make_scale(label);
	data->RG_zoomx = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	row++;
	label = make_label_group(_("Date filter"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->CY_range = make_daterange(label, DATE_RANGE_CUSTOM_DISABLE);
	gtk_grid_attach (GTK_GRID (table), data->CY_range, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_From:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->PO_mindate = gtk_date_entry_new(label);
	gtk_grid_attach (GTK_GRID (table), data->PO_mindate, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_To:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->PO_maxdate = gtk_date_entry_new(label);
	gtk_grid_attach (GTK_GRID (table), data->PO_maxdate, 2, row, 1, 1);

	row++;
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	row++;
	label = make_label_group(_("Account filter"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	treebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	
	gtk_grid_attach (GTK_GRID (table), treebox, 1, row, 2, 1);

		label = make_label (_("Select:"), 0, 0.5);
		gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);

		label = make_clicklabel("all", _("All"));
		data->BT_all = label;
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);
		
		label = make_clicklabel("non", _("None"));
		data->BT_non = label;
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);

		label = make_clicklabel("inv", _("Invert"));
		data->BT_inv = label;
		gtk_box_pack_start (GTK_BOX (treebox), label, FALSE, FALSE, 0);


	row++;
 	sw = gtk_scrolled_window_new(NULL,NULL);
 	data->SW_acc = sw;
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(sw), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_widget_set_margin_bottom (sw, SPACING_LARGE);
	widget = ui_acc_listview_new(TRUE);
	data->LV_acc = widget;
	gtk_widget_set_vexpand (widget, TRUE);
	gtk_widget_set_size_request(data->LV_acc, HB_MINWIDTH_LIST, -1);
	gtk_container_add(GTK_CONTAINER(sw), data->LV_acc);
	gtk_grid_attach (GTK_GRID (table), sw, 1, row, 2, 1);


	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);


	//toolbar
	widget = repbalance_toolbar_create(data);
	data->TB_bar = widget; 
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

	
	//infos
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER(hbox), SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	widget = make_label(NULL, 0.5, 0.5);
	gimp_label_set_attributes (GTK_LABEL (widget), PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL, -1);
	data->TX_daterange = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	label = gtk_label_new(NULL);
	data->TX_info = label;
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);


	notebook = gtk_notebook_new();
	data->GR_result = notebook;
	gtk_widget_show(notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

	//page: list
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, NULL);

	widget = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	treeview = lst_repbal_create();
	data->LV_report = treeview;
	gtk_container_add (GTK_CONTAINER(widget), treeview);
	//gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);
	gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

	//detail
	widget = gtk_scrolled_window_new (NULL, NULL);
	data->GR_detail = widget;
	//gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW (widget), GTK_CORNER_TOP_RIGHT);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = create_list_transaction(LIST_TXN_TYPE_DETAIL, PREFS->lst_det_columns);
	data->LV_detail = treeview;
	gtk_container_add (GTK_CONTAINER(widget), treeview);

    gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);

	list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);

	//page: 2d lines
	widget = gtk_chart_new(CHART_TYPE_LINE);
	data->RE_chart = widget;
	//gtk_chart_set_minor_prefs(GTK_CHART(widget), PREFS->euro_value, PREFS->minor_cur.suffix_symbol);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);


	// connect dialog signals
    g_signal_connect (window, "delete-event", G_CALLBACK (repbalance_window_dispose), (gpointer)data);
	g_signal_connect (window, "map-event"   , G_CALLBACK (repbalance_window_mapped), NULL);

	// setup, init and show window
	wg = &PREFS->ove_wg;
	if( wg->l && wg->t )
		gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
	gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);

	// toolbar	
	if(PREFS->toolbar_style == 0)
		gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_bar));
	else
		gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_bar), PREFS->toolbar_style-1);

	gtk_widget_show_all (window);

	//minor ?
	hb_widget_visible(data->CM_minor, PREFS->euro_active);
	repbalance_sensitive(window, NULL);
	repbalance_update_detail(window, NULL);

	return(window);
}


/*
** ============================================================================
*/


static void lst_repbal_cell_data_function_date (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gchar *datestr;
gboolean is_ovrdraft;
gchar *color;
gint weight;

	gtk_tree_model_get(model, iter,
		LST_OVER_DATESTR, &datestr,
		LST_OVER_OVER, &is_ovrdraft,
		-1);

	color = NULL;
	weight = PANGO_WEIGHT_NORMAL;

	if(is_ovrdraft==TRUE)
	{
		if(PREFS->custom_colors == TRUE)
			color = PREFS->color_warn;

		weight = PANGO_WEIGHT_BOLD;
	}

	g_object_set(renderer,
		"weight", weight,
		"foreground",  color,
		"text", datestr,
		NULL);

	//leak
	g_free(datestr);
}


static void lst_repbal_cell_cell_data_function_amount (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gdouble  value;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gboolean is_ovrdraft;
gchar *color;
//gint weight;
guint32 kcur = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gtk_tree_view_column_get_tree_view(col)), "kcur_data"));


	//get datas
	gtk_tree_model_get(model, iter,
		LST_OVER_OVER, &is_ovrdraft,
		GPOINTER_TO_INT(user_data), &value,
		-1);

	//fix: 400483
	if( value == 0.0 )
		g_object_set(renderer, "text", NULL, NULL);
	else
	{
		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, kcur, GLOBALS->minor);

		color = NULL;
		//weight = PANGO_WEIGHT_NORMAL;

		if(value != 0.0 && PREFS->custom_colors == TRUE)
			color = (value > 0.0) ? PREFS->color_inc : PREFS->color_exp;

		if(is_ovrdraft==TRUE)
		{
			if(PREFS->custom_colors == TRUE)
				color = PREFS->color_warn;

			//weight = PANGO_WEIGHT_BOLD;
		}

		g_object_set(renderer,
			//"weight", weight,
			"foreground",  color,
			"text", buf,
			NULL);
	}

}

static GtkTreeViewColumn *lst_repbal_column_amount_create(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbal_cell_cell_data_function_amount, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}


static void lst_repbal_set_cur(GtkTreeView *treeview, guint32 kcur)
{
	g_object_set_data(G_OBJECT(treeview), "kcur_data", GUINT_TO_POINTER(kcur));
}


/*
** create our statistic list
*/
static GtkWidget *lst_repbal_create(void)
{
GtkListStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	/* create list store */
	store = gtk_list_store_new(
	  	NUM_LST_OVER,
		G_TYPE_BOOLEAN,
		G_TYPE_INT,
		G_TYPE_STRING,
		G_TYPE_DOUBLE,
		G_TYPE_DOUBLE,
		G_TYPE_DOUBLE
		);

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (view), PREFS->grid_lines);

	/* column debug balance */
/*
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "debug balance");
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", LST_OVER_OVER);
*/

	/* column date */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Date"));
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);
	renderer = gtk_cell_renderer_text_new();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_OVER_DATE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbal_cell_data_function_date, NULL, NULL);


	/* column: Expense */
	column = lst_repbal_column_amount_create(_("Expense"), LST_OVER_EXPENSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Income */
	column = lst_repbal_column_amount_create(_("Income"), LST_OVER_INCOME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Balance */
	column = lst_repbal_column_amount_create(_("Balance"), LST_OVER_BALANCE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

  /* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);



	return(view);
}
