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

#include "rep-balance.h"

#include "list-operation.h"
#include "gtk-chart.h"
#include "gtk-dateentry.h"

#include "ui-account.h"
#include "dsp-mainwindow.h"
#include "ui-transaction.h"

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


/* prototypes */
static GtkWidget *lst_repbal_create(void);
static void lst_repbal_set_cur(GtkTreeView *treeview, guint32 kcur);
static GString *lst_repbal_to_string(ToStringMode mode, GtkTreeView *treeview, gchar *title);


//extern gchar *CYA_REPORT_MODE[];
extern HbKvData CYA_REPORT_INTVL[];


/* = = = = = = = = = = = = = = = = */


static gchar *
repbalance_compute_title(gint intvl)
{
gchar *title;

	//TRANSLATORS: example 'Balance by Month'
	title = g_strdup_printf(_("Balance by %s"), hbtk_get_label(CYA_REPORT_INTVL, intvl) );
	return title;
}


static void repbalance_sensitive(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gboolean visible;
gint page;

	DB( g_print(" \n[rep-bal] sensitive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	visible = page == 0 ? TRUE : FALSE;
	hb_widget_visible (data->BT_detail, visible);
	//sensitive = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), NULL, NULL);
	//gtk_action_set_sensitive(action, sensitive);

	visible = page == 0 ? FALSE : TRUE;
	//5.7
	//hb_widget_visible (data->BT_print, visible);
	hb_widget_visible(data->LB_zoomx, visible);
	hb_widget_visible(data->RG_zoomx, visible);

}



static void repbalance_update_daterange(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gchar *daterange;

	DB( g_print("\n[rep-bal] update daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	daterange = filter_daterange_text_get(data->filter);
	gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
	g_free(daterange);
}


static void repbalance_update_info(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gchar *info;
gchar   buf[128];

	DB( g_print("\n[rep-bal] update info\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	gboolean minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	hb_strfmon(buf, 127, data->minimum, data->usrkcur, minor);

	////TRANSLATORS: count of transaction in overdrawn / count of total transaction under overdrawn amount threshold
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

	DB( g_print("\n[rep-bal] detail\n") );

	tmpintvl = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));

	/* clear and detach our model */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

	if(data->detail && data->txn_queue)
	{
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), NULL); /* Detach model from view */

		/* fill in the model */
		list = g_queue_peek_head_link(data->txn_queue);
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


static guint32
_data_compute_currency(struct repbalance_data *data)
{
GList *lst_acc, *lnk_acc;
guint32 lastcurr = GLOBALS->kcur;
guint count = 0;

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *accitem = lnk_acc->data;

		if( da_flt_status_acc_get(data->filter, accitem->key) == TRUE ) 
		{
			if( count == 0 )
				lastcurr = accitem->kcur;
			else if( accitem->kcur != lastcurr )
			{
				lastcurr = GLOBALS->kcur;
				goto end;
			}
			count++;
		}
		lnk_acc = g_list_next(lnk_acc);
	}
end:
	g_list_free(lst_acc);
	return lastcurr;
}


static void
_data_collect_txn(struct repbalance_data *data)
{
GList *lst_acc, *lnk_acc;
GList *lnk_txn;
gboolean inclxfer;
guint usrnbacc;

	DB( g_print("\n- - - - - - - -\n[rep-bal] collect txn\n") );

	//clear all
	if(data->txn_queue != NULL)
		g_queue_free (data->txn_queue);
	data->txn_queue = NULL;
	data->firstbalance = 0.0;

	//#2019876 return is invalid date range
	if( data->filter->maxdate < data->filter->mindate )
		return;

	//grab user selection
	inclxfer  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_inclxfer));



	//as it is not the filter dialog, count
	//todo, we could count into filter other func
	da_flt_count_item(data->filter);

	usrnbacc = data->filter->n_item[FLT_GRP_ACCOUNT];

	//todo: find another way to define mono/multi currency
	data->usrkcur = _data_compute_currency(data);


	if(usrnbacc == 1)
		inclxfer = TRUE;

	//5.8 fake filter
	filter_preset_type_set(data->filter, FLT_TYPE_ALL, FLT_OFF);
	if( inclxfer == FALSE )
		filter_preset_type_set(data->filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);


	DB( g_print(" usr: n_acc=%d, kcur=%d, incxfer=%d\n", usrnbacc, data->usrkcur, inclxfer) );

	if( (usrnbacc == 0) )
		goto end;



//collect
	DB( g_print(" -- collect txn\n") );

	data->txn_queue = g_queue_new ();

	//grab data from user selected account
	//+ compute sum of initialbalance
	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;
	gdouble amount = 0.0;

		DB( g_print(" eval acc=%d '%s' >> fltincl=%d\n", acc->key, acc->name, da_flt_status_acc_get(data->filter, acc->key) ) );

		// avoid unselected and noreport (maybe already filtered into list)
		if( (acc->flags & (AF_NOREPORT)) || (da_flt_status_acc_get(data->filter, acc->key) == FALSE) )
			goto next_acc;

		// set minimum (only used when 1 account is selected)
		data->minimum = acc->minimum;

		// if more than 1 cur get amount as base currency
		if( (data->usrkcur != acc->kcur) )
			amount = hb_amount_base(acc->initial, acc->kcur);
		else
			amount = acc->initial;

		data->firstbalance += amount;
		DB( g_print(" - stored initial %.2f for account %d:%s\n", amount, acc->key, acc->name) );

		//collect every txn for account
		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *txn = lnk_txn->data;

			//5.5 forgot to filter...
			//#1886123 include remind based on user prefs
			if( !transaction_is_balanceable(txn) )
				goto next_txn;

			//#2045514 xclude xfer from selected account
			//if( usrnbacc > 1 && da_flt_status_acc_get(data->filter, txn->kacc) != 0 && data->tmp_acckeys[txn->kxferacc] != 0 )
			if( (inclxfer == FALSE) 
			 && (da_flt_status_acc_get(data->filter, txn->kacc) == TRUE )
			 && (da_flt_status_acc_get(data->filter, txn->kxferacc) == TRUE)
			  )
				goto next_txn;

			//#2104162 compute 1stbalance here
			// if more than 1 cur get amount as base currency
			if( (data->usrkcur != txn->kcur) )
				amount = hb_amount_base(txn->amount, txn->kcur);
			else
				amount = txn->amount;

			// cumulate pre-date range balance
			if( (txn->date < data->filter->mindate) )
				data->firstbalance += amount;

			g_queue_push_head (data->txn_queue, txn);

		next_txn:
			lnk_txn = g_list_next(lnk_txn);
		}

	next_acc:
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);


	end:

	DB( g_print(" - first balance %.2f\n", data->firstbalance) );

}


static void repbalance_compute(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gint tmpintvl;
gboolean showempty;
guint32 i;

GtkTreeModel *model;
GtkTreeIter  iter;
//Account *acc;
gint usrnbacc;

	DB( g_print("\n- - - - - - - -\n[rep-bal] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//clear all
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	gtk_list_store_clear (GTK_LIST_STORE(model));
	gtk_chart_set_datas_none(GTK_CHART(data->RE_chart));

	//reset our data
	data->nbope = 0;
	data->nbovrdraft = 0;
	repbalance_update_info(widget, user_data);


	//test 5.8
	da_flt_count_item(data->filter);
	usrnbacc = data->filter->n_item[FLT_GRP_ACCOUNT];
	gchar *txt = filter_text_summary_get(data->filter);
	ui_label_set_integer(GTK_LABEL(data->TX_fltactive), data->filter->n_active);
	gtk_widget_set_tooltip_text(data->TT_fltactive, txt);
	g_free(txt);



//compute

	if( (!data->txn_queue) || g_queue_get_length(data->txn_queue) == 0)
		goto end;

	DB( g_print(" -- compute\n") );

	//grab user selection
	tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
	showempty = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_showempty));


	// grab nb result and allocate memory
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
	gdouble amount;

		/* sort by date & compute the balance */
		list = g_queue_peek_head_link(data->txn_queue);
		while (list != NULL)
		{
		Transaction *ope = list->data;

			if( (da_flt_status_acc_get(data->filter, ope->kacc) == TRUE) )
			{
				// if more than 1 cur get amount as base currency
				if( (data->usrkcur != ope->kcur) )
					amount = hb_amount_base(ope->amount, ope->kcur);
				else
					amount = ope->amount;

				//check: this should be useless as filtered in collect_txn
				if( (ope->date >= data->filter->mindate) && (ope->date <= data->filter->maxdate) )
				{
				gint pos = report_interval_get_pos(tmpintvl, data->filter->mindate, ope);

					//deal with transactions
					if(amount < 0)
						data->tmp_expense[pos] += amount;
					else
						data->tmp_income[pos] += amount;
				}
			}

			list = g_list_next(list);
		}
	}


	// set currency
	lst_repbal_set_cur(GTK_TREE_VIEW(data->LV_report), data->usrkcur);
	gtk_chart_set_currency(GTK_CHART(data->RE_chart), data->usrkcur);

//populate
	DB( g_print(" -- populate\n") );

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
			LST_OVER_POS, i,
			LST_OVER_KEY, i,
			LST_OVER_LABEL, intvlname,
			LST_OVER_EXPENSE, data->tmp_expense[i],
			LST_OVER_INCOME , data->tmp_income[i],
			LST_OVER_TOTAL, balance,
			LST_OVER_FLAGS, (is_ovrdraft == TRUE) ? REPORT_FLAG_OVER : 0,
			-1);

		data->nbope++;
		if(is_ovrdraft == TRUE)
			data->nbovrdraft++;
	}

	gboolean visible = (usrnbacc > 1) ? FALSE : TRUE;
	gtk_widget_set_visible (GTK_WIDGET(data->CM_inclxfer), !visible);
	gtk_widget_set_visible (GTK_WIDGET(data->TX_info), visible);
	gtk_chart_show_overdrawn(GTK_CHART(data->RE_chart), visible);

	if( visible == TRUE )
	{
		repbalance_update_info(widget, NULL);
		gtk_chart_set_overdrawn(GTK_CHART(data->RE_chart), data->minimum);
	}

	//5.8.6 update column 0 title
	GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report), 0);
	if(column)
		gtk_tree_view_column_set_title(column, hbtk_get_label(CYA_REPORT_INTVL, tmpintvl) );


	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	/* Re-attach model to view */
	gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
	g_object_unref(model);

	/* update bar chart */
	gchar *title = repbalance_compute_title(tmpintvl); 
	gtk_chart_set_datas_total(GTK_CHART(data->RE_chart), model, LST_OVER_TOTAL, LST_OVER_TOTAL, title, NULL);
	//gtk_chart_set_line_datas(GTK_CHART(data->RE_chart), model, LST_OVER_TOTAL, LST_OVER_DATE);
	g_free(title);

end:
	g_free(data->tmp_expense);
	g_free(data->tmp_income);

	data->tmp_expense = NULL;
	data->tmp_income  = NULL;
}


//reset the filter
static void repbalance_filter_setup(struct repbalance_data *data)
{
guint32 accnum;

	DB( g_print("\n[rep-bal] filter setup\n") );

	filter_reset(data->filter);
	filter_preset_daterange_set(data->filter, PREFS->date_range_rep, 0);

	data->filter->option[FLT_GRP_ACCOUNT] = 1;

	//5.6 set default account
	accnum = data->accnum;
	if(!accnum)
	{
		accnum = da_acc_get_first_key();
	}
	DB( g_print(" accnum=%d\n", accnum) );

	ui_acc_listview_set_active(GTK_TREE_VIEW(data->LV_acc), accnum);
	ui_acc_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_acc), data->filter);

}



/* = = = = = = = = = = = = = = = = */





//beta
#if PRIV_FILTER
static void repbalance_action_filter(GtkWidget *toolbutton, gpointer user_data)
{
struct repbalance_data *data = user_data;

	//debug
	//create_deffilter_window(data->filter, TRUE);

	if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, TRUE, FALSE) != GTK_RESPONSE_REJECT)
	{
		//repbalance_compute(data->window, NULL);
		//ui_repdtime_update_date_widget(data->window, NULL);
		//ui_repdtime_update_daterange(data->window, NULL);

		//hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_ALLDATE);
	}
}
#endif

static void repbalance_action_viewlist(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 0);
	repbalance_sensitive(data->window, NULL);
}

/*static void repbalance_action_mode_changed(GtkWidget *toolbutton, gpointer user_data)
{
struct repbalance_data *data = user_data;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->BT_list), TRUE);
	repbalance_action_viewlist(toolbutton, data);
}
*/

static void repbalance_action_viewline(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	repbalance_sensitive(data->window, NULL);
}


static void repbalance_action_print(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data = user_data;
gint tmpintvl, page;
gchar *title, *name;

	tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	name  = g_strdup_printf("hb-repbalance_%s.csv", hbtk_get_label(CYA_REPORT_INTVL, tmpintvl) );

	if( page == 0 )
	{
	GString *node;
	
		title = repbalance_compute_title(tmpintvl);
	
		node = lst_repbal_to_string(HB_STRING_PRINT, GTK_TREE_VIEW(data->LV_report), title);

		hb_print_listview(GTK_WINDOW(data->window), node->str, NULL, title, name, FALSE);

		g_string_free(node, TRUE);
		g_free(title);
	}
	else
	{
		gtk_chart_print(GTK_CHART(data->RE_chart), GTK_WINDOW(data->window), NULL, name);
	}

}


static void repbalance_cb_filter_changed(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	DB( g_print("\n[rep-bal] cb filter changed\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	_data_collect_txn(data);
	repbalance_compute(data->window, NULL);
}


static void repbalance_action_filter_reset(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	DB( g_print("\n[rep-bal] filter reset\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//TODO: to review: clean selection
	ui_acc_listview_quick_select(GTK_TREE_VIEW(data->LV_acc), "non");

	repbalance_filter_setup(data);

	g_signal_handler_block(data->CY_range, data->hid[HID_REPBALANCE_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), PREFS->date_range_rep);
	g_signal_handler_unblock(data->CY_range, data->hid[HID_REPBALANCE_RANGE]);

	repbalance_cb_filter_changed(widget, user_data);
}


static void repbalance_date_change(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	DB( g_print("\n[rep-bal] date minmax change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	// set min/max date for both widget
	//5.8 check for error
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), ( data->filter->mindate > data->filter->maxdate ) ? TRUE : FALSE);
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), ( data->filter->maxdate < data->filter->mindate ) ? TRUE : FALSE);

	g_signal_handler_block(data->CY_range, data->hid[HID_REPBALANCE_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->hid[HID_REPBALANCE_RANGE]);

	repbalance_update_daterange(widget, NULL);

	repbalance_cb_filter_changed(widget, user_data);
}


static void repbalance_range_change(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gint range;

	DB( g_print("\n[rep-bal] date range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));

	//should never happen
	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, 0);
	}

	//#2046032 set min/max date for both widget
	//5.8 check for error
	gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), ( data->filter->mindate > data->filter->maxdate ) ? TRUE : FALSE);
	gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), ( data->filter->maxdate < data->filter->mindate ) ? TRUE : FALSE);


	g_signal_handler_block(data->PO_mindate, data->hid[HID_REPBALANCE_MINDATE]);
	g_signal_handler_block(data->PO_maxdate, data->hid[HID_REPBALANCE_MAXDATE]);

	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);

	g_signal_handler_unblock(data->PO_mindate, data->hid[HID_REPBALANCE_MINDATE]);
	g_signal_handler_unblock(data->PO_maxdate, data->hid[HID_REPBALANCE_MAXDATE]);


	repbalance_update_daterange(widget, NULL);

	repbalance_cb_filter_changed(widget, user_data);

}


static void
repbalance_cb_acc_changed(GtkCellRendererToggle *cell, gchar *path_str, gpointer user_data)
{
struct repbalance_data *data = user_data;

	ui_acc_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_acc), data->filter);

	repbalance_cb_filter_changed(data->window, user_data);
}


static gboolean
repbalance_cb_acc_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
struct repbalance_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(label), GTK_TYPE_WINDOW)), "inst_data");

	ui_acc_listview_quick_select(GTK_TREE_VIEW(data->LV_acc), uri);
	repbalance_cb_acc_changed(NULL, NULL, data);
	
    return TRUE;
}


static void
repbalance_detail_onRowActivated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
struct repbalance_data *data;
Transaction *active_txn;
gboolean result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print ("\n[rep-bal] A detail row has been double-clicked!\n") );

	active_txn = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_detail));
	if(active_txn)
	{
	Transaction *old_txn, *new_txn;

		//#1909749 skip reconciled if lock is ON
		if( PREFS->safe_lock_recon == TRUE && active_txn->status == TXN_STATUS_RECONCILED )
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

	//#2018039
	list_txn_set_lockreconciled(GTK_TREE_VIEW(data->LV_detail), PREFS->safe_lock_recon);

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

	DB( g_print("\n[rep-bal] toggledetail to %d\n", (int)data->detail) );

	repbalance_update_detail(widget, user_data);
}


static void repbalance_cb_zoomx_changed(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;
gdouble value;

	DB( g_print("(\n[rep-bal] zoomx\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	value = gtk_range_get_value(GTK_RANGE(data->RG_zoomx));

	DB( g_print(" + scale is %.2f\n", value) );

	gtk_chart_set_barw(GTK_CHART(data->RE_chart), value);

}


static void repbalance_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct repbalance_data *data;

	DB( g_print("\n[rep-bal] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	repbalance_update_info(widget,NULL);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	gtk_chart_show_minor(GTK_CHART(data->RE_chart), GLOBALS->minor);
	gtk_chart_queue_redraw(GTK_CHART(data->RE_chart));
}


static void repbalance_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter iter;
guint key = -1;

	DB( g_print("\n[rep-bal] selection\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_OVER_KEY, &key, -1);
	}

	DB( g_print(" - active is %d\n", key) );

	repbalance_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
}



/* = = = = = = = = = = = = = = = = */


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


static void repbalance_window_setup(struct repbalance_data *data)
{

	DB( g_print("\n[rep-bal] setup\n") );

	DB( g_print(" init data\n") );

	DB( g_print(" populate\n") );
	ui_acc_listview_populate(data->LV_acc, ACC_LST_INSERT_REPORT, NULL);

	repbalance_filter_setup(data);

	DB( g_print(" set widgets default\n") );

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor),GLOBALS->minor);

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);

	gtk_chart_show_legend(GTK_CHART(data->RE_chart), FALSE, FALSE);
	gtk_chart_show_xval(GTK_CHART(data->RE_chart), TRUE);
	gtk_chart_set_smallfont (GTK_CHART(data->RE_chart), PREFS->rep_smallfont);

	DB( g_print(" connect widgets signals\n") );

	//display signals
	g_signal_connect (data->CY_intvl,	"changed", G_CALLBACK (repbalance_compute), NULL);
	g_signal_connect (data->CM_showempty,"toggled", G_CALLBACK (repbalance_compute), NULL);
	g_signal_connect (data->CM_minor,	"toggled", G_CALLBACK (repbalance_toggle_minor), NULL);
	g_signal_connect (data->RG_zoomx, "value-changed", G_CALLBACK (repbalance_cb_zoomx_changed), NULL);

	//filter signals
	#if PRIV_FILTER
	g_signal_connect (G_OBJECT (data->BT_filter), "clicked", G_CALLBACK (repbalance_action_filter), (gpointer)data);
	#endif
	g_signal_connect (data->BT_reset , "clicked", G_CALLBACK (repbalance_action_filter_reset), NULL);
	data->hid[HID_REPBALANCE_RANGE]   = g_signal_connect (data->CY_range, "changed", G_CALLBACK (repbalance_range_change), NULL);
	data->hid[HID_REPBALANCE_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (repbalance_date_change), (gpointer)data);
	data->hid[HID_REPBALANCE_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (repbalance_date_change), (gpointer)data);
	g_signal_connect (data->BT_all, "activate-link", G_CALLBACK (repbalance_cb_acc_activate_link), NULL);
	g_signal_connect (data->BT_non, "activate-link", G_CALLBACK (repbalance_cb_acc_activate_link), NULL);
	g_signal_connect (data->BT_inv, "activate-link", G_CALLBACK (repbalance_cb_acc_activate_link), NULL);
	g_signal_connect (data->CM_inclxfer, "toggled", G_CALLBACK (repbalance_cb_filter_changed), NULL);
	//item filter
	GtkCellRendererToggle *renderer = g_object_get_data(G_OBJECT(data->LV_acc), "togrdr_data");
	g_signal_connect_after (G_OBJECT(renderer), "toggled", G_CALLBACK (repbalance_cb_acc_changed), (gpointer)data);


	//toolbar signals
	g_signal_connect (data->BT_list, "clicked", G_CALLBACK (repbalance_action_viewlist), (gpointer)data);
	g_signal_connect (data->BT_line, "clicked", G_CALLBACK (repbalance_action_viewline), (gpointer)data);
	g_signal_connect (data->BT_detail, "clicked", G_CALLBACK (repbalance_toggle_detail), (gpointer)data);
	g_signal_connect (data->BT_refresh, "clicked", G_CALLBACK (repbalance_cb_filter_changed), (gpointer)data);
	g_signal_connect (data->BT_print, "clicked", G_CALLBACK (repbalance_action_print), (gpointer)data);

	//treeview signals
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), "changed", G_CALLBACK (repbalance_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_detail), "row-activated", G_CALLBACK (repbalance_detail_onRowActivated), NULL);

}


static gboolean repbalance_window_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbalance_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[rep-bal] window mapped\n") );

	//setup, init and show window
	repbalance_window_setup(data);

	//trigger update
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), PREFS->date_range_rep);

	data->mapped_done = TRUE;

	return FALSE;
}


static gboolean repbalance_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbalance_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[rep-bal] window dispose\n") );

	if(data->txn_queue != NULL)
		g_queue_free (data->txn_queue);

	da_flt_free(data->filter);

	g_free(data);

	//store position and size
	wg = &PREFS->ove_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);

	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d\n", wg->l, wg->t, wg->w, wg->h) );

	//enable define windows
	GLOBALS->define_off--;
	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	//unref window to our open window list
	GLOBALS->openwindows = g_slist_remove(GLOBALS->openwindows, widget);

	return FALSE;
}


//allocate our object/memory
static void repbalance_window_acquire(struct repbalance_data *data)
{
	DB( g_print("\n[rep-bal] acquire\n") );

	data->filter = da_flt_malloc();
	data->detail = 0;
}


// the window creation
GtkWidget *repbalance_window_new(guint accnum)
{
struct repbalance_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainbox, *hbox, *vbox, *fbox, *bbox, *notebook, *vpaned, *scrollwin;
GtkWidget *label, *widget, *table, *treebox, *treeview;
gint row;

	DB( g_print("\n[rep-bal] window new\n") );

	data = g_malloc0(sizeof(struct repbalance_data));
	if(!data) return NULL;

	data->accnum = accnum;
	repbalance_window_acquire (data);

	//disable define windows
	GLOBALS->define_off++;
	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

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
	mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	hb_widget_set_margin(GTK_WIDGET(mainbox), SPACING_SMALL);
	gtk_window_set_child(GTK_WINDOW(window), mainbox);

	//control part
	table = gtk_grid_new ();
	gtk_widget_set_hexpand (GTK_WIDGET(table), FALSE);
	gtk_box_prepend (GTK_BOX (mainbox), table);

	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);

	row = 0;
	//label = make_label_group(_("Display"));
	//gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	//row++;
	fbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_grid_attach (GTK_GRID (table), fbox, 0, row, 3, 1);

		label = make_label_group(_("Display"));
		//gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);
		gtk_box_prepend (GTK_BOX (fbox), label);
		
	//5.5
	row++;
	label = make_label_widget(_("Inter_val:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
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


	//-- filter
	row++;
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	//5.8 test
	row++;
	fbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_grid_attach (GTK_GRID (table), fbox, 0, row, 3, 1);

		label = make_label_group(_("Filter"));
		//gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);
		gtk_box_prepend (GTK_BOX (fbox), label);

		// active
		label = make_label_widget(_("Active:"));
		gtk_widget_set_margin_start(label, SPACING_MEDIUM);
		gtk_box_prepend (GTK_BOX (fbox), label);
		label = make_label(NULL, 0.0, 0.5);
		gtk_widget_set_margin_start(label, SPACING_SMALL);
		data->TX_fltactive = label;
		gtk_box_prepend (GTK_BOX (fbox), label);
		widget = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_QUICKTIPS);
		data->TT_fltactive = fbox;
		gtk_box_prepend (GTK_BOX (fbox), widget);
		
		//test button
		bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_style_context_add_class (gtk_widget_get_style_context (bbox), GTK_STYLE_CLASS_LINKED);
		gtk_box_append (GTK_BOX (fbox), bbox);
		widget = make_image_button(ICONNAME_HB_CLEAR, _("Clear filter"));
		data->BT_reset = widget;
		gtk_box_prepend (GTK_BOX (bbox), widget);

	row++;
	//label = make_label_group(_("Date filter"));
	label = make_label_group(_("Date"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 2, 1);

	row++;
	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->CY_range = make_daterange(label, DATE_RANGE_FLAG_CUSTOM_DISABLE);
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

	//row++;
	//widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	//gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	row++;
	//label = make_label_group(_("Account filter"));
	label = make_label_group(_("Account"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 2, 1);

	row++;
	treebox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	
	gtk_grid_attach (GTK_GRID (table), treebox, 1, row, 2, 1);

		label = make_label (_("Select:"), 0, 0.5);
		gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_SMALL, -1);
		gtk_box_prepend (GTK_BOX (treebox), label);

		label = make_clicklabel("all", _("All"));
		data->BT_all = label;
		gtk_box_prepend (GTK_BOX (treebox), label);
		
		label = make_clicklabel("non", _("None"));
		data->BT_non = label;
		gtk_box_prepend (GTK_BOX (treebox), label);

		label = make_clicklabel("inv", _("Invert"));
		data->BT_inv = label;
		gtk_box_prepend (GTK_BOX (treebox), label);

	row++;
 	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
 	data->SW_acc = scrollwin;
	treeview = ui_acc_listview_new(TRUE);
	data->LV_acc = treeview;
	gtk_widget_set_vexpand (treeview, TRUE);
	gtk_widget_set_size_request(treeview, HB_MINWIDTH_LIST, -1);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_grid_attach (GTK_GRID (table), scrollwin, 1, row, 2, 1);

	//#2083175 option for xfer
	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Include _transfer"));
	data->CM_inclxfer = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);


	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start (vbox, SPACING_SMALL);
    hbtk_box_prepend (GTK_BOX (mainbox), vbox);

	//toolbar
	widget = repbalance_toolbar_create(data);
	data->TB_bar = widget; 
	gtk_box_prepend (GTK_BOX (vbox), widget);

	//infos
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_widget_set_margin_bottom (hbox, SPACING_SMALL);
	gtk_box_prepend (GTK_BOX (vbox), hbox);

	widget = make_label(NULL, 0.5, 0.5);
	gimp_label_set_attributes (GTK_LABEL (widget), PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL, -1);
	data->TX_daterange = widget;
	gtk_box_prepend (GTK_BOX (hbox), widget);

	label = gtk_label_new(NULL);
	data->TX_info = label;
	gtk_box_append (GTK_BOX (hbox), label);


	/* report area */
	notebook = gtk_notebook_new();
	data->GR_result = notebook;
	gtk_widget_show(notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);

    hbtk_box_prepend (GTK_BOX (vbox), notebook);

	//page: list
	vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vpaned, NULL);

		scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		treeview = lst_repbal_create();
		data->LV_report = treeview;
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
		gtk_paned_pack1 (GTK_PANED(vpaned), scrollwin, TRUE, TRUE);

		//detail
		scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		data->GR_detail = scrollwin;
		//gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW (scrollwin), GTK_CORNER_TOP_RIGHT);
		treeview = create_list_transaction(LIST_TXN_TYPE_DETAIL, PREFS->lst_det_columns);
		data->LV_detail = treeview;
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
		gtk_paned_pack2 (GTK_PANED(vpaned), scrollwin, TRUE, TRUE);

		list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);

	//page: lines
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

	//hide start widget
	hb_widget_visible(data->LB_zoomx, FALSE);
	hb_widget_visible(data->RG_zoomx, FALSE);
	hb_widget_visible(data->CM_minor, PREFS->euro_active);
	hb_widget_visible(data->CM_inclxfer, FALSE);
	hb_widget_visible(data->GR_detail, data->detail);

	return window;
}


/* = = = = = = = = = = = = = = = = */



static GString *lst_repbal_to_string(ToStringMode mode, GtkTreeView *treeview, gchar *title)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
gchar sep;

	node = g_string_new(NULL);

	sep = (mode == HB_STRING_EXPORT) ? ';' : '\t';
	
	// header
	g_string_append (node, _("Date") );	
	g_string_append_c (node, sep );
	g_string_append (node, _("Expense") );	
	g_string_append_c (node, sep );
	g_string_append (node, _("Income") );	
	g_string_append_c (node, sep );
	g_string_append (node, _("Balance") );
	g_string_append (node, "\n" );
	
	// lines
	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	gchar *intvlname;
	gdouble values[4];

		gtk_tree_model_get (model, &iter,
			LST_OVER_LABEL, &intvlname,
			LST_OVER_EXPENSE, &values[0],
			LST_OVER_INCOME,  &values[1],
			LST_OVER_TOTAL,   &values[2],
			-1);

		g_string_append (node, intvlname );	

		for(guint i=0;i<3;i++)
		{
			g_string_append_c(node, sep);
			_format_decimal(node, mode, values[i]);
		}
		g_string_append_c(node, '\n');

		//leak
		g_free(intvlname);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}


static void lst_repbal_cell_data_function_date (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gchar *datestr;
gint flags;
gchar *color;
gint weight;

	gtk_tree_model_get(model, iter,
		LST_OVER_LABEL, &datestr,
		LST_OVER_FLAGS, &flags,
		-1);

	color = NULL;
	weight = PANGO_WEIGHT_NORMAL;

	if( flags & REPORT_FLAG_OVER )
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
gint flags;
gchar *color;
//gint weight;
guint32 kcur = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gtk_tree_view_column_get_tree_view(col)), "kcur_data"));


	//get datas
	gtk_tree_model_get(model, iter,
		LST_OVER_FLAGS, &flags,
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

		if( flags & REPORT_FLAG_OVER )
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
		G_TYPE_INT,		//pos
		G_TYPE_INT,		//key
		G_TYPE_STRING,	//label
		G_TYPE_DOUBLE,	//exp
		G_TYPE_DOUBLE,	//inc
		G_TYPE_DOUBLE,	//total
		G_TYPE_INT		//flags
		);

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (view), PREFS->grid_lines);

	/* column: Label */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Date"));
	renderer = gtk_cell_renderer_text_new();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbal_cell_data_function_date, NULL, NULL);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_OVER_DATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Expense */
	column = lst_repbal_column_amount_create(_("Expense"), LST_OVER_EXPENSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Income */
	column = lst_repbal_column_amount_create(_("Income"), LST_OVER_INCOME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Total/Balance */
	column = lst_repbal_column_amount_create(_("Balance"), LST_OVER_TOTAL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

  /* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), FALSE);

	return(view);
}


