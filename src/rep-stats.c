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

#include "rep-stats.h"

#include "list-report.h"
#include "list-operation.h"
#include "gtk-chart.h"
#include "gtk-dateentry.h"

#include "dsp-mainwindow.h"
#include "ui-filter.h"
#include "ui-transaction.h"

#define HB_STATS_DO_TOTAL 1
#define HB_STATS_DO_TIME  1


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
static void repstats_range_change(GtkWidget *widget, gpointer user_data);
static void repstats_filter_setup(struct repstats_data *data);
static void repstats_compute(GtkWidget *widget, gpointer user_data);
static void repstats_sensitive(GtkWidget *widget, gpointer user_data);
static void repstats_update_daterange(GtkWidget *widget, gpointer user_data);
static void repstats_update_date_widget(GtkWidget *widget, gpointer user_data);
static void repstats_selection(GtkTreeSelection *treeselection, gpointer user_data);
static void repstats_selection2(GtkTreeSelection *treeselection, gpointer user_data);


extern HbKvData CYA_REPORT_SRC[];
extern HbKvData CYA_REPORT_TYPE[];

extern gchar *CYA_REPORT_MODE[];
extern HbKvData CYA_REPORT_INTVL[];


/* action functions -------------------- */




static void repstats_action_viewlist(GtkRadioButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;
gint tmpmode;

	tmpmode  = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));

	hb_widget_visible (data->SW_total, tmpmode ? FALSE : TRUE);
	hb_widget_visible (data->SW_trend, tmpmode ? TRUE : FALSE);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 0);
	data->charttype = 0;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_mode_changed(GtkRadioButton *radiobutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	//ignore event triggered from inactive radiobutton
	if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radiobutton)) == FALSE )
		return;
	
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->BT_list), TRUE);
	repstats_action_viewlist(radiobutton, data);
	
	if( data->detail == TRUE )
	{
	gint tmpmode  = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));
	
		if( tmpmode == 0 )
			repstats_selection(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), NULL);
		else
			repstats_selection2(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report2)), NULL);

	}
}


static void repstats_action_viewbar(GtkToolButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	gtk_chart_set_type (GTK_CHART(data->RE_chart), CHART_TYPE_COL);
	data->charttype = CHART_TYPE_COL;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_viewpie(GtkToolButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	gtk_chart_set_type (GTK_CHART(data->RE_chart), CHART_TYPE_PIE);
	data->charttype = CHART_TYPE_PIE;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_viewstack(GtkToolButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 2);
	gtk_chart_set_type (GTK_CHART(data->RE_chart2), CHART_TYPE_STACK);
	data->charttype = CHART_TYPE_STACK;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_viewstack100(GtkToolButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 2);
	gtk_chart_set_type (GTK_CHART(data->RE_chart2), CHART_TYPE_STACK100);
	data->charttype = CHART_TYPE_STACK100;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_print(GtkToolButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;
gint tmpsrc, tmpmode;
gchar *name;

	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));
	tmpmode  = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));
	
	name = g_strdup_printf("hb-repstat_%s", hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

	if( tmpmode == 0 )
		gtk_chart_print(GTK_CHART(data->RE_chart), GTK_WINDOW(data->window), PREFS->path_export, name);
	else
		gtk_chart_print(GTK_CHART(data->RE_chart2), GTK_WINDOW(data->window), PREFS->path_export, name);

	g_free(name);
}


static void repstats_action_filter(GtkToolButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;
gint response_id;

	DB( g_print("\n[repdist] filter\n") );

	//debug
	//create_deffilter_window(data->filter, TRUE);
	response_id = ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, TRUE, FALSE);
	if( response_id != GTK_RESPONSE_REJECT)
	{
		if( response_id == 55 )	//reset
		{
			DB( g_print(" reset filter\n") );
			repstats_filter_setup(data);
		}
		else
		{
			if( data->filter->range == FLT_RANGE_MISC_CUSTOM )
			{
				filter_preset_daterange_set(data->filter, data->filter->range, 0);
			}
		}

		g_signal_handler_block(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), data->filter->range);
		g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);


		repstats_compute(data->window, NULL);
		repstats_update_date_widget(data->window, NULL);
		repstats_update_daterange(data->window, NULL);
	}
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */



static void repstats_date_change(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	DB( g_print("\n[repdist] date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	// set min/max date for both widget
	gtk_date_entry_set_maxdate(GTK_DATE_ENTRY(data->PO_mindate), data->filter->maxdate);
	gtk_date_entry_set_mindate(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->mindate);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);

	repstats_compute(widget, NULL);
	repstats_update_daterange(widget, NULL);

}


static void repstats_range_change(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gint range;

	DB( g_print("\n[repdist] range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_range));

	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, 0);

		repstats_update_date_widget(data->window, NULL);

		repstats_compute(data->window, NULL);
		repstats_update_daterange(data->window, NULL);
	}
	//TODO: this can never happen anymore
	else
	{
		if(ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, TRUE, FALSE) != GTK_RESPONSE_REJECT)
		{
			repstats_update_date_widget(data->window, NULL);
			repstats_compute(data->window, NULL);
			repstats_update_daterange(data->window, NULL);
		}
	}
}


static void repstats_update(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gboolean byamount;
GtkTreeModel		 *model;
gint tmpsrc, tmptype, usrcomp, column;
gboolean xval;
gchar *title;

	DB( g_print("\n[repdist] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	byamount = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_byamount));
	usrcomp = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_compare));
	tmptype = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_type));
	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));
	
	//debug option
	DB( g_print(" option: byamount=%d tmptype=%d '%s' tmpsrc=%d '%s'\n\n", byamount, tmptype, hbtk_get_label(CYA_REPORT_TYPE,tmptype), tmpsrc, hbtk_get_label(CYA_REPORT_SRC,tmpsrc)) );

	// define view/sort column
	column = LST_REPDIST_POS; 

	if( byamount )
	{
		switch( tmptype )
		{
			case REPORT_TYPE_TOTAL:
				column = LST_REPDIST_TOTAL;
				break;
			case REPORT_TYPE_EXPENSE:
				column = LST_REPDIST_EXPENSE;
				break;
			case REPORT_TYPE_INCOME:
				column = LST_REPDIST_INCOME;
				break;
		}
	}

	DB( g_print(" sort on column %d\n\n", column) );
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), column, GTK_SORT_ASCENDING);


	gtk_chart_set_color_scheme(GTK_CHART(data->RE_chart), PREFS->report_color_scheme);

	//TRANSLATORS: example 'Expense by Category'
	title = g_strdup_printf(_("%s by %s"), hbtk_get_label(CYA_REPORT_TYPE,tmptype), hbtk_get_label(CYA_REPORT_SRC, tmpsrc) );

	/* update absolute or not */
	gboolean abs = (tmptype == REPORT_TYPE_EXPENSE || tmptype == REPORT_TYPE_INCOME) ? TRUE : FALSE;
	gtk_chart_set_absolute(GTK_CHART(data->RE_chart), abs);

	/* show xval for month/year and no by amount display */
	xval = FALSE;

	if( !byamount && (tmpsrc == REPORT_SRC_MONTH || tmpsrc == REPORT_SRC_YEAR) )
	{
		xval = TRUE;
		/*switch( tmpsrc)
		{
			case REPORT_SRC_MONTH:
				gtk_chart_set_every_xval(GTK_CHART(data->RE_chart), 4);
				break;
			case REPORT_SRC_YEAR:
				gtk_chart_set_every_xval(GTK_CHART(data->RE_chart), 2);
				break;
		}*/
	}

	gtk_chart_show_xval(GTK_CHART(data->RE_chart), xval);

	/* update bar chart */
	if( (tmptype == REPORT_TYPE_TOTAL) && (usrcomp == TRUE) ) //dual exp/inc
	{
		DB( g_print(" set bar to dual exp %d/inc %d\n\n", LST_REPDIST_EXPENSE, LST_REPDIST_INCOME) );
		gtk_chart_set_datas_total(GTK_CHART(data->RE_chart), model, LST_REPDIST_EXPENSE, LST_REPDIST_INCOME, title, NULL);
	}
	else
	{
		switch(tmptype)
		{
			case REPORT_TYPE_EXPENSE: 
				column = LST_REPDIST_EXPENSE;
				break;
			case REPORT_TYPE_INCOME: 
				column = LST_REPDIST_INCOME;
				break;
			default:
			case REPORT_TYPE_TOTAL:
				column = LST_REPDIST_TOTAL;
				break;
		} 
		
		DB( g_print(" set bar to %d\n\n", column) );
		gtk_chart_set_datas_total(GTK_CHART(data->RE_chart), model, column, column, title, NULL);
	}

	//time chart
	gtk_chart_set_color_scheme(GTK_CHART(data->RE_chart2), PREFS->report_color_scheme);
	gtk_chart_show_xval(GTK_CHART(data->RE_chart2), TRUE);
	gtk_chart_set_datas_time(GTK_CHART(data->RE_chart2), GTK_TREE_VIEW(data->LV_report2), data->trendrows, data->trendcols, NULL, NULL);

	g_free(title);
	
}


static void repstats_update_date_widget(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	DB( g_print("\n[repdist] update date widget\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	g_signal_handler_block(data->PO_mindate, data->handler_id[HID_REPDIST_MINDATE]);
	g_signal_handler_block(data->PO_maxdate, data->handler_id[HID_REPDIST_MAXDATE]);
	
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);
	
	g_signal_handler_unblock(data->PO_mindate, data->handler_id[HID_REPDIST_MINDATE]);
	g_signal_handler_unblock(data->PO_maxdate, data->handler_id[HID_REPDIST_MAXDATE]);

}


static void repstats_update_daterange(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gchar *daterange;

	DB( g_print("\n[repdist] update daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	daterange = filter_daterange_text_get(data->filter);
	gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
	g_free(daterange);
}

static void repstats_update_total(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
//gboolean minor;

	DB( g_print("\n[repdist] update total\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	hb_label_set_colvalue(GTK_LABEL(data->TX_total[0]), data->total_expense, GLOBALS->kcur, GLOBALS->minor);
	hb_label_set_colvalue(GTK_LABEL(data->TX_total[1]), data->total_income, GLOBALS->kcur, GLOBALS->minor);
	hb_label_set_colvalue(GTK_LABEL(data->TX_total[2]), data->total_expense + data->total_income, GLOBALS->kcur, GLOBALS->minor);


}


static void repstats_export_result_clipboard(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
GtkClipboard *clipboard;
GString *node;
gchar *title, *catsubcat;
gint tmpmode, tmpsrc;

	DB( g_print("\n[repdist] export result clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpmode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));

	//#1886299/1900281
	catsubcat = NULL;
	title = hbtk_get_label(CYA_REPORT_SRC,tmpsrc);
	if(tmpsrc == REPORT_SRC_SUBCATEGORY)
	{
		catsubcat = g_strjoin(":",hbtk_get_label(CYA_REPORT_SRC, REPORT_SRC_CATEGORY), hbtk_get_label(CYA_REPORT_SRC, REPORT_SRC_SUBCATEGORY), NULL); 
		title = catsubcat;
	}

	if( tmpmode == 0 )
		node = lst_rep_total_to_string(GTK_TREE_VIEW(data->LV_report), title, TRUE);
	else
		node = lst_rep_time_to_string(GTK_TREE_VIEW(data->LV_report2), title, TRUE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
	
	g_free(catsubcat);
}


static void repstats_export_result_csv(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gchar *filename = NULL;
GString *node;
GIOChannel *io;
gchar *name, *title, *catsubcat;
gint tmpmode, tmpsrc;

	DB( g_print("\n[repdist] export result csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpmode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));
	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));
	
	name = g_strdup_printf("hb-repstat_%s.csv", hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

	if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, name) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );
		io = g_io_channel_new_file(filename, "w", NULL);
		if(io != NULL)
		{

			//#1886299/1900281
			catsubcat = NULL;
			title = hbtk_get_label(CYA_REPORT_SRC,tmpsrc);
			if(tmpsrc == REPORT_SRC_SUBCATEGORY)
			{
				catsubcat = g_strjoin(":",hbtk_get_label(CYA_REPORT_SRC, REPORT_SRC_CATEGORY), hbtk_get_label(CYA_REPORT_SRC, REPORT_SRC_SUBCATEGORY), NULL); 
				title = catsubcat;
			}

			if( tmpmode == 0 )
				node = lst_rep_total_to_string(GTK_TREE_VIEW(data->LV_report), title, FALSE);
			else
				node = lst_rep_time_to_string(GTK_TREE_VIEW(data->LV_report2), title, FALSE);

			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
			g_free(catsubcat);
		}
		g_free( filename );
	}
	g_free(name);
}


static void repstats_export_detail_clipboard(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
GtkClipboard *clipboard;
GString *node;

	DB( g_print("\n[repdist] export detail clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), TRUE, FALSE, TRUE, FALSE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void repstats_export_detail_csv(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gchar *filepath = NULL;
GString *node;
GIOChannel *io;
gchar *name;
gint tmpsrc;
gboolean hassplit, hasstatus;

	DB( g_print("\n[repdist] export detail csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));
	name = g_strdup_printf("hb-repstat-detail_%s.csv", hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

	filepath = g_build_filename(PREFS->path_export, name, NULL);

	//#2019312
	//if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, name) == TRUE )
	if( ui_dialog_export_csv(GTK_WINDOW(data->window), &filepath, &hassplit, &hasstatus, FALSE) == GTK_RESPONSE_ACCEPT )
	{
		DB( g_print(" + filepath is %s\n", filepath) );

		io = g_io_channel_new_file(filepath, "w", NULL);
		if(io != NULL)
		{
			node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), FALSE, hassplit, hasstatus, FALSE);
			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);

			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
	}

	g_free( filepath );
	g_free(name);
}


static void repstats_detail(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
guint active = GPOINTER_TO_INT(user_data);
guint tmpsrc;
GList *list;
GtkTreeModel *model;
GtkTreeIter  iter, child;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repdist] detail\n") );

	/* clear and detach our model */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

	if(data->detail)
	{
		//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
		tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));

		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), NULL); /* Detach model from view */

		/* fill in the model */
		list = g_queue_peek_head_link(data->txn_queue);
		while (list != NULL)
		{
		Transaction *ope = list->data;
		gdouble dtlamt = ope->amount;

			if(filter_txn_match(data->filter, ope) == 1)
			{
			gboolean match = FALSE;
			guint i, pos = 0;

				if( tmpsrc != REPORT_SRC_TAG )
				{
					if( (tmpsrc == REPORT_SRC_CATEGORY || tmpsrc == REPORT_SRC_SUBCATEGORY) && ope->flags & OF_SPLIT )
					{
					guint nbsplit = da_splits_length(ope->splits);
					Split *split;

						dtlamt = 0.0;
						for(i=0;i<nbsplit;i++)
						{
							split = da_splits_get(ope->splits, i);
							pos = category_report_id(split->kcat, (tmpsrc == REPORT_SRC_SUBCATEGORY) ? TRUE : FALSE);
							if( pos == active )
							{
								match = TRUE;
								dtlamt += split->amount;
								// no more break here as we need to compute split 4 cat
								//break;
							}
						}
					}
					else
					{
						pos = report_items_get_pos(tmpsrc, data->filter->mindate, ope);
						if( pos == active )
						{
							match = TRUE;
						}
					}
				}
				else
				/* the TAG process is particular */
				{
					if(ope->tags != NULL)
					{
					guint32 *tptr = ope->tags;

						while(*tptr)
						{
							pos = *tptr - 1;

							DB( g_print(" -> storing tag %d %.2f\n", pos, ope->amount) );

							if( pos == active )
							{
								match = TRUE;
								break;
							}
							tptr++;
						}
					}
				}

				if( match == TRUE )
				{
					gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &iter, NULL, -1,
					    MODEL_TXN_POINTER, ope,
						MODEL_TXN_SPLITAMT, dtlamt,
						-1);

					//#1875801 show split detail
					if( ope->flags & OF_SPLIT )
					{
					guint nbsplit = da_splits_length(ope->splits);
					Split *split;
					gboolean sinsert = TRUE;

						for(i=0;i<nbsplit;i++)
						{
							sinsert = TRUE;
							split = da_splits_get(ope->splits, i);
							if( (tmpsrc == REPORT_SRC_CATEGORY || tmpsrc == REPORT_SRC_SUBCATEGORY) )
							{
								pos = category_report_id(split->kcat, (tmpsrc == REPORT_SRC_SUBCATEGORY) ? TRUE : FALSE);
								DB( g_print("  %d =? %d => %d\n", pos, active, pos != active ? FALSE : TRUE) );
								if( pos != active )
								{
									sinsert = FALSE;
								}
							}
												
							if( sinsert == TRUE )							
							{
								gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &child, &iter, -1,
									MODEL_TXN_POINTER, ope,
									MODEL_TXN_SPLITPTR, split,
									-1);
							}
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
		//gtk_tree_view_expand_all( GTK_TREE_VIEW(data->LV_detail) );

	}

}


static void repstats_compute(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
DataTable *dt;
gint tmpsrc, tmptype, tmpintvl;
gdouble totexp, totinc, ratetotal;
guint n_result, n_cols;
guint i, n_inserted;
GtkTreeModel *model;
GtkTreeIter  iter;

	DB( g_print("\n[repdist] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//#2019876 return is invalid date range
	if( data->filter->maxdate < data->filter->mindate )
		return;

	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));
	tmptype = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_type));

	//beta
	tmpintvl = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_intvl));

	g_queue_free (data->txn_queue);
	data->txn_queue = hbfile_transaction_get_partial(data->filter->mindate, data->filter->maxdate);
	
	DB( g_print(" for=%d,kind=%d\n", tmpsrc, tmptype) );
	DB( g_print(" nb-txn=%d\n", g_queue_get_length (data->txn_queue) ) );

	//todo: remove this later on
	n_result = report_items_count(tmpsrc, data->filter->mindate, data->filter->maxdate);
	n_cols   = report_interval_count(tmpintvl, data->filter->mindate, data->filter->maxdate);
	DB( g_print(" %s :: n_result=%d, n_cols=%d\n", hbtk_get_label(CYA_REPORT_SRC,tmpsrc), n_result, n_cols) );

	#if HB_STATS_DO_TOTAL == 1
//TODO: add if mode==0
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	n_inserted = 0;

	totexp = totinc = 0.0;

	dt = report_compute_total(tmpsrc, data->filter, data->txn_queue);
	if(dt)
	{
		data->total_expense = dt->totexp;
		data->total_income  = dt->totinc;
		//total_total = dt->totexp + dt->totinc;
		//#2018145 rate total must remains ABS to get accurate %
		ratetotal   = ABS(dt->totexp) + ABS(dt->totinc);

		DB( g_printf(" total exp %9.2f\n", dt->totexp) );
		DB( g_printf(" total inc %9.2f\n", dt->totinc) );
		DB( g_printf(" total tot %9.2f\n", total_total) );

		/* clear and detach our model */
		gtk_list_store_clear (GTK_LIST_STORE(model));
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), NULL); /* Detach model from view */

		DB( g_print("\npopulate total listview\n") );

		/* insert into the treeview */
		for(i=0; i<n_result; i++)
		{
		DataRow *dr = dt->rows[i];
		gdouble drtotal;

			//DB( g_print("try to insert item %d - %.2f %.2f\n", i, dr->expense[0], dr->income[1]) );

			/* skip empty results */
			if( !(tmpsrc == REPORT_SRC_MONTH || tmpsrc == REPORT_SRC_YEAR) )
			{
				if( tmptype == REPORT_TYPE_EXPENSE && !dr->expense[0] ) continue;
				if( tmptype == REPORT_TYPE_INCOME && !dr->income[1] ) continue;
				if( !dr->expense[0] && !dr->income[1] ) continue;
			}

			/* skip no account */ 
			if( tmpsrc == REPORT_SRC_ACCOUNT && i == 0 )
				continue;

			n_inserted++;
			totexp += dr->expense[0];
			totinc += dr->income[1];
			drtotal = dr->expense[0] + dr->income[1];
			
	    	gtk_list_store_insert_with_values (GTK_LIST_STORE(model), &iter, -1,
		        LST_REPDIST_POS , dr->pos,
		        LST_REPDIST_KEY , i,
				LST_REPDIST_NAME, dr->label,
				LST_REPDIST_EXPENSE, dr->expense[0],
				LST_REPDIST_INCOME , dr->income[1],
				LST_REPDIST_TOTAL  , drtotal,
				//#1925976 abs is used into hb_rate, no need to - expense
				//#1942494 but when sum exp+inc, exp must be abs !
				//#1988489 total must be computed with sign
				LST_REPDIST_EXPRATE, hb_rate(dr->expense[0], data->total_expense),
				LST_REPDIST_INCRATE, hb_rate(dr->income[1] , data->total_income),
				LST_REPDIST_TOTRATE, hb_rate(drtotal, ratetotal),
				-1);

			DB( g_printf("\n insert %2d, '%s', %9.2f  %9.2f  %9.2f\n", i, dr->label, 
				dr->expense[0], dr->income[1], drtotal) );

			DB( g_printf(" rates  %2d, '%s', %9.2f%% %9.2f%% %9.2f%%\n", i, dr->label, 
				hb_rate(dr->expense[0], data->total_expense), 
				hb_rate(dr->income[1] , data->total_income), 
				hb_rate(drtotal, ratetotal)) );
		}

		/* update column 0 title */
		GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report), 0);
		gtk_tree_view_column_set_title(column, hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));
		
		/* Re-attach model to view */
  		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
		g_object_unref(model);
	}

	da_datatable_free (dt);

	//insert total
	if( n_inserted > 1 )
	{
		
		gtk_list_store_insert_with_values (GTK_LIST_STORE(model), &iter, -1,
		    LST_REPDIST_POS, LST_REPDIST_POS_TOTAL,
		    LST_REPDIST_KEY, -1,
			LST_REPDIST_NAME, _("Total"),
			LST_REPDIST_EXPENSE, totexp,
			LST_REPDIST_INCOME , totinc,
			LST_REPDIST_TOTAL, totexp + totinc,
			//#1942494 added for 5.5.4
			LST_REPDIST_EXPRATE, hb_rate(-totexp, -data->total_expense),
			LST_REPDIST_INCRATE, hb_rate(totinc , data->total_income),
			LST_REPDIST_TOTRATE, hb_rate(-totexp + totinc, -data->total_expense + data->total_income),
			-1);
	}
	#endif

	/* ---- trend with test multi to remove ---- */

	//TODO: add if mode==1 && choose a better max column value
	#if HB_STATS_DO_TIME == 1
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report2));
	gtk_tree_store_clear (GTK_TREE_STORE(model));
	n_inserted = 0;
	if( n_cols > 365*4 )
	{

		//lst_rep_time_renewcol(GTK_TREE_VIEW(data->LV_report2), 0, data->filter->mindate, tmpintvl);

		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
		_("Too much columns to display"),
		_("Please select a wider interval and / or a narrower date range")
		);
	}
	else
	{
	gboolean tmpaccbal;

		DB( g_print("\n[repdist] compute test multi\n") );
		
		if(data->trend)
			da_datatable_free (data->trend);

		//beta
		tmpaccbal = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_balance));
		if( tmpsrc != REPORT_SRC_ACCOUNT )
			tmpaccbal = FALSE;

		if(!tmpaccbal)
			//not beta
			dt = report_compute_trend(tmpsrc, tmpintvl, data->filter, data->txn_queue);
		else
			dt = report_compute_trend_balance(tmpsrc, tmpintvl, data->filter);
		//beta end

		data->trend = dt;

		DB( g_print("\npopulate time listview\n") );

		/* clear and detach our model */
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report2), NULL); /* Detach model from view */

		lst_rep_time_renewcol(GTK_TREE_VIEW(data->LV_report2), n_cols, data->filter->mindate, tmpintvl, tmpaccbal ? FALSE : TRUE);

		/* update column 0 title */
		GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report2), 0);
		gtk_tree_view_column_set_title(column, hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

		//#1955046 treeview with child was a test faulty released
		for(i=0; i<n_result; i++)
		{
		DataRow *dr = dt->rows[i];
		gdouble total = da_datarow_get_cell_sum(dr, dt->nbcols+1);

			DB( g_printf(" eval item %4d:'%s' %.2f\n", i, dr->label, total ) );

			//#2024940 test on exp/inc individually
			if( hb_amount_equal(total, 0.0) )
			{
				DB( g_printf("  hide because no data\n") );
				continue;
			}

			DB( g_printf(" --> insert\n") );
			n_inserted++;
			gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, -1,
				LST_REPDIST2_KEY, i,
				LST_REPDIST2_LABEL, dr->label,
				LST_REPDIST2_ROW, dr,
				-1);
		}

		data->trendrows = n_inserted;
		data->trendcols = n_cols;

		//insert total
		if( n_inserted > 1 )
		{
		DataRow *dr = dt->totrow;

			dr->pos = LST_REPDIST_POS_TOTAL;

			DB( g_printf(" eval item total:'%s'\n", dr->label ) );

			DB( g_printf(" --> insert total\n") );
			n_inserted++;
			gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, -1,
				LST_REPDIST2_POS, LST_REPDIST_POS_TOTAL,
				LST_REPDIST2_KEY, -1,
				LST_REPDIST2_ROW, dr,
				LST_REPDIST2_LABEL, _("Total"),
				-1);
		}

		/* Re-attach model to view */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report2), model);
		g_object_unref(model);

		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), LST_REPDIST2_POS, GTK_SORT_DESCENDING);


		/* test liststore */
		//liststore_benchmark();

		/* test multi end */

	}
	#endif

	//here we use data->total_expense/income
	repstats_update_total(widget, NULL);
	repstats_update(widget, user_data);
		
}


/*
** update visibility/sensitivity
*/
static void repstats_sensitive(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gboolean visible, sensitive;
gint mode, type, tmpsrc, page;

	DB( g_print("\n[repdist] sensitive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	mode = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));
	type = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_type));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX_TEXT(data->CY_src));
	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));
	
	//total
	visible = mode == 0 ? TRUE : FALSE;
	hb_widget_visible(data->LB_type, visible);
	hb_widget_visible(data->CY_type, visible);
	hb_widget_visible(data->CM_byamount, visible);
	hb_widget_visible(data->CM_compare, visible);
	hb_widget_visible(data->BT_column, visible);
	hb_widget_visible(data->BT_donut, visible);
	hb_widget_visible(data->BT_stack, !visible);
	hb_widget_visible(data->BT_stack100, !visible);
	hb_widget_visible(data->LB_intvl, !visible);
	hb_widget_visible(data->CY_intvl, !visible);

	visible = (mode==0 && page == 0) ? TRUE : FALSE;
	hb_widget_visible(data->BT_rate, visible);

	visible = ((mode == 1) && (tmpsrc == REPORT_SRC_ACCOUNT)) ? TRUE : FALSE;
	hb_widget_visible(data->CM_balance, visible);

	//zoom
	visible = (data->charttype == CHART_TYPE_COL 
			|| data->charttype == CHART_TYPE_STACK 
			|| data->charttype == CHART_TYPE_STACK100) ? TRUE : FALSE;
	hb_widget_visible(data->LB_zoomx, visible);
	hb_widget_visible(data->RG_zoomx, visible);

	//compare
	visible = (type == REPORT_TYPE_TOTAL) && (data->charttype == CHART_TYPE_COL) ? TRUE : FALSE; 
	hb_widget_visible(data->CM_compare, visible);

	visible = page == 0 ? TRUE : FALSE;
	hb_widget_visible (data->BT_detail, visible);
	hb_widget_visible (data->BT_export, visible);

	visible = (page > 0) ? TRUE : FALSE;
	hb_widget_visible (data->BT_legend, visible);
	hb_widget_visible (data->BT_print, visible);
	
	page = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail)), NULL);
	sensitive = ((page > 0) && data->detail) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->MI_detailtoclip, sensitive);
	gtk_widget_set_sensitive(data->MI_detailtocsv, sensitive);
}


static void repstats_detail_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
struct repstats_data *data;
Transaction *active_txn;
gboolean result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print ("\n[repdist] A detail row has been double-clicked!\n") );

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
			gint tmpmode  = hbtk_radio_button_get_active(GTK_CONTAINER(data->RA_mode));
			
			treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW(tmpmode == 0 ? data->LV_report : data->LV_report2));
			if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
			{
				path = gtk_tree_model_get_path(model, &iter);
			}

			//#1640885
			GLOBALS->changes_count++;
			repstats_compute(data->window, NULL);

			if( path != NULL )
			{
				gtk_tree_selection_select_path(treeselection, path);
				gtk_tree_path_free(path);
			}
		}

		da_transaction_free (old_txn);
	}
}


static void repstats_update_detail(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	DB( g_print("\n[repdist] update detail\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if(GTK_IS_TREE_VIEW(data->LV_report))
	{
		DB( g_print(" showdetail=%d\n", data->detail) );

		//#2018039
		list_txn_set_lockreconciled(GTK_TREE_VIEW(data->LV_detail), PREFS->lockreconciled);

		if(data->detail)
		{
		GtkTreeSelection *treeselection;
		GtkTreeModel *model;
		GtkTreeIter iter;
		guint key;

			treeselection = gtk_tree_view_get_selection (GTK_TREE_VIEW(data->LV_report));

			if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
			{
				gtk_tree_model_get(model, &iter, LST_REPDIST_KEY, &key, -1);

				DB( g_print(" - active is %d\n", key) );

				repstats_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
			}



			gtk_widget_show(data->GR_detail);
		}
		else
			gtk_widget_hide(data->GR_detail);

	}
}




static void repstats_toggle_detail(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->detail ^= 1;

	DB( g_print("\n[repdist] toggledetail to %d\n", data->detail) );

	repstats_update_detail(widget, user_data);

}

/*
** change the chart legend visibility
*/
static void repstats_toggle_legend(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	DB( g_print("\n[repdist] toggle legend\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->legend ^= 1;

	gtk_chart_show_legend(GTK_CHART(data->RE_chart), data->legend, FALSE);
	gtk_chart_queue_redraw(GTK_CHART(data->RE_chart));
	gtk_chart_show_legend(GTK_CHART(data->RE_chart2), data->legend, FALSE);
	gtk_chart_queue_redraw(GTK_CHART(data->RE_chart2));
}

static void repstats_zoomx_callback(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gdouble value;

	DB( g_print("\n[repdist] zoomx\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	value = gtk_range_get_value(GTK_RANGE(data->RG_zoomx));

	DB( g_print(" + scale is %.2f\n", value) );

	gtk_chart_set_barw(GTK_CHART(data->RE_chart), value);
	gtk_chart_set_barw(GTK_CHART(data->RE_chart2), value);

}


/*
** change the chart rate columns visibility
*/
static void repstats_toggle_rate(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
GtkTreeViewColumn *column;

	DB( g_print("\n[repdist] toggle rate\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->rate ^= 1;

	if(GTK_IS_TREE_VIEW(data->LV_report))
	{

		column = gtk_tree_view_get_column (GTK_TREE_VIEW(data->LV_report), 2);
		gtk_tree_view_column_set_visible(column, data->rate);

		column = gtk_tree_view_get_column (GTK_TREE_VIEW(data->LV_report), 4);
		gtk_tree_view_column_set_visible(column, data->rate);

		column = gtk_tree_view_get_column (GTK_TREE_VIEW(data->LV_report), 6);
		gtk_tree_view_column_set_visible(column, data->rate);
	}

}

static void repstats_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	DB( g_print("\n[repdist] toggle minor\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	repstats_update_total(widget,NULL);

	//hbfile_update(data->LV_acc, (gpointer)4);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	gtk_chart_show_minor(GTK_CHART(data->RE_chart), GLOBALS->minor);
	gtk_chart_queue_redraw(GTK_CHART(data->RE_chart));
	gtk_chart_show_minor(GTK_CHART(data->RE_chart2), GLOBALS->minor);
	gtk_chart_queue_redraw(GTK_CHART(data->RE_chart2));
}


static void repstats_cb_sortcolumnchanged(GtkTreeSortable *sortable, gpointer user_data)
{
	DB( g_print("\n[repdist] sort column chnaged\n") );




}


static void repstats_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter iter;
guint key = -1;

	DB( g_print("\n[repdist] selection total\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_REPDIST_KEY, &key, -1);
	}

	DB( g_print(" - total active is %d\n", key) );

	repstats_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
	repstats_sensitive(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static void repstats_selection2(GtkTreeSelection *treeselection, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter iter;
guint key = -1;

	DB( g_print("\n[repdist] selection time\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_REPDIST2_KEY, &key, -1);
	}

	DB( g_print(" - time active is %d\n", key) );

	repstats_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
	repstats_sensitive(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static GtkWidget *
repstats_toolbar_create(struct repstats_data *data)
{
GtkWidget *toolbar, *button;

	toolbar = gtk_toolbar_new();

	button = (GtkWidget *)gtk_radio_tool_button_new(NULL);
	data->BT_list = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_LIST, "label", _("List"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as list"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_column = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_COLUMN, "label", _("Column"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as column"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_donut = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_DONUT, "label", _("Donut"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as donut"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_stack = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_STACK, "label", _("Stack"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as stack"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_stack100 = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_STACK100, "label", _("Stack 100%"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as stack 100%"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	button = gtk_widget_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
		"icon-name", ICONNAME_HB_OPE_SHOW,
	    "label", _("Detail"),
		"tooltip-text", _("Toggle detail"),
		"active", PREFS->stat_showdetail,
		NULL);
	data->BT_detail = button;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = gtk_widget_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
		"icon-name", ICONNAME_HB_SHOW_LEGEND,
	    "label", _("Legend"),
		"tooltip-text", _("Toggle legend"),
	    "active", TRUE,
		NULL);
	data->BT_legend = button;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = gtk_widget_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
		"icon-name", ICONNAME_HB_SHOW_RATE,
	    "label", _("Rate"),
		"tooltip-text", _("Toggle rate"),
	    "active", PREFS->stat_showrate,
		NULL);
	data->BT_rate = button;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);
	
	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_FILTER, _("Filter"), _("Edit filter"));
	data->BT_filter = button;
	
	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REFRESH, _("Refresh"), _("Refresh results"));
	data->BT_refresh = button;
	
	GtkWidget *menu, *menuitem;

	menu = gtk_menu_new ();
	//gtk_widget_set_halign (menu, GTK_ALIGN_END);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Result to clipboard"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repstats_export_result_clipboard), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Result to CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repstats_export_result_csv), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Detail to clipboard"));
	data->MI_detailtoclip = menuitem;
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repstats_export_detail_clipboard), data);

	menuitem = gtk_menu_item_new_with_mnemonic (_("_Detail to CSV"));
	data->MI_detailtocsv = menuitem;
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
	g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (repstats_export_detail_csv), data);

	gtk_widget_show_all (menu);


	
	button = gtk_menu_button_new();
	data->BT_export = button;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(button)), GTK_STYLE_CLASS_FLAT);

	//gtk_menu_button_set_direction (GTK_MENU_BUTTON(widget), GTK_ARROW_DOWN);
	//gtk_widget_set_halign (widget, GTK_ALIGN_END);
	GtkWidget *image = gtk_image_new_from_icon_name (ICONNAME_HB_FILE_EXPORT, GTK_ICON_SIZE_LARGE_TOOLBAR);
	g_object_set (button, "image", image, "popup", GTK_MENU(menu), NULL);

	GtkToolItem *toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), button);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem), -1);

	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_PRINT, _("Print"), _("Print"));
	data->BT_print = button;
	
	
	return toolbar;
}


//reset the filter
static void repstats_filter_setup(struct repstats_data *data)
{
	DB( g_print("\n[repdist] reset filter\n") );

	filter_reset(data->filter);
	filter_preset_daterange_set(data->filter, PREFS->date_range_rep, 0);

	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);

	//#1989211 option to inlcude xfer by default
	if( PREFS->stat_includexfer == FALSE )
	{
		/* 3.4 : make int transfer out of stats */
		data->filter->option[FLT_GRP_TYPE] = 2;
		//data->filter->type = FLT_TYPE_INTXFER;
		data->filter->typ_xfr = TRUE;
	}

}


// setup default for our object/memory
static void repstats_window_setup(struct repstats_data *data)
{
	DB( g_print("\n[repdist] setup\n") );

	DB( g_print(" init data\n") );
	
	repstats_filter_setup(data);


	//DB( g_print(" populate\n") );

	DB( g_print(" set widgets default\n") );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor), GLOBALS->minor);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_byamount), PREFS->stat_byamount);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_type), REPORT_TYPE_EXPENSE);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_range), PREFS->date_range_rep);

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail))), "minor", (gpointer)data->CM_minor);
	
	gtk_chart_set_smallfont (GTK_CHART(data->RE_chart), PREFS->rep_smallfont);
	gtk_chart_set_smallfont (GTK_CHART(data->RE_chart2), PREFS->rep_smallfont);

	repstats_toggle_rate(data->window, NULL);

	DB( g_print(" connect widgets signals\n") );
	
	g_signal_connect (data->CM_minor, "toggled", G_CALLBACK (repstats_toggle_minor), NULL);

    data->handler_id[HID_REPDIST_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (repstats_date_change), (gpointer)data);
    data->handler_id[HID_REPDIST_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (repstats_date_change), (gpointer)data);

	data->handler_id[HID_REPDIST_RANGE] = g_signal_connect (data->CY_range, "changed", G_CALLBACK (repstats_range_change), NULL);

    g_signal_connect (data->CY_src, "changed", G_CALLBACK (repstats_compute), (gpointer)data);

	//test multi
	hbtk_radio_button_connect (GTK_CONTAINER(data->RA_mode), "toggled", G_CALLBACK (repstats_action_mode_changed), (gpointer)data);

	g_signal_connect (data->CY_intvl, "changed", G_CALLBACK (repstats_compute), (gpointer)data);
	
    data->handler_id[HID_REPDIST_VIEW] = g_signal_connect (data->CY_type, "changed", G_CALLBACK (repstats_compute), (gpointer)data);

	g_signal_connect (data->RG_zoomx, "value-changed", G_CALLBACK (repstats_zoomx_callback), NULL);


	g_signal_connect (data->CM_balance, "toggled", G_CALLBACK (repstats_compute), NULL);
	g_signal_connect (data->CM_byamount, "toggled", G_CALLBACK (repstats_update), NULL);
	g_signal_connect (data->CM_compare, "toggled", G_CALLBACK (repstats_update), NULL);

	g_signal_connect (G_OBJECT (data->BT_list), "clicked", G_CALLBACK (repstats_action_viewlist), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_column), "clicked", G_CALLBACK (repstats_action_viewbar), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_donut), "clicked", G_CALLBACK (repstats_action_viewpie), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_stack), "clicked", G_CALLBACK (repstats_action_viewstack), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_stack100), "clicked", G_CALLBACK (repstats_action_viewstack100), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_detail), "clicked", G_CALLBACK (repstats_toggle_detail), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_legend), "clicked", G_CALLBACK (repstats_toggle_legend), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_rate), "clicked", G_CALLBACK (repstats_toggle_rate), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_filter), "clicked", G_CALLBACK (repstats_action_filter), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_refresh), "clicked", G_CALLBACK (repstats_compute), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_print), "clicked", G_CALLBACK (repstats_action_print), (gpointer)data);

	//export is a menu

	
	
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report )), "changed", G_CALLBACK (repstats_selection), NULL);
	g_signal_connect (gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report)), "sort-column-changed", G_CALLBACK (repstats_cb_sortcolumnchanged), NULL);

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report2)), "changed", G_CALLBACK (repstats_selection2), NULL);


	g_signal_connect (GTK_TREE_VIEW(data->LV_detail), "row-activated", G_CALLBACK (repstats_detail_onRowActivated), NULL);
	


}


static gboolean repstats_window_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repstats_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[repdist] window mapped\n") );

	//setup, init and show window
	repstats_window_setup(data);	
	repstats_compute(data->window, NULL);
	//repstats_update_daterange(data->window, NULL);

	data->mapped_done = TRUE;

	return FALSE;
}


static gboolean repstats_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repstats_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[repdist] dispose\n") );

	/* test multi */
	if(data->trend)
		da_datatable_free (data->trend);

	
	g_queue_free (data->txn_queue);

	da_flt_free(data->filter);

	g_free(data);

	//store position and size
	wg = &PREFS->sta_wg;
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
static void repstats_window_acquire(struct repstats_data *data)
{
	DB( g_print("\n[repdist] acquire\n") );

	data->txn_queue = g_queue_new ();
	data->filter = da_flt_malloc();
	data->detail = PREFS->stat_showdetail;
	data->legend = 1;
	data->rate = PREFS->stat_showrate^1;

}


// the window creation
GtkWidget *repstats_window_new(void)
{
struct repstats_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainvbox, *hbox, *vbox, *notebook, *treeview, *vpaned, *sw;
GtkWidget *label, *widget, *table, *entry;
gint row;

	DB( g_print("\n[repdist] new\n") );

	
	data = g_malloc0(sizeof(struct repstats_data));
	if(!data) return NULL;

	repstats_window_acquire(data);

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

	gtk_window_set_title (GTK_WINDOW (window), _("Statistics Report"));

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

	row++;
	label = make_label_widget(_("Mode:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_radio_button_new(GTK_ORIENTATION_HORIZONTAL, CYA_REPORT_MODE, TRUE);
	data->RA_mode = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);
	
	row++;
	label = make_label_widget(_("_View by:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	//widget = make_cycle(label, CYA_REPORT_SRC);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_SRC);
	data->CY_src = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_Type:"));
	data->LB_type = label;
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	//widget = make_cycle(label, CYA_REPORT_TXN_TYPE);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_TYPE);
	data->CY_type = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("_Balance mode"));
	data->CM_balance = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Inter_val:"));
	data->LB_intvl = label;
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	//widget = make_cycle(label, CYA_REPORT_INTVL);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_INTVL);
	data->CY_intvl = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX_TEXT(data->CY_intvl), REPORT_INTVL_MONTH);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Sort by _amount"));
	data->CM_byamount = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Compare Exp. & Inc."));
	data->CM_compare = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_Zoom X:"));
	data->LB_zoomx = label;
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = make_scale(label);
	data->RG_zoomx = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);
	
	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);
	
/*
	row++;
	widget = gtk_check_button_new_with_mnemonic ("Legend");
	data->CM_legend = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 1, 2, row, row+1);
*/

	//quick date
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

	//widget = gtk_combo_box_text_new();
	//gtk_widget_set_hexpand(widget, FALSE);
	//gtk_combo_box_text_append_text (widget, "--------");
	//gtk_combo_box_text_append_text (widget, "EDF invoices from 2012 to 2016");
	//gtk_combo_box_text_append_text (widget, "Lorem ipsum dolor sit amet, consectetur adipiscing elit");
	//gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	/*row++;
	label = gtk_label_new(_("Lorem ipsum dolor sit amet, consectetur adipiscing elit"));
	gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END);
	//gtk_widget_set_hexpand(label, FALSE);
	gtk_widget_set_size_request(label, HB_MINWIDTH_LIST, -1);
	gtk_grid_attach (GTK_GRID (table), label, 2, row, 1, 1);
	*/
	

	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start (GTK_BOX (hbox), vbox, TRUE, TRUE, 0);

	//toolbar
	widget = repstats_toolbar_create(data);
	data->TB_bar = widget;
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

	
	
	//test date on top right
	/*
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER(hbox), SPACING_SMALL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
	
	data->PO_mindate = gtk_date_entry_new(NULL);
	gtk_widget_set_hexpand(data->PO_mindate, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), data->PO_mindate, FALSE, FALSE, 0);

	data->PO_maxdate = gtk_date_entry_new(NULL);
	gtk_widget_set_hexpand(data->PO_maxdate, FALSE);
	gtk_box_pack_start (GTK_BOX (hbox), data->PO_maxdate, FALSE, FALSE, 0);
	
	data->CY_range = make_daterange(label, DATE_RANGE_CUSTOM_DISABLE);
	gtk_box_pack_start (GTK_BOX (hbox), data->CY_range, FALSE, FALSE, 0);
	*/
	//end test date
	

	//infos + total
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_container_set_border_width (GTK_CONTAINER(hbox), SPACING_SMALL);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);

	widget = make_label(NULL, 0.5, 0.5);
	gimp_label_set_attributes (GTK_LABEL (widget), PANGO_ATTR_SCALE,  PANGO_SCALE_SMALL, -1);
	data->TX_daterange = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	entry = gtk_label_new(NULL);
	data->TX_total[2] = entry;
	gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	label = gtk_label_new(_("Total:"));
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	entry = gtk_label_new(NULL);
	data->TX_total[1] = entry;
	gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	label = gtk_label_new(_("Income:"));
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);


	entry = gtk_label_new(NULL);
	data->TX_total[0] = entry;
	gtk_box_pack_end (GTK_BOX (hbox), entry, FALSE, FALSE, 0);
	label = gtk_label_new(_("Expense:"));
	gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);


	/* report area */
	notebook = gtk_notebook_new();
	data->GR_result = notebook;
	gtk_widget_show(notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), FALSE);
	gtk_notebook_set_show_border(GTK_NOTEBOOK(notebook), FALSE);
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);

	// page list/detail
	vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vpaned, NULL);

	// list group
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_paned_pack1 (GTK_PANED(vpaned), vbox, TRUE, TRUE);
	// list total
	sw = gtk_scrolled_window_new (NULL, NULL);
	data->SW_total = sw;
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX(vbox), sw, TRUE, TRUE, 0);
	treeview = lst_rep_total_create();
	data->LV_report = treeview;
	gtk_container_add (GTK_CONTAINER(sw), treeview);

	// list trend
	sw = gtk_scrolled_window_new (NULL, NULL);
	data->SW_trend = sw;	
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX(vbox), sw, TRUE, TRUE, 0);
	treeview = lst_rep_time_create();
	data->LV_report2 = treeview;
	gtk_container_add (GTK_CONTAINER(sw), treeview);

	//detail
	sw = gtk_scrolled_window_new (NULL, NULL);
	data->GR_detail = sw;
	//gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW (sw), GTK_CORNER_TOP_RIGHT);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = create_list_transaction(LIST_TXN_TYPE_DETAIL, PREFS->lst_det_columns);
	data->LV_detail = treeview;
	gtk_container_add (GTK_CONTAINER(sw), treeview);
	gtk_paned_pack2 (GTK_PANED(vpaned), sw, TRUE, TRUE);	

	list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);

	//page: 2d bar /pie
	widget = gtk_chart_new(CHART_TYPE_COL);
	data->RE_chart = widget;
	gtk_chart_set_minor_prefs(GTK_CHART(widget), PREFS->euro_value, PREFS->minor_cur.symbol);
	gtk_chart_set_currency(GTK_CHART(widget), GLOBALS->kcur);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);

	//page: 2d stack/stack100
	widget = gtk_chart_new(CHART_TYPE_NONE);
	data->RE_chart2 = widget;
	gtk_chart_set_minor_prefs(GTK_CHART(widget), PREFS->euro_value, PREFS->minor_cur.symbol);
	gtk_chart_set_currency(GTK_CHART(widget), GLOBALS->kcur);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);


	// connect dialog signals
	g_signal_connect (window, "delete-event", G_CALLBACK (repstats_window_dispose), (gpointer)data);
	g_signal_connect (window, "map-event"   , G_CALLBACK (repstats_window_mapped), NULL);

	// setup, init and show window
	wg = &PREFS->sta_wg;
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
	gtk_widget_hide(data->SW_trend);

	repstats_sensitive(window, NULL);
	repstats_update_detail(window, NULL);

	return window;
}

