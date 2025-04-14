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

#include "rep-time.h"

#include "list-operation.h"
#include "gtk-chart.h"
#include "gtk-dateentry.h"

#include "dsp-mainwindow.h"
#include "ui-account.h"
#include "ui-payee.h"
#include "ui-category.h"
#include "ui-filter.h"
#include "ui-transaction.h"
#include "ui-tag.h"

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


/* prototypes */
static GtkWidget *lst_reptime_create(void);
static GString *lst_reptime_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard);
static void lst_reptime_set_cur(GtkTreeView *treeview, guint32 kcur);


extern HbKvData CYA_REPORT_GRPBY_TREND[];
extern HbKvData CYA_REPORT_INTVL[];


/* = = = = = = = = = = = = = = = = */


static gchar *
reptime_compute_title(gint src, gint intvl)
{
gchar *title;

	//TRANSLATORS: example 'Category by Month'
	title = g_strdup_printf(_("%s by %s"), 
		hbtk_get_label(CYA_REPORT_GRPBY_TREND, src), 
		hbtk_get_label(CYA_REPORT_INTVL, intvl)
	);

	return title;
}


static void reptime_sensitive(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
gboolean visible, sensitive;
gint page;

	DB( g_print("\n[rep-time] sensitive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	visible = page == 0 ? TRUE : FALSE;
	hb_widget_visible (data->BT_detail, visible);
	hb_widget_visible (data->BT_export, visible);

	visible = page == 0 ? FALSE : TRUE;
	//5.7
	//hb_widget_visible(data->BT_print, visible);
	hb_widget_visible(data->LB_zoomx, visible);
	hb_widget_visible(data->RG_zoomx, visible);

	page = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail)), NULL);
	sensitive = ((page > 0) && data->detail) ? TRUE : FALSE;
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "detclip")), sensitive);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "detcsv")), sensitive);

}


static void reptime_update_daterange(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
gchar *daterange;

	DB( g_print("\n[rep-time] update daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	daterange = filter_daterange_text_get(data->filter);
	gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
	g_free(daterange);
}


static void reptime_detail(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
guint active = GPOINTER_TO_INT(user_data);
guint tmpintvl;
guint32 from;
GList *list;
GtkTreeModel *model;
GtkTreeIter  iter, child;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[rep-time] detail\n") );

	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpintvl = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));

	//reptime_compute_set_filter was already called here

	//get our min max date
	from = data->filter->mindate;
	//to   = data->filter->maxdate;

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

			if(filter_txn_match(data->filter, ope) == 1)
			{
				pos = report_interval_get_pos(tmpintvl, from, ope);
				if( pos == active )
				{
				gdouble dtlamt = report_txn_amount_get(data->filter, ope);

					gtk_tree_store_insert_with_values (GTK_TREE_STORE(model),
						&iter, NULL, -1,
						MODEL_TXN_POINTER, ope,
						MODEL_TXN_SPLITAMT, dtlamt,
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


static void reptime_update(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
GtkTreeModel *model;
gint page;
gint tmpsrc, tmpintvl;
gboolean cumul;
gchar *title;

	DB( g_print("\n[rep-time] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");


	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	//byamount = 0;
	tmpsrc   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	tmpintvl = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
	cumul    = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cumul));

	// ensure not exp & inc for piechart
	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	DB( g_print(" page %d\n\n", page) );
	//DB( g_print(" tmpintvl %d\n\n", tmpintvl) );

	//column = LST_HUBREPTIME_POS;
	//DB( g_print(" sort on column %d\n\n", column) );
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), column, GTK_SORT_DESCENDING);

	gtk_chart_show_legend(GTK_CHART(data->RE_chart), FALSE, FALSE);
	gtk_chart_show_xval(GTK_CHART(data->RE_chart), TRUE);

	gtk_chart_show_average(GTK_CHART(data->RE_chart), data->average, cumul);

	title = reptime_compute_title(tmpsrc, tmpintvl);
	gtk_chart_set_datas_total(GTK_CHART(data->RE_chart), model, LST_HUBREPTIME_TOTAL, LST_HUBREPTIME_TOTAL, title, NULL);
	g_free(title);

	if(page == 1)
	{
		DB(	g_print(" change chart type to %d\n", data->charttype) );
		gtk_chart_set_type (GTK_CHART(data->RE_chart), data->charttype);
		gtk_chart_set_showmono(GTK_CHART(data->RE_chart), TRUE);
	}
	
	//test 5.8
	//as it is not the filter dialog, count
	da_flt_count_item(data->filter);
	gchar *txt = filter_text_summary_get(data->filter);
	ui_label_set_integer(GTK_LABEL(data->TX_fltactive), data->filter->n_active);
	gtk_widget_set_tooltip_text(data->TT_fltactive, txt);
	g_free(txt);	
}


static void reptime_export_result_clipboard(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct reptime_data *data;
GtkClipboard *clipboard;
GString *node;
gchar *coltitle;
gint tmpintvl;

	DB( g_print("\n[rep-time] export result clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));

	coltitle = hbtk_get_label(CYA_REPORT_INTVL, tmpintvl);
	node = lst_reptime_to_string(GTK_TREE_VIEW(data->LV_report), coltitle, TRUE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void reptime_export_result_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct reptime_data *data;
gchar *filename = NULL;
GString *node;
GIOChannel *io;
gchar *name;
gint tmpsrc;
gchar *title;
gint tmpintvl;

	DB( g_print("\n[rep-time] export result csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	name = g_strdup_printf("hb-reptime_%s.csv", hbtk_get_label(CYA_REPORT_GRPBY_TREND, tmpsrc) );

	if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, name) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );
		io = g_io_channel_new_file(filename, "w", NULL);
		if(io != NULL)
		{
			tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
			title = hbtk_get_label(CYA_REPORT_INTVL, tmpintvl);

			node = lst_reptime_to_string(GTK_TREE_VIEW(data->LV_report), title, FALSE);
			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
		g_free( filename );
	}
	g_free(name);
}


static void reptime_export_detail_clipboard(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct reptime_data *data;
GtkClipboard *clipboard;
GString *node;
guint flags;

	DB( g_print("\n[rep-time] export detail clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	flags = LST_TXN_EXP_CLR | LST_TXN_EXP_PMT | LST_TXN_EXP_CAT | LST_TXN_EXP_TAG;
	node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), TRUE, FALSE, FALSE, flags);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void reptime_export_detail_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct reptime_data *data;
gchar *filepath = NULL;
GString *node;
GIOChannel *io;
gchar *name;
gint tmpsrc;
gboolean hassplit, hasstatus;

	DB( g_print("\n[rep-time] export detail csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	name = g_strdup_printf("hb-reptime-detail_%s.csv", hbtk_get_label(CYA_REPORT_GRPBY_TREND, tmpsrc) );

	filepath = g_build_filename(PREFS->path_export, name, NULL);

	//#2019312
	//if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filepath, name) == TRUE )
	if( ui_dialog_export_csv(GTK_WINDOW(data->window), &filepath, &hassplit, &hasstatus, FALSE) == GTK_RESPONSE_ACCEPT )
	{
		DB( g_print(" + filename is %s\n", filepath) );

		io = g_io_channel_new_file(filepath, "w", NULL);
		if(io != NULL)
		{
		guint flags;

			flags = LST_TXN_EXP_PMT | LST_TXN_EXP_CAT | LST_TXN_EXP_TAG;
			if( hasstatus )
				flags |= LST_TXN_EXP_CLR;
			node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), FALSE, hassplit, FALSE, flags);
			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);

			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
	}

	g_free( filepath );
	g_free(name);
}


static void reptime_update_for(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
gint tmpsrc, pageid;
Filter *flt;

	DB( g_print("\n[rep-time] update for\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	flt = data->filter;

	DB( g_print("  src=%d\n", tmpsrc) );

	//filter_reset(flt);

	//inactive all filters
	flt->option[FLT_GRP_CATEGORY] = 0;
	flt->option[FLT_GRP_PAYEE] = 0;
	flt->option[FLT_GRP_ACCOUNT] = 0;
	flt->option[FLT_GRP_TAG] = 0;

	switch(tmpsrc)
	{
		case REPORT_GRPBY_CATEGORY:
			pageid = 1;
			flt->option[FLT_GRP_CATEGORY] = 1;
			da_flt_status_cat_set(flt, 0, TRUE);
			break;
		case REPORT_GRPBY_PAYEE:
			pageid = 2;
			flt->option[FLT_GRP_PAYEE] = 1;
			da_flt_status_pay_set(flt, 0, TRUE);
			break;
		case REPORT_GRPBY_TAG:
			pageid = 3;
			flt->option[FLT_GRP_TAG] = 1;
			break;
		default: //REPORT_GRPBY_ACCOUNT
			pageid = 0;
			flt->option[FLT_GRP_ACCOUNT] = 1;
			break;
	}

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_itemtype), pageid);

	//hb_widget_visible(data->CM_all, defvisible);
	//hb_widget_visible(data->BT_filter, !monoflt);


}


//reset the filter
static void reptime_filter_setup(struct reptime_data *data)
{
guint32 accnum;

	DB( g_print("\n[rep-time] reset filter\n") );

	filter_reset(data->filter);
	filter_preset_daterange_set(data->filter, PREFS->date_range_rep, data->accnum);
	/* 3.4 : make int transfer out of stats */
	//TODO: for compatibility with < 5.3, keep this unset, but normally it should be set
	//filter_preset_type_set(data->filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);

	//5.6 set default account
	data->filter->option[FLT_GRP_ACCOUNT] = 1;
	accnum = data->accnum;
	if(!accnum)
	{
		accnum = da_acc_get_first_key();
	}
	DB( g_print(" accnum=%d\n", accnum) );

	ui_acc_listview_set_active(GTK_TREE_VIEW(data->LV_acc), accnum);
	ui_acc_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_acc), data->filter);
}


//beta
#if BETA_FILTER
static void reptime_action_filter(GtkWidget *toolbutton, gpointer user_data)
{
struct reptime_data *data = user_data;

	//debug
	//create_deffilter_window(data->filter, TRUE);

	if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, TRUE, FALSE) != GTK_RESPONSE_REJECT)
	{
		reptime_compute(data->window, NULL);
		//ui_repdtime_update_date_widget(data->window, NULL);
		//ui_repdtime_update_daterange(data->window, NULL);

		/*g_signal_handler_block(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_ALLDATE);
		g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
		*/
	}
}
#endif

/* = = = = = = = = = = = = = = = = */



static void reptime_compute(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
gint tmpintvl, showempty;
guint32 from, to;
gboolean cumul;
gdouble cumulation, average;

GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
gint id;
guint n_result, i;

	DB( g_print("----------------\n[rep-time] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//clear all
	if(data->txn_queue != NULL)
		g_queue_free (data->txn_queue);
	data->txn_queue = NULL;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	gtk_list_store_clear (GTK_LIST_STORE(model));
	gtk_chart_set_datas_none(GTK_CHART(data->RE_chart));

	//#2019876 return is invalid date range
	if( data->filter->maxdate < data->filter->mindate )
		return;

	tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
	cumul     = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cumul));
	//range     = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));
	showempty = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_showempty));

	data->accnum = 0;

	//to remove > 5.0.2
	//#1715532 5.0.5: no... but only showall
	/*if( (showall == TRUE) && (range == FLT_RANGE_MISC_ALLDATE) )
	{
		filter_preset_daterange_set(data->filter, data->filter->range, data->accnum);
		reptime_update_quickdate(widget, NULL);
	}*/

	//get our min max date
	from = data->filter->mindate;
	to   = data->filter->maxdate;

	//TODO: not necessary until date range change
	//free previous txn
	data->txn_queue = hbfile_transaction_get_partial(data->filter->mindate, data->filter->maxdate);

	n_result = report_interval_count(tmpintvl, from, to);

	//DB( gint tmpsrc = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src)) );
	//DB( g_print(" %s :: n_result=%d\n", hbtk_get_label(CYA_REPORT_GRPBY_TREND, tmpsrc), n_result) );

	/* allocate some memory */
	data->tmp_income  = g_malloc0((n_result+2) * sizeof(gdouble));
	data->tmp_expense = g_malloc0((n_result+2) * sizeof(gdouble));

	if(data->tmp_income && data->tmp_expense)
	{
		list = g_queue_peek_head_link(data->txn_queue);
		while (list != NULL)
		{
		Transaction *ope = list->data;

			//DB( g_print("** testing '%s', acc=%d, cat=%d, pay=%d\n", ope->memo, ope->kacc, ope->kcat, ope->kpay) );

			if( (filter_txn_match(data->filter, ope) == 1) )
			{
			guint pos;
			gdouble amount;

				amount = report_txn_amount_get(data->filter, ope);

				 //#1829603 Multi currencies problem in Trend Time Report
				//if( ! ( tmpsrc == REPORT_GRPBY_ACCOUNT && showall == FALSE) )
					amount = hb_amount_base(amount, ope->kcur);

				pos = report_interval_get_pos(tmpintvl, from, ope);
				if( pos <= n_result )
				{
					//DB( g_print("** pos=%d : add of %.2f\n", pos, amount) );
					if(amount < 0)
						data->tmp_expense[pos] += amount;
					else
						data->tmp_income[pos] += amount;
				}
				else
				{
					//DB( g_print("** pos=%d : invalid offset\n", pos) );
				}
			}
			list = g_list_next(list);
		}

		/* clear and detach our model */
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), NULL); /* Detach model from view */

		cumulation = 0.0;

		/* insert into the treeview */
		for(i=0, id=0; i<n_result; i++)
		{
		gchar intvlname[64];
		gdouble total = data->tmp_expense[i] + data->tmp_income[i];
		gdouble value;

			//if( !showempty && total == 0 )
			//#2091004 5.8.6 keep lines with exact 0
			if( !showempty 
			 && (hb_amount_cmp(data->tmp_expense[i], 0.0) == 0)
			 && (hb_amount_cmp(data->tmp_income[i], 0.0) == 0)
			 )
				continue;

			report_interval_snprint_name(intvlname, sizeof(intvlname)-1, tmpintvl, from, i);

			//DB( g_print("try to insert item %d\n", i) );

			cumulation += total;
			value = (cumul == TRUE) ? cumulation : total;

			//DB( g_print(" inserting %2d, '%s', %9.2f\n", i, intvlname, value) );

	    	gtk_list_store_insert_with_values (GTK_LIST_STORE(model), &iter, -1,
				LST_HUBREPTIME_POS, id++,
				LST_HUBREPTIME_KEY, i,
				LST_HUBREPTIME_LABEL, intvlname,
				LST_HUBREPTIME_EXPENSE, data->tmp_expense[i],
				LST_HUBREPTIME_INCOME, data->tmp_income[i],
				LST_HUBREPTIME_TOTAL, value,
				-1);

		}

		// set chart and listview currency
		//TODO: we should maybe display in native instead of global ?
		//maybe to be done while getting active checkbos and store into filter
		guint32 kcur = GLOBALS->kcur;
		/*if( (showall == FALSE) && (tmpsrc == REPORT_GRPBY_ACCOUNT) )
		{
		Account *acc;

			//selkey = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));
			selkey = ui_acc_entry_popover_get_key(GTK_BOX(data->PO_acc));
			acc = da_acc_get(selkey);

			if( acc != NULL )
			{
				kcur = acc->kcur;
				//fix 5.4.2 crash here
				gtk_chart_set_overdrawn(GTK_CHART(data->RE_chart), acc->minimum);
			}
		}*/

		lst_reptime_set_cur(GTK_TREE_VIEW(data->LV_report), kcur);
		gtk_chart_set_currency(GTK_CHART(data->RE_chart), kcur);


		/* update column 0 title */
		GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report), 0);
		if(column)
			gtk_tree_view_column_set_title(column, hbtk_get_label(CYA_REPORT_INTVL, tmpintvl) );

		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

		/* Re-attach model to view */
  		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
		g_object_unref(model);

		//update average
		gtk_label_set_text(GTK_LABEL(data->TX_info), "");

		if( cumul == TRUE )
		{
		gchar *info;
		gchar   buf[128];

			average = cumulation / n_result;
			data->average = average;

			hb_strfmon(buf, 127, average, kcur, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor)) );

			info = g_strdup_printf(_("Average: %s"), buf);
			gtk_label_set_text(GTK_LABEL(data->TX_info), info);
			g_free(info);
		}
	}

	/* free our memory */
	g_free(data->tmp_expense);
	g_free(data->tmp_income);

	reptime_update(widget, user_data);
}




/* = = = = = = = = = = = = = = = = */


static void reptime_for(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;

	DB( g_print("\n[rep-time] for\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	reptime_update_for(widget, data);

	reptime_compute(widget, data);
}


static void reptime_action_filter_reset(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;

	DB( g_print("\n[rep-time] filter reset\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//TODO: to review: clean selection
	ui_acc_listview_quick_select(GTK_TREE_VIEW(data->LV_acc), "non");
	ui_cat_listview_quick_select(GTK_TREE_VIEW(data->LV_cat), "non");
	ui_pay_listview_quick_select(GTK_TREE_VIEW(data->LV_pay), "non");
	ui_tag_listview_quick_select(GTK_TREE_VIEW(data->LV_tag), "non");

	reptime_filter_setup(data);

	g_signal_handler_block(data->CY_range, data->hid[HID_REPTIME_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), PREFS->date_range_rep);
	g_signal_handler_unblock(data->CY_range, data->hid[HID_REPTIME_RANGE]);

	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_src), REPORT_GRPBY_ACCOUNT);

	reptime_compute(data->window, NULL);
}


static void
reptime_date_change(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;

	DB( g_print("\n[rep-time] date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	// set min/max date for both widget
	//5.8 check for error
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), ( data->filter->mindate > data->filter->maxdate ) ? TRUE : FALSE);
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), ( data->filter->maxdate < data->filter->mindate ) ? TRUE : FALSE);

	g_signal_handler_block(data->CY_range, data->hid[HID_REPTIME_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->hid[HID_REPTIME_RANGE]);

	reptime_compute(widget, NULL);
	reptime_update_daterange(widget, NULL);

}


static void reptime_update_quickdate(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;

	DB( g_print("\n[rep-time] update quickdate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	g_signal_handler_block(data->PO_mindate, data->hid[HID_REPTIME_MINDATE]);
	g_signal_handler_block(data->PO_maxdate, data->hid[HID_REPTIME_MAXDATE]);

	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);

	g_signal_handler_unblock(data->PO_mindate, data->hid[HID_REPTIME_MINDATE]);
	g_signal_handler_unblock(data->PO_maxdate, data->hid[HID_REPTIME_MAXDATE]);

}


static void reptime_range_change(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
gint range;

	DB( g_print("\n[rep-time] range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));

	//should never happen
	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, data->accnum);
	}

	//#2046032 set min/max date for both widget
	//5.8 check for error
	gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), ( data->filter->mindate > data->filter->maxdate ) ? TRUE : FALSE);
	gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), ( data->filter->maxdate < data->filter->mindate ) ? TRUE : FALSE);

	reptime_update_quickdate(widget, NULL);

	reptime_compute(widget, NULL);
	reptime_update_daterange(widget, NULL);

}


/* = = = = = = = = = = = = = = = = */


//beta
#if PRIV_FILTER
static void reptime_action_filter(GtkWidget *toolbutton, gpointer user_data)
{
struct reptime_data *data = user_data;

	//debug
	//create_deffilter_window(data->filter, TRUE);

	if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, TRUE, FALSE) != GTK_RESPONSE_REJECT)
	{
		reptime_compute(data->window, NULL);
		//ui_repdtime_update_date_widget(data->window, NULL);
		//ui_repdtime_update_daterange(data->window, NULL);

		/*g_signal_handler_block(data->CY_range, data->hid[HID_REPDIST_RANGE]);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_ALLDATE);
		g_signal_handler_unblock(data->CY_range, data->hid[HID_REPDIST_RANGE]);
		*/
	}
}
#endif


static void reptime_action_viewlist(GtkWidget *toolbutton, gpointer user_data)
{
struct reptime_data *data = user_data;

	data->charttype = CHART_TYPE_NONE;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 0);
	reptime_sensitive(data->window, NULL);
}


static void reptime_action_viewline(GtkWidget *toolbutton, gpointer user_data)
{
struct reptime_data *data = user_data;

	data->charttype = CHART_TYPE_LINE;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	reptime_sensitive(data->window, NULL);
	reptime_update(data->window, NULL);
}


static void reptime_action_viewcolumn(GtkWidget *toolbutton, gpointer user_data)
{
struct reptime_data *data = user_data;

	data->charttype = CHART_TYPE_COL;
	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	reptime_sensitive(data->window, NULL);
	reptime_update(data->window, NULL);

}


static void reptime_action_print(GtkWidget *toolbutton, gpointer user_data)
{
struct reptime_data *data = user_data;
gint tmpsrc, tmpintvl, page;
gchar *coltitle, *title, *name;

	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	title = reptime_compute_title(tmpsrc, tmpintvl);

	name = g_strdup_printf("hb-reptime_%s.csv", hbtk_get_label(CYA_REPORT_GRPBY_TREND, tmpsrc) );

	if( page == 0 )
	{
	GString *node;

		coltitle = hbtk_get_label(CYA_REPORT_INTVL, tmpintvl);
		node = lst_reptime_to_string(GTK_TREE_VIEW(data->LV_report), coltitle, TRUE);

		hb_print_listview(GTK_WINDOW(data->window), node->str, NULL, title, name, FALSE);

		g_string_free(node, TRUE);
	}
	else
	{
		gtk_chart_print(GTK_CHART(data->RE_chart), GTK_WINDOW(data->window), PREFS->path_export, name);

	}

	g_free(name);
}


static void
reptime_cb_acc_changed(GtkCellRendererToggle *cell, gchar *path_str, gpointer user_data)
{
struct reptime_data *data = user_data;

	DB( g_print ("\n[rep-time] acc list changed\n") );

	ui_acc_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_acc), data->filter);
	reptime_compute(GTK_WIDGET(data->LV_acc), NULL);
}


static void
reptime_cb_cat_changed(GtkCellRendererToggle *cell, gchar *path_str, gpointer user_data)
{
struct reptime_data *data = user_data;

	DB( g_print ("\n[rep-time] cat list changed\n") );

	ui_cat_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_cat), data->filter);
	reptime_compute(GTK_WIDGET(data->LV_cat), NULL);
}


static void
reptime_cb_pay_changed(GtkCellRendererToggle *cell, gchar *path_str, gpointer user_data)
{
struct reptime_data *data = user_data;

	DB( g_print ("\n[rep-time] pay list changed\n") );

	ui_pay_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_pay), data->filter);
	reptime_compute(GTK_WIDGET(data->LV_pay), NULL);
}


static void
reptime_cb_tag_changed(GtkCellRendererToggle *cell, gchar *path_str, gpointer user_data)
{
struct reptime_data *data = user_data;

	DB( g_print ("\n[rep-time] tag list changed\n") );

	ui_tag_listview_toggle_to_filter(GTK_TREE_VIEW(data->LV_tag), data->filter);
	reptime_compute(GTK_WIDGET(data->LV_tag), NULL);
}


static gboolean
reptime_cb_activate_link (GtkWidget *label, const gchar *uri, gpointer user_data)
{
struct reptime_data *data;
gint tmpsrc;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(label), GTK_TYPE_WINDOW)), "inst_data");

	tmpsrc = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));

	switch(tmpsrc)
	{
		case REPORT_GRPBY_ACCOUNT:
			ui_acc_listview_quick_select(GTK_TREE_VIEW(data->LV_acc), uri);
			reptime_cb_acc_changed(NULL, NULL, data);
			break;
		case REPORT_GRPBY_CATEGORY:
			ui_cat_listview_quick_select(GTK_TREE_VIEW(data->LV_cat), uri);
			reptime_cb_cat_changed(NULL, NULL, data);
			break;
		case REPORT_GRPBY_PAYEE:
			ui_pay_listview_quick_select(GTK_TREE_VIEW(data->LV_pay), uri);
			reptime_cb_pay_changed(NULL, NULL, data);
			break;
		case REPORT_GRPBY_TAG:
			ui_tag_listview_quick_select(GTK_TREE_VIEW(data->LV_tag), uri);
			reptime_cb_tag_changed(NULL, NULL, data);
			break;
	}

	reptime_compute (GTK_WIDGET(data->window), NULL);

    return TRUE;
}


static void reptime_detail_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
struct reptime_data *data;
Transaction *active_txn;
gboolean result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print ("\n[rep-time] A detail row has been double-clicked!\n") );

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
			reptime_compute(data->window, NULL);

			if( path != NULL )
			{
				gtk_tree_selection_select_path(treeselection, path);
				gtk_tree_path_free(path);
			}

		}

		da_transaction_free (old_txn);
	}
}


static void reptime_update_detail(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;

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
			gtk_tree_model_get(model, &iter, LST_HUBREPTIME_KEY, &key, -1);

			DB( g_print(" - active is %d\n", key) );

			reptime_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
		}

		gtk_widget_show(data->GR_detail);
	}
	else
		gtk_widget_hide(data->GR_detail);
}


static void reptime_toggle_detail(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->detail ^= 1;

	DB( g_print("\n[rep-time] toggledetail to %d\n", data->detail) );

	reptime_update_detail(widget, user_data);
	reptime_sensitive(widget, NULL);
}


static void reptime_cb_zoomx_changed(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;
gdouble value;

	DB( g_print("\n[rep-time] zoomx\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	value = gtk_range_get_value(GTK_RANGE(data->RG_zoomx));

	DB( g_print(" + scale is %.2f\n", value) );

	gtk_chart_set_barw(GTK_CHART(data->RE_chart), value);

}


static void reptime_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct reptime_data *data;

	DB( g_print("\n[rep-time] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	//hbfile_update(data->LV_acc, (gpointer)4);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	gtk_chart_show_minor(GTK_CHART(data->RE_chart), GLOBALS->minor);
	gtk_chart_queue_redraw(GTK_CHART(data->RE_chart));
}


static void reptime_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter iter;
guint key = -1;

	DB( g_print("\n[rep-time] selection\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_HUBREPTIME_KEY, &key, -1);

	}

	DB( g_print(" - active is %d\n", key) );

	reptime_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
	reptime_sensitive(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


/* = = = = = = = = = = = = = = = = */


static const GActionEntry win_actions[] = {
	{ "resclip"		, reptime_export_result_clipboard, NULL, NULL, NULL, {0,0,0} },
	{ "rescsv"		, reptime_export_result_csv, NULL, NULL, NULL, {0,0,0} },
	{ "detclip"		, reptime_export_detail_clipboard, NULL, NULL, NULL, {0,0,0} },
	{ "detcsv"		, reptime_export_detail_csv, NULL, NULL, NULL, {0,0,0} },
//	{ "actioname"	, not_implemented, NULL, NULL, NULL, {0,0,0} },
};


static GtkWidget *
reptime_toolbar_create(struct reptime_data *data)
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

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_column = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_COLUMN, "label", _("Column"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as column"));
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

	//export button
	button = gtk_menu_button_new();
	data->BT_export = button;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(button)), GTK_STYLE_CLASS_FLAT);
	GtkWidget *image = hbtk_image_new_from_icon_name_24 (ICONNAME_HB_FILE_EXPORT);
	g_object_set (button, "image", image, NULL);
	GtkToolItem *toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), button);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem), -1);

	GMenu *menu = g_menu_new ();
	GMenu *section = g_menu_new ();
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("_Result to clipboard"), "win.resclip");
	g_menu_append (section, _("_Result to CSV")      , "win.rescsv");
	g_object_unref (section);

	section = g_menu_new ();
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("_Detail to clipboard"), "win.detclip");
	g_menu_append (section, _("_Detail to CSV")      , "win.detcsv");
	g_object_unref (section);

	GActionGroup *actiongroup = (GActionGroup*)g_simple_action_group_new ();
	data->actions = actiongroup;
	g_action_map_add_action_entries (G_ACTION_MAP (actiongroup), win_actions, G_N_ELEMENTS (win_actions), data);

	gtk_widget_insert_action_group (button, "win", G_ACTION_GROUP(actiongroup));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), G_MENU_MODEL (menu));


	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_PRINT, _("Print"), _("Print"));
	data->BT_print = button;

	return toolbar;
}



static void reptime_window_setup(struct reptime_data *data)
{
	DB( g_print("\n[rep-time] window setup\n") );

	DB( g_print(" init data\n") );

	DB( g_print(" populate\n") );
	ui_acc_listview_populate(data->LV_acc, ACC_LST_INSERT_REPORT, NULL);
	ui_cat_listview_populate(data->LV_cat, CAT_TYPE_ALL, NULL, TRUE);
	ui_pay_listview_populate(data->LV_pay, NULL, TRUE);
	ui_tag_listview_populate(data->LV_tag, 0);

	reptime_filter_setup(data);
	reptime_update_for(data->window, data);

	DB( g_print(" set widgets default\n") );
	//src is account
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_intvl), REPORT_INTVL_MONTH);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor), GLOBALS->minor);
	

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail))), "minor", (gpointer)data->CM_minor);

	gtk_chart_set_smallfont (GTK_CHART(data->RE_chart), PREFS->rep_smallfont);

	DB( g_print(" connect widgets signals\n") );

	//display signals
	data->hid[HID_REPTIME_VIEW] = g_signal_connect (data->CY_intvl, "changed", G_CALLBACK (reptime_compute), (gpointer)data);

	g_signal_connect (data->CM_cumul, "toggled", G_CALLBACK (reptime_compute), NULL);
	g_signal_connect (data->CM_showempty, "toggled", G_CALLBACK (reptime_compute), NULL);
	g_signal_connect (data->CM_minor, "toggled", G_CALLBACK (reptime_toggle_minor), NULL);
	g_signal_connect (data->RG_zoomx, "value-changed", G_CALLBACK (reptime_cb_zoomx_changed), NULL);

	//filter signals
	g_signal_connect (data->BT_reset , "clicked", G_CALLBACK (reptime_action_filter_reset), NULL);
    data->hid[HID_REPTIME_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (reptime_date_change), (gpointer)data);
    data->hid[HID_REPTIME_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (reptime_date_change), (gpointer)data);
	data->hid[HID_REPTIME_RANGE] = g_signal_connect (data->CY_range, "changed", G_CALLBACK (reptime_range_change), NULL);

	//item filter
	g_signal_connect (data->CY_src, "changed", G_CALLBACK (reptime_for), (gpointer)data);
	g_signal_connect (data->BT_all, "activate-link", G_CALLBACK (reptime_cb_activate_link), NULL);
	g_signal_connect (data->BT_non, "activate-link", G_CALLBACK (reptime_cb_activate_link), NULL);
	g_signal_connect (data->BT_inv, "activate-link", G_CALLBACK (reptime_cb_activate_link), NULL);
	
	GtkCellRendererToggle *renderer;
	renderer = g_object_get_data(G_OBJECT(data->LV_acc), "togrdr_data");
	g_signal_connect_after (G_OBJECT(renderer), "toggled", G_CALLBACK (reptime_cb_acc_changed), (gpointer)data);
	renderer = g_object_get_data(G_OBJECT(data->LV_cat), "togrdr_data");
	g_signal_connect_after (G_OBJECT(renderer), "toggled", G_CALLBACK (reptime_cb_cat_changed), (gpointer)data);
	renderer = g_object_get_data(G_OBJECT(data->LV_pay), "togrdr_data");
	g_signal_connect_after (G_OBJECT(renderer), "toggled", G_CALLBACK (reptime_cb_pay_changed), (gpointer)data);
	renderer = g_object_get_data(G_OBJECT(data->LV_tag), "togrdr_data");
	g_signal_connect_after (G_OBJECT(renderer), "toggled", G_CALLBACK (reptime_cb_tag_changed), (gpointer)data);

	//toolbar signals
	g_signal_connect (G_OBJECT (data->BT_list), "clicked", G_CALLBACK (reptime_action_viewlist), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_line), "clicked", G_CALLBACK (reptime_action_viewline), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_column), "clicked", G_CALLBACK (reptime_action_viewcolumn), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_detail), "clicked", G_CALLBACK (reptime_toggle_detail), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_refresh), "clicked", G_CALLBACK (reptime_compute), (gpointer)data);
	//export is a menu
	g_signal_connect (G_OBJECT (data->BT_print), "clicked", G_CALLBACK (reptime_action_print), (gpointer)data);

	//treeview signals
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), "changed", G_CALLBACK (reptime_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_detail), "row-activated", G_CALLBACK (reptime_detail_onRowActivated), NULL);

}


static gboolean reptime_window_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct reptime_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[rep-time] window mapped\n") );

	//setup, init and show window
	reptime_window_setup(data);

	//trigger update
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), PREFS->date_range_rep);

	data->mapped_done = TRUE;

	return FALSE;
}


static gboolean reptime_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct reptime_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[rep-time] window dispose\n") );

	if(data->txn_queue != NULL)
		g_queue_free (data->txn_queue);

	da_flt_free(data->filter);

	g_free(data);

	//store position and size
	wg = &PREFS->tme_wg;
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
static void reptime_window_acquire(struct reptime_data *data)
{
	DB( g_print("\n[rep-time] window acquire\n") );

	data->txn_queue = g_queue_new ();
	data->filter = da_flt_malloc();
	data->detail = 0;
}


// the window creation
GtkWidget *reptime_window_new(guint32 accnum)
{
struct reptime_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainbox, *hbox, *vbox, *fbox, *bbox, *notebook, *treeview, *treebox, *vpaned, *scrollwin;
GtkWidget *label, *widget, *table;
gint row;

	DB( g_print("\n[rep-time] window new\n") );

	data = g_malloc0(sizeof(struct reptime_data));
	if(!data) return NULL;

	data->accnum = accnum;
	reptime_window_acquire(data);

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

	gtk_window_set_title (GTK_WINDOW (window), _("Trend Time Report"));

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
	label = make_label_group(_("Display"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	label = make_label_widget(_("Inter_val:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_INTVL);
	data->CY_intvl = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("_Cumulate"));
	data->CM_cumul = widget;
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

	row++;
	label = make_label_group(_("Item"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 2, 1);

	row++;
	label = make_label_widget(_("_By:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_GRPBY_TREND);
	data->CY_src = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

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
	notebook = gtk_notebook_new();
	data->GR_itemtype = notebook;
	gtk_widget_show(notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);	
	gtk_grid_attach (GTK_GRID (table), notebook, 1, row, 2, 1);

	 	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	 	data->SW_acc = scrollwin;
		gtk_widget_set_margin_bottom (scrollwin, SPACING_LARGE);
		treeview = ui_acc_listview_new(TRUE);
		data->LV_acc = treeview;
		gtk_widget_set_vexpand (treeview, TRUE);
		//gtk_widget_set_size_request(widget, HB_MINWIDTH_LIST, -1);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollwin, NULL);

	 	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	 	//data->SW_acc = scrollwin;
		gtk_widget_set_margin_bottom (scrollwin, SPACING_LARGE);
		treeview = ui_cat_listview_new(TRUE, FALSE);
		data->LV_cat = treeview;
		gtk_widget_set_vexpand (treeview, TRUE);
		//gtk_widget_set_size_request(widget, HB_MINWIDTH_LIST, -1);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollwin, NULL);

	 	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	 	//data->SW_acc = scrollwin;
		gtk_widget_set_margin_bottom (scrollwin, SPACING_LARGE);
		treeview = ui_pay_listview_new(TRUE, FALSE);
		data->LV_pay = treeview;
		gtk_widget_set_vexpand (treeview, TRUE);
		//gtk_widget_set_size_request(widget, HB_MINWIDTH_LIST, -1);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollwin, NULL);

	 	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	 	//data->SW_acc = scrollwin;
		gtk_widget_set_margin_bottom (scrollwin, SPACING_LARGE);
		treeview = ui_tag_listview_new(TRUE, FALSE);
		data->LV_tag = treeview;
		gtk_widget_set_vexpand (treeview, TRUE);
		//gtk_widget_set_size_request(widget, HB_MINWIDTH_LIST, -1);
		gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
		gtk_notebook_append_page(GTK_NOTEBOOK(notebook), scrollwin, NULL);

	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start (vbox, SPACING_SMALL);
    hbtk_box_prepend (GTK_BOX (mainbox), vbox);

	//toolbar
	widget = reptime_toolbar_create(data);
	data->TB_bar = widget;
	gtk_box_prepend (GTK_BOX (vbox), widget);

	//infos
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	hb_widget_set_margin(GTK_WIDGET(hbox), SPACING_SMALL);
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
		treeview = lst_reptime_create();
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
	g_signal_connect (window, "delete-event", G_CALLBACK (reptime_window_dispose), (gpointer)data);
	g_signal_connect (window, "map-event"   , G_CALLBACK (reptime_window_mapped), NULL);

	// setup, init and show window
	wg = &PREFS->tme_wg;
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
	hb_widget_visible(data->GR_detail, data->detail);

	return window;
}


/* = = = = = = = = = = = = = = = = */


static GString *lst_reptime_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
const gchar *format;

	node = g_string_new(NULL);

	// header
	format = (clipboard == TRUE) ? "%s\t%s\n" : "%s;%s\n";
	g_string_append_printf(node, format, (title == NULL) ? _("Time slice") : title, _("Amount"));

	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	gchar *name;
	gdouble amount;

		gtk_tree_model_get (model, &iter,
			//LST_HUBREPTIME_KEY, i,
			LST_HUBREPTIME_LABEL  , &name,
			LST_HUBREPTIME_TOTAL , &amount,
			-1);

		format = (clipboard == TRUE) ? "%s\t%.2f\n" : "%s;%.2f\n";
		g_string_append_printf(node, format, name, amount);

		//leak
		g_free(name);

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}


static void lst_reptime_cell_data_function_amount (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gdouble value;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gint colid = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter, colid, &value, -1);

	//#2091004 we have exact 0.0, do we force display ?
	if( (hb_amount_cmp(value, 0.0) != 0) || (colid == LST_HUBREPTIME_TOTAL) )
	{
	guint32 kcur = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(gtk_tree_view_column_get_tree_view(col)), "kcur_data"));

		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, kcur, GLOBALS->minor);

		g_object_set(renderer,
			"foreground",  get_normal_color_amount(value),
			"text", buf,
			NULL);
	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}
}


static GtkTreeViewColumn *lst_reptime_column_create_amount(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_reptime_cell_data_function_amount, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}


static gint lst_reptime_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
gint pos1, pos2;
gdouble val1, val2;

	gtk_tree_model_get(model, a,
		LST_HUBREPTIME_POS, &pos1,
		sortcol, &val1,
		-1);
	gtk_tree_model_get(model, b,
		LST_HUBREPTIME_POS, &pos2,
		sortcol, &val2,
		-1);
/*
	if(pos1 == -1) return(1);
	if(pos2 == -1) return(-1);
*/

	if(sortcol == LST_HUBREPTIME_POS)
		retval = pos2 - pos1;
	else
		retval = (ABS(val1) - ABS(val2)) > 0 ? 1 : -1;

	DB( g_print(" sort %d=%d or %.2f=%.2f :: %d\n", pos1,pos2, val1, val2, retval) );

    return retval;
}


static void lst_reptime_set_cur(GtkTreeView *treeview, guint32 kcur)
{
	g_object_set_data(G_OBJECT(treeview), "kcur_data", GUINT_TO_POINTER(kcur));
}


/*
** create our statistic list
*/
static GtkWidget *lst_reptime_create(void)
{
GtkListStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	/* create list store */
	store = gtk_list_store_new(
	  	NUM_LST_HUBREPTIME,
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
	//gtk_tree_view_column_set_title(column, _("Time slice"));
	renderer = gtk_cell_renderer_text_new();
	//#2004631 date and column title alignement
	//g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_set_cell_data_func(column, renderer, ope_result_cell_data_function, NULL, NULL);
	gtk_tree_view_column_add_attribute(column, renderer, "text", LST_HUBREPTIME_LABEL);
	//gtk_tree_view_column_set_sort_column_id (column, LST_HUBREPTIME_NAME);
	gtk_tree_view_column_set_resizable(column, TRUE);

	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	//gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Expense */
	column = lst_reptime_column_create_amount(_("Expense"), LST_HUBREPTIME_EXPENSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Income */
	column = lst_reptime_column_create_amount(_("Income"), LST_HUBREPTIME_INCOME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Total */
	column = lst_reptime_column_create_amount(_("Total"), LST_HUBREPTIME_TOTAL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

  /* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* sort */
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_HUBREPTIME_POS   , lst_reptime_compare_func, GINT_TO_POINTER(LST_HUBREPTIME_POS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_HUBREPTIME_TOTAL, lst_reptime_compare_func, GINT_TO_POINTER(LST_HUBREPTIME_TOTAL), NULL);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), FALSE);

	return(view);
}


