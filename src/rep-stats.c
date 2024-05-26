/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2024 Maxime DOYEN
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
#include "hbtk-switcher.h"

#include "dsp-mainwindow.h"
#include "ui-filter.h"
#include "ui-transaction.h"

#include "ui-flt-widget.h"


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
static gchar *repstats_compute_title(gint mode, gint src, gint type, gint intvl);


extern HbKvData CYA_REPORT_SRC[];
extern HbKvData CYA_REPORT_TYPE[];

extern gchar *CYA_REPORT_MODE[];
extern HbKvData CYA_REPORT_INTVL[];


/* action functions -------------------- */




static void repstats_action_viewlist(GtkRadioButton *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;
gint tmpmode;

	tmpmode  = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));

	hb_widget_visible (data->SW_total, tmpmode ? FALSE : TRUE);
	hb_widget_visible (data->SW_trend, tmpmode ? TRUE : FALSE);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 0);
	data->charttype = 0;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_mode_changed(GtkRadioButton *radiobutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->BT_list), TRUE);
	repstats_action_viewlist(radiobutton, data);

	if( data->detail == TRUE )
	{
	gint tmpmode  = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));

		if( tmpmode == 0 )
			repstats_selection(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), NULL);
		else
			repstats_selection2(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report2)), NULL);

	}
}


static void repstats_action_viewbar(GtkWidget *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	gtk_chart_set_type (GTK_CHART(data->RE_chart), CHART_TYPE_COL);
	data->charttype = CHART_TYPE_COL;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_viewpie(GtkWidget *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	gtk_chart_set_type (GTK_CHART(data->RE_chart), CHART_TYPE_PIE);
	data->charttype = CHART_TYPE_PIE;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_viewstack(GtkWidget *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 2);
	gtk_chart_set_type (GTK_CHART(data->RE_chart2), CHART_TYPE_STACK);
	data->charttype = CHART_TYPE_STACK;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_viewstack100(GtkWidget *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 2);
	gtk_chart_set_type (GTK_CHART(data->RE_chart2), CHART_TYPE_STACK100);
	data->charttype = CHART_TYPE_STACK100;
	repstats_sensitive(data->window, NULL);
}


static void repstats_action_print(GtkWidget *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;
gint tmpsrc, tmpmode, tmptype, tmpintvl, page;
gchar *title, *name;

	tmpmode  = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));
	tmpsrc   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	tmptype  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));
	tmpintvl = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
	page = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	name = g_strdup_printf("hb-repstat_%s", hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

	if( page == 0 )
	{
	GString *node;

		title = repstats_compute_title(tmpmode, tmpsrc, tmptype, tmpintvl);
		if( tmpmode == 0 )
			node = lst_report_to_string(GTK_TREE_VIEW(data->LV_report), tmpsrc, hbtk_get_label(CYA_REPORT_SRC, tmpsrc), TRUE);
		else
			node = lst_rep_time_to_string(GTK_TREE_VIEW(data->LV_report2), tmpsrc, NULL, TRUE);

		hb_print_listview(GTK_WINDOW(data->window), node->str, NULL, title, name, FALSE);

		g_string_free(node, TRUE);
		g_free(title);
	}
	else
	{
		if( tmpmode == 0 )
			gtk_chart_print(GTK_CHART(data->RE_chart), GTK_WINDOW(data->window), PREFS->path_export, name);
		else
			gtk_chart_print(GTK_CHART(data->RE_chart2), GTK_WINDOW(data->window), PREFS->path_export, name);

	}

	g_free(name);

}


static void repstats_action_filter(GtkWidget *toolbutton, gpointer user_data)
{
struct repstats_data *data = user_data;
gint response_id;

	DB( g_print("\n[repdist] filter\n") );

	//debug
	//create_deffilter_window(data->filter, TRUE);
	response_id = ui_flt_manage_dialog_new(GTK_WINDOW(data->window), data->filter, TRUE, FALSE);
	if( response_id != GTK_RESPONSE_REJECT)
	{
		switch( response_id )
		{
			case HB_RESPONSE_FLT_RESET:
				DB( g_print(" reset filter\n") );
				repstats_filter_setup(data);
				break;
			case HB_RESPONSE_FLT_SAVE_USE:
				ui_flt_popover_hub_save(data->PO_hubfilter, NULL);
				break;
			case GTK_RESPONSE_ACCEPT:
				if( data->filter->range == FLT_RANGE_MISC_CUSTOM )
				{
					filter_preset_daterange_set(data->filter, data->filter->range, 0);
				}
				break;
		}

		//beta 5.8
		ui_flt_manage_header_sensitive(data->PO_hubfilter, NULL);

		g_signal_handler_block(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), data->filter->range);
		g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);

		repstats_update_date_widget(data->window, NULL);
		repstats_update_daterange(data->window, NULL);
		repstats_compute(data->window, NULL);
	}
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void beta_repstats_filter_cb_reset(GtkWidget *widget, gpointer user_data);


static void beta_repstats_filter_cb_preset_change(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
Filter *newflt;

	DB( g_print("\n[repdist] filter change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	newflt = ui_flt_popover_hub_get(GTK_BOX(data->PO_hubfilter), NULL);
	if( newflt )
	{
		DB( g_print(" key:%d, copy filter\n", newflt->key) );
		da_flt_copy(newflt, data->filter);

		hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), data->filter->range);
	}
	else
		beta_repstats_filter_cb_reset(widget, user_data);

	ui_flt_manage_header_sensitive(data->PO_hubfilter, NULL);
}


static void beta_repstats_filter_cb_reset(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
GtkWidget * combobox;

	DB( g_print("\n[repdist] filter reset\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	combobox = ui_flt_popover_hub_get_combobox(GTK_BOX(data->PO_hubfilter), NULL);
	g_signal_handlers_block_by_func (G_OBJECT (combobox), G_CALLBACK (beta_repstats_filter_cb_preset_change), NULL);
	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), 0);
	g_signal_handlers_unblock_by_func (G_OBJECT (combobox), G_CALLBACK (beta_repstats_filter_cb_preset_change), NULL);

	repstats_filter_setup(data);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), data->filter->range);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);

	repstats_update_date_widget(data->window, NULL);
	repstats_update_daterange(data->window, NULL);
	repstats_compute(data->window, NULL);
	ui_flt_manage_header_sensitive(data->PO_hubfilter, NULL);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static gchar *
repstats_compute_title(gint mode, gint src, gint type, gint intvl)
{
gchar *x, *y;

	//5.7.3 fix title
	if( mode == REPORT_MODE_TOTAL )
	{
		x = hbtk_get_label(CYA_REPORT_TYPE,type);
		y = hbtk_get_label(CYA_REPORT_SRC, src);
	}
	else
	{
		x = hbtk_get_label(CYA_REPORT_SRC, src);
		y = hbtk_get_label(CYA_REPORT_INTVL, intvl);
	}

	//TRANSLATORS: example 'Expense by Category'
	//TRANSLATORS: example 'Category by Month'
	return g_strdup_printf(_("%s by %s"), x, y );
}


static void repstats_date_change(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	DB( g_print("\n[repdist] date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	//5.8 check for error
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_mindate), ( data->filter->mindate > data->filter->maxdate ) ? TRUE : FALSE);
		gtk_date_entry_set_error(GTK_DATE_ENTRY(data->PO_maxdate), ( data->filter->maxdate < data->filter->mindate ) ? TRUE : FALSE);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPDIST_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
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

	repstats_update_date_widget(data->window, NULL);

	//5.7.1 test add disable forecast
	/*g_signal_handler_block(data->CM_forecast, data->handler_id[HID_REPDIST_FORECAST]);
	gint can_future = filter_preset_daterange_future_enable(data->filter, range);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_forecast), can_future ? PREFS->rep_forcast : FALSE);

	gtk_widget_set_sensitive(data->CM_forecast, can_future);
	g_signal_handler_unblock(data->CM_forecast, data->handler_id[HID_REPDIST_FORECAST]);
	*/

	repstats_compute(data->window, NULL);
	repstats_update_daterange(data->window, NULL);
}


static void repstats_update(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
gboolean byamount;
GtkTreeModel		 *model;
gint tmpmode, tmpsrc, tmptype, tmpintvl, usrcomp, column;
gboolean xval;
gchar *title;

	DB( g_print("\n[repdist] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	byamount = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_byamount));
	usrcomp  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_compare));
	tmpmode  = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));
	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	tmptype  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));
	tmpintvl = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));

	//debug option
	DB( g_print(" option: byamount=%d tmptype=%d '%s' tmpsrc=%d '%s'\n\n", byamount, tmptype, hbtk_get_label(CYA_REPORT_TYPE,tmptype), tmpsrc, hbtk_get_label(CYA_REPORT_SRC,tmpsrc)) );

	// define view/sort column
	column = LST_REPORT_POS;

	if( byamount )
	{
		switch( tmptype )
		{
			case REPORT_TYPE_TOTAL:
				column = LST_REPORT_TOTAL;
				break;
			case REPORT_TYPE_EXPENSE:
				column = LST_REPORT_EXPENSE;
				break;
			case REPORT_TYPE_INCOME:
				column = LST_REPORT_INCOME;
				break;
		}
	}

	DB( g_print(" sort on column %d\n\n", column) );
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(model), column, GTK_SORT_ASCENDING);

	title = repstats_compute_title(tmpmode, tmpsrc, tmptype, tmpintvl);

	#if HB_STATS_DO_TOTAL == 1
	gtk_chart_set_color_scheme(GTK_CHART(data->RE_chart), PREFS->report_color_scheme);

	/* update absolute or not */
	gboolean abs = (tmptype == REPORT_TYPE_EXPENSE || tmptype == REPORT_TYPE_INCOME) ? TRUE : FALSE;
	gtk_chart_set_absolute(GTK_CHART(data->RE_chart), abs);

	/* show xval for month/year and no by amount display */
	xval = FALSE;

	if( !byamount && (tmpsrc == REPORT_GRPBY_MONTH || tmpsrc == REPORT_GRPBY_YEAR) )
	{
		xval = TRUE;
		/*switch( tmpsrc)
		{
			case REPORT_GRPBY_MONTH:
				gtk_chart_set_every_xval(GTK_CHART(data->RE_chart), 4);
				break;
			case REPORT_GRPBY_YEAR:
				gtk_chart_set_every_xval(GTK_CHART(data->RE_chart), 2);
				break;
		}*/
	}

	gtk_chart_show_xval(GTK_CHART(data->RE_chart), xval);

	/* update bar chart */
	if( (tmptype == REPORT_TYPE_TOTAL) && (usrcomp == TRUE) ) //dual exp/inc
	{
		DB( g_print(" set bar to dual exp %d/inc %d\n\n", LST_REPORT_EXPENSE, LST_REPORT_INCOME) );
		//set column1 != column2 will dual display
		gtk_chart_set_datas_total(GTK_CHART(data->RE_chart), model, LST_REPORT_EXPENSE, LST_REPORT_INCOME, title, NULL);
	}
	else
	{
		switch(tmptype)
		{
			case REPORT_TYPE_EXPENSE:
				column = LST_REPORT_EXPENSE;
				break;
			case REPORT_TYPE_INCOME:
				column = LST_REPORT_INCOME;
				break;
			default:
			case REPORT_TYPE_TOTAL:
				column = LST_REPORT_TOTAL;
				break;
		}

		DB( g_print(" set data total to col=%d\n\n", column) );
		//set column1 != column2 will dual display
		gtk_chart_set_datas_total(GTK_CHART(data->RE_chart), model, column, column, title, NULL);
	}


	#endif

	//time chart
	#if HB_STATS_DO_TIME == 1
	gtk_chart_set_color_scheme(GTK_CHART(data->RE_chart2), PREFS->report_color_scheme);
	gtk_chart_show_xval(GTK_CHART(data->RE_chart2), TRUE);
	//5.7 trendrow is unused, we pass the treeview to get the column labels
	DB( g_print(" set data time\n") );
	gtk_chart_set_datas_time(GTK_CHART(data->RE_chart2), GTK_TREE_VIEW(data->LV_report2), data->trend, data->trendrows, data->trendcols, title, NULL);
	#endif

	//test 5.8
	da_flt_count_item(data->filter);
	gchar *txt = filter_text_summary_get(data->filter);
	ui_label_set_integer(GTK_LABEL(data->TX_fltactive), data->filter->n_active);
	gtk_widget_set_tooltip_text(data->TT_fltactive, txt);
	g_free(txt);

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


static void repstats_export_result_clipboard(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct repstats_data *data;
GtkClipboard *clipboard;
GString *node;
gchar *title, *catsubcat;
gint tmpmode, tmpsrc;

	DB( g_print("\n[repdist] export result clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpmode = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));

	//#1886299/1900281
	catsubcat = NULL;
	title = hbtk_get_label(CYA_REPORT_SRC,tmpsrc);
	/*if(tmpsrc == REPORT_GRPBY_SUBCATEGORY)
	{
		catsubcat = g_strjoin(":",hbtk_get_label(CYA_REPORT_SRC, REPORT_GRPBY_CATEGORY), hbtk_get_label(CYA_REPORT_SRC, REPORT_GRPBY_SUBCATEGORY), NULL);
		title = catsubcat;
	}*/

	if( tmpmode == 0 )
		node = lst_report_to_string(GTK_TREE_VIEW(data->LV_report), tmpsrc, title, TRUE);
	else
		node = lst_rep_time_to_string(GTK_TREE_VIEW(data->LV_report2), tmpsrc, title, TRUE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);

	g_free(catsubcat);
}


static void repstats_export_result_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
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

	tmpmode = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));
	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));

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
			/*if(tmpsrc == REPORT_GRPBY_SUBCATEGORY)
			{
				catsubcat = g_strjoin(":",hbtk_get_label(CYA_REPORT_SRC, REPORT_GRPBY_CATEGORY), hbtk_get_label(CYA_REPORT_SRC, REPORT_GRPBY_SUBCATEGORY), NULL);
				title = catsubcat;
			}*/

			if( tmpmode == 0 )
				node = lst_report_to_string(GTK_TREE_VIEW(data->LV_report), tmpsrc, title, FALSE);
			else
				node = lst_rep_time_to_string(GTK_TREE_VIEW(data->LV_report2), tmpsrc, title, FALSE);

			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
			g_free(catsubcat);
		}
		g_free( filename );
	}
	g_free(name);
}


static void repstats_export_detail_clipboard(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct repstats_data *data;
GtkClipboard *clipboard;
GString *node;
guint flags;

	DB( g_print("\n[repdist] export detail clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	flags = LST_TXN_EXP_CLR | LST_TXN_EXP_PMT | LST_TXN_EXP_CAT | LST_TXN_EXP_TAG;
	node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), TRUE, FALSE, FALSE, flags);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void repstats_export_detail_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
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
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
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


static void repstats_detail(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
guint active = GPOINTER_TO_INT(user_data);
guint tmpsrc;
GList *list;
GtkTreeModel *model;
GtkTreeIter  iter, child;
Category *active_cat;
gboolean is_subcat = FALSE;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repdist] detail populate\n") );

	/* clear and detach our model */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

	if( data->detail == 0 || data->txn_queue == NULL )
	{
		DB( g_print(" > skipped: detail hidden or no txn\n") );
		return;
	}

	//get cat/subcat
	active_cat = da_cat_get(active);
	if( active_cat )
		is_subcat = (active_cat->parent == 0) ? FALSE : TRUE;

	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));

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
		guint i, key = 0;

			switch( tmpsrc )
			{
				case REPORT_GRPBY_CATEGORY:
					if( ope->flags & OF_SPLIT )
					{
					guint nbsplit = da_splits_length(ope->splits);
					Split *split;

						dtlamt = 0.0;
						for(i=0;i<nbsplit;i++)
						{
							split = da_splits_get(ope->splits, i);
							//key = category_report_id(split->kcat, (tmpsrc == REPORT_GRPBY_SUBCATEGORY) ? TRUE : FALSE);
							key = category_report_id(split->kcat, is_subcat);
							if( key == active )
							{
								match = TRUE;
								dtlamt += split->amount;
								// no more break here as we need to compute split total
								//break;
							}
						}
					}
					else
					{
						key = category_report_id(ope->kcat, is_subcat);
						if( key == active )
						{
							match = TRUE;
						}
					}
					break;
				/* the TAG process is particular */
				case REPORT_GRPBY_TAG:
					if(ope->tags != NULL)
					{
					guint32 *tptr = ope->tags;

						while(*tptr)
						{
							key = *tptr;
							if( key == active )
							{
								match = TRUE;
								break;
							}
							tptr++;
						}
					}
					else
						match = ( key == active ) ? TRUE : FALSE;
					break;
				default:
					key = report_items_get_key(tmpsrc, data->filter->mindate, ope);
					if( key == active )
					{
						match = TRUE;
					}
					break;
			}

			if( match == TRUE )
			{
				DB( g_print(" txn match to key=%d\n", active) );

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
						//if( (tmpsrc == REPORT_GRPBY_CATEGORY || tmpsrc == REPORT_GRPBY_SUBCATEGORY) )
						if( tmpsrc == REPORT_GRPBY_CATEGORY )
						{
							//key = category_report_id(split->kcat, (tmpsrc == REPORT_GRPBY_SUBCATEGORY) ? TRUE : FALSE);
							key = category_report_id(split->kcat, FALSE);
							DB( g_print("  %d =? %d => %d\n", key, active, key != active ? FALSE : TRUE) );
							if( key != active )
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


static void repstats_compute(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;
DataTable *dt;
gint tmpsrc, tmptype, tmpintvl;
gdouble totexp, totinc;
gboolean tmpaccbal, tmpforecast;
guint i, n_inserted;
GtkTreeModel *model;
GtkTreeIter  iter, parent, *tmpparent;

	DB( g_print("\n----------------\n[repdist] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//clear all
	//free previous txn
	if(data->txn_queue != NULL)
		g_queue_free (data->txn_queue);
	data->txn_queue = NULL;
	#if HB_STATS_DO_TOTAL == 1
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	gtk_tree_store_clear (GTK_TREE_STORE(model));
	gtk_chart_set_datas_none(GTK_CHART(data->RE_chart));
	#endif

	//refresh the datatable
	if(data->trend)
		da_datatable_free (data->trend);
	data->trend = NULL;
	#if HB_STATS_DO_TIME == 1
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report2));
	gtk_tree_store_clear (GTK_TREE_STORE(model));
	gtk_chart_set_datas_none(GTK_CHART(data->RE_chart2));
	#endif

	data->total_expense = 0.0;
	data->total_income = 0.0;
	repstats_update_total(widget, user_data);

	//#2019876 return is invalid date range
	if( data->filter->maxdate < data->filter->mindate )
		return;

	//tmpsrc  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_src));
	tmpsrc    = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
	tmptype   = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));
	tmpintvl  = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_intvl));
	tmpaccbal = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_balance));
	tmpforecast = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_forecast));
	if( tmpsrc != REPORT_GRPBY_ACCOUNT && tmpsrc != REPORT_GRPBY_ACCGROUP)
		tmpaccbal = FALSE;

	DB( hb_print_date(data->filter->mindate, "min:") );
	DB( hb_print_date(data->filter->maxdate, "max:") );

	//#2030334 get the forecat max date
	guint32 jmaxdateforecast = data->filter->maxdate;

	//#2036228 don't force for custom range
	gint range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));
	if( range != FLT_RANGE_MISC_CUSTOM )
	{
		if( tmpforecast == TRUE )
			jmaxdateforecast = filter_get_maxdate_forecast(data->filter);

		DB( hb_print_date(jmaxdateforecast, "maxforecast:") );
	}

	//TODO: not necessary until date range change
	//data->txn_queue = hbfile_transaction_get_partial(data->filter->mindate, data->filter->maxdate);
	data->txn_queue = hbfile_transaction_get_partial(data->filter->mindate, jmaxdateforecast);

	DB( g_print(" for=%d,kind=%d\n", tmpsrc, tmptype) );
	DB( g_print(" nb-txn=%d\n", g_queue_get_length (data->txn_queue) ) );

	//set filter
	/*if(!tmpaccbal)
		filter_preset_type_set(data->filter, FLT_TYPE_INTXFER, (PREFS->stat_includexfer == FALSE) ? FLT_EXCLUDE : FLT_INCLUDE);
	else
		filter_preset_type_set(data->filter, FLT_TYPE_ALL, FLT_OFF);*/


	#if HB_STATS_DO_TOTAL == 1

	DB( g_print(" -- start total --\n") );

//TODO: add if mode==0
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
	n_inserted = 0;

	totexp = totinc = 0.0;

	dt = report_compute(tmpsrc, tmpintvl, data->filter, data->txn_queue, (tmpaccbal == TRUE) ? REPORT_COMP_FLG_BALANCE : REPORT_COMP_FLG_NONE);
	if(dt)
	{
	struct lst_report_data *lst_data;
	DataRow *dr;

		//store the totals from total datarow
		dr = dt->totrow;

		lst_data = g_object_get_data(G_OBJECT(data->LV_report), "inst_data");
		if( lst_data != NULL )
		{
			lst_data->tot_exp = dr->rowexp;
			lst_data->tot_inc = dr->rowinc;
		}
		data->total_expense = dr->rowexp;
		data->total_income  = dr->rowinc;
		//total_total = dt->totexp + dt->totinc;
		//#2018145 rate total must remains ABS to get accurate %
		//ratetotal   = ABS(dr->rowexp) + ABS(dr->rowinc);

		DB( g_printf(" total exp %9.2f\n", dr->rowexp) );
		DB( g_printf(" total inc %9.2f\n", dr->rowinc) );
		//DB( g_printf(" total tot %9.2f\n", total_total) );

		// clear and detach our model
		g_object_ref(model); // Make sure the model stays with us after the tree view unrefs it
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), NULL); // Detach model from view

		DB( g_print("\n -- populate total listview : %d rows --\n", dt->nbrows) );

		// insert into the treeview
		for(i=0; i < dt->nbrows; i++)
		{
		//gdouble drtotal;
		gboolean insert;
		guint32 reskey;
		gchar *overlabel = NULL;

			//since 5.7 we use the dt-keylst here to insert cat before subcat
			reskey = dt->keylist[i];
			DB( g_print(" get row for key=%d\n", reskey) );
			dr = report_data_get_row(dt, reskey);

			// skip empty results
			insert = TRUE;
			if( !(tmpsrc == REPORT_GRPBY_MONTH || tmpsrc == REPORT_GRPBY_YEAR) )
			{
				if( (tmptype == REPORT_TYPE_EXPENSE && !dr->rowexp) ||
				    (tmptype == REPORT_TYPE_INCOME  && !dr->rowinc) ||
				    (!dr->rowexp && !dr->rowinc)
				  )
				  insert = FALSE;
			}

			//#2065531 skip tag filtered
			if( tmpsrc == REPORT_GRPBY_TAG && data->filter->option[FLT_GRP_TAG] != 0)
			{
				if( !da_flt_status_tag_get(data->filter, reskey) )
					insert = FALSE;
			}

			// skip no account
			//if( (tmpsrc == REPORT_GRPBY_ACCOUNT) && (i == 0) )
			//	insert = FALSE;

			if( insert == FALSE )
			{
				DB( g_printf("   skip: %2d, '%s', %9.2f  %9.2f  %9.2f\n", i, dr->label,
					dr->rowexp, dr->rowinc, dr->rowexp - dr->rowinc) );
				continue;
			}


			n_inserted++;

			// manage the toplevel for category
			tmpparent = NULL;
			if( tmpsrc == REPORT_GRPBY_CATEGORY )
			{
			Category *tmpcat = da_cat_get(reskey);

				if( tmpcat != NULL )
				{
					overlabel = (tmpcat->key == 0) ? NULL : tmpcat->typename;

					if( tmpcat->parent != 0 )
					{
						//if( lst_report_get_top_level (GTK_TREE_MODEL(model), tmpcat->parent, &parent) == TRUE )
						if( hbtk_tree_store_get_top_level(GTK_TREE_MODEL(model), LST_REPORT_KEY, tmpcat->parent, &parent) )
						{
							tmpparent = &parent;
						}
						else
						{
							DB( g_print(" !! no parent %d found for %d %s\n", tmpcat->parent, tmpcat->key, tmpcat->fullname) );
						}
					}
					else
					//if( tmpcat->parent == 0 )
					//only sum for category, not subcat
					{
						totexp += dr->rowexp;
						totinc += dr->rowinc;
					}
				}
			}
			//if not category, sum
			else
			{
				totexp += dr->rowexp;
				totinc += dr->rowinc;
			}

	    	gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &iter, tmpparent, -1,
		        LST_REPORT_POS , dr->pos,
		        LST_REPORT_KEY , reskey,
				LST_REPORT_LABEL, dr->label,
				LST_REPORT_OVERLABEL, overlabel,
				//LST_REPORT_ROW,  dr,
				LST_REPORT_EXPENSE, dr->rowexp,
				LST_REPORT_INCOME , dr->rowinc,
				LST_REPORT_TOTAL  , dr->rowexp + dr->rowinc,
				-1);

			DB( g_printf(" insert: %2d, '%s', %9.2f  %9.2f  %9.2f\n", i, dr->label,
				dr->rowexp, dr->rowinc, dr->rowexp - dr->rowinc) );
		}

		// update column 0 title
		GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report), 0);
		gtk_tree_view_column_set_title(column, hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

		// Re-attach model to view
  		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
		g_object_unref(model);

		gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_report));
		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));
	}

	da_datatable_free (dt);

	//insert total
	if( n_inserted > 1 )
	{
		DB( g_print(" insert total\n") );
		gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &iter, NULL, -1,
		    LST_REPORT_POS, LST_REPORT_POS_TOTAL,
		    LST_REPORT_KEY, -1,
			LST_REPORT_LABEL, _("Total"),
			LST_REPORT_OVERLABEL, NULL,
			LST_REPORT_EXPENSE, totexp,
			LST_REPORT_INCOME , totinc,
			LST_REPORT_TOTAL, totexp + totinc,
			-1);
	}
	#endif

	/* ---- trend with test multi to remove ---- */

	//TODO: add if mode==1 && choose a better max column value
	#if HB_STATS_DO_TIME == 1

	DB( g_print(" -- start time --\n") );
	n_inserted = 0;

	//check limit
	i = report_interval_count(tmpintvl, data->filter->mindate, data->filter->maxdate);

	if( i > LST_REP_COLID_MAX )
	{
		//lst_rep_time_renewcol(GTK_TREE_VIEW(data->LV_report2), 0, data->filter->mindate, tmpintvl);

		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
		_("Too much columns to display"),
		_("Please select a wider interval and / or a narrower date range")
		);
	}
	else
	{

		//#2030334 to avoid forecast column to remains
		//we force the date here only when forecast is ON
		guint32 saveddate = data->filter->maxdate;
		data->filter->maxdate = jmaxdateforecast;
		DB( hb_print_date(saveddate, "saveddate:") );
		DB( hb_print_date(data->filter->maxdate, "maxdate:") );

		guint flags = REPORT_COMP_FLG_NONE;
		if( tmpforecast )
			flags |= REPORT_COMP_FLG_FORECAST;
		if( tmpaccbal )
			flags |= REPORT_COMP_FLG_BALANCE;

		dt = report_compute(tmpsrc, tmpintvl, data->filter, data->txn_queue, flags);
		data->trend = dt;
		data->filter->maxdate = saveddate;
		DB( hb_print_date(data->filter->maxdate, "maxdate after compute:") );
		//end of forceddate

		DB( g_print("\n -- populate time listview : %d rows, %d cols --\n", dt->nbrows, dt->nbcols) );

		// clear and detach our model
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report2));
		g_object_ref(model); // Make sure the model stays with us after the tree view unrefs it
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report2), NULL); // Detach model from view

		lst_rep_time_renewcol(GTK_TREE_VIEW(data->LV_report2), model, dt, tmpaccbal ? FALSE : TRUE);


		// update column 0 title
		GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report2), 0);
		gtk_tree_view_column_set_title(column, hbtk_get_label(CYA_REPORT_SRC,tmpsrc));

		//#1955046 treeview with child was a test faulty released
		for(i=0; i < dt->nbrows; i++)
		{
		DataRow *dr;
		guint32 reskey;
		gchar *overlabel = NULL;

			reskey = dt->keylist[i];
			dr = report_data_get_row(dt, reskey);


			//#2024940 test on exp/inc individually
			if( hb_amount_equal(dr->rowexp, 0.0) && hb_amount_equal(dr->rowinc, 0.0) )
			{
				DB( g_printf(" %4d:'%s' %.2f :: hide (no data)\n", i, dr->label, dr->rowexp + dr->rowinc ) );
				continue;
			}

			//#2065531 skip tag filtered
			if( tmpsrc == REPORT_GRPBY_TAG && data->filter->option[FLT_GRP_TAG] != 0)
			{
				if( !da_flt_status_tag_get(data->filter, reskey) )
				{
					DB( g_printf(" %4d:'%s' %.2f :: hide (tag filtered)\n", i, dr->label, dr->rowexp + dr->rowinc ) );
					continue;
				}
			}

			DB( g_printf(" %4d:'%s' %.2f :: insert\n", i, dr->label, dr->rowexp + dr->rowinc ) );
			n_inserted++;

			// manage the toplevel for category
			tmpparent = NULL;
			//if( tmpsrc == REPORT_GRPBY_CATEGORY || tmpsrc == REPORT_GRPBY_SUBCATEGORY )
			if( tmpsrc == REPORT_GRPBY_CATEGORY )
			{
			Category *tmpcat = da_cat_get(reskey);
				if( tmpcat != NULL)
				{
					overlabel = (tmpcat->key == 0) ? NULL : tmpcat->typename;
					//if( lst_rep_time_get_top_level (GTK_TREE_MODEL(model), tmpcat->parent, &parent) == TRUE )
					if( hbtk_tree_store_get_top_level(GTK_TREE_MODEL(model), LST_REPORT2_KEY, tmpcat->parent, &parent) )
					{
						tmpparent = &parent;
					}
					else
					{
						DB( g_print(" !! no parent %d found for %d %s\n", tmpcat->parent, tmpcat->key, tmpcat->fullname) );
					}
				}
			}

			DB( g_printf(" --> insert\n") );

			gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, tmpparent, -1,
				LST_REPORT2_POS, n_inserted,
				LST_REPORT2_KEY, reskey,
				LST_REPORT2_LABEL, dr->label,
				LST_REPORT2_OVERLABEL, overlabel,
				LST_REPORT2_ROW, dr,
				-1);
		}

		data->trendrows = n_inserted;
		data->trendcols = dt->nbcols;

		//insert total
		if( n_inserted > 1 )
		{
		DataRow *dr = dt->totrow;

			dr->pos = LST_REPORT_POS_TOTAL;

			DB( g_printf(" eval item total:'%s'\n", dr->label ) );

			DB( g_printf(" --> insert total\n") );
			n_inserted++;
			gtk_tree_store_insert_with_values(GTK_TREE_STORE(model), &iter, NULL, -1,
				LST_REPORT2_POS, LST_REPORT_POS_TOTAL,
				LST_REPORT2_KEY, -1,
				LST_REPORT2_LABEL, _("Total"),
				LST_REPORT2_OVERLABEL, NULL,
				LST_REPORT2_ROW, dr,
				-1);
		}

		// Re-attach model to view
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report2), model);
		g_object_unref(model);

		gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_report2));
		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report2));

		// test liststore
		//liststore_benchmark();

		// test multi end

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

	mode = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));
	type = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));
	tmpsrc = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_src));
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

	visible = mode == 0 ? FALSE : PREFS->rep_forcast;
	hb_widget_visible(data->CM_forecast, visible);

	visible = (mode==0 && page == 0) ? TRUE : FALSE;
	hb_widget_visible(data->BT_rate, visible);

	visible = (tmpsrc == REPORT_GRPBY_ACCOUNT) || (tmpsrc == REPORT_GRPBY_ACCGROUP) ? TRUE : FALSE;
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
	hb_widget_visible (data->BT_detail  , visible);
	hb_widget_visible (data->BT_export  , visible);

	visible = (page == 0 && tmpsrc == REPORT_GRPBY_CATEGORY) ? TRUE : FALSE;
	hb_widget_visible (data->GR_listbar, visible);

	visible = (page > 0) ? TRUE : FALSE;
	hb_widget_visible (data->BT_legend, visible);
	//hb_widget_visible (data->BT_print, visible);

	page = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail)), NULL);
	sensitive = ((page > 0) && data->detail) ? TRUE : FALSE;
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "detclip")), sensitive);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "detcsv")), sensitive);
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
			gint tmpmode  = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));

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
				gtk_tree_model_get(model, &iter, LST_REPORT_KEY, &key, -1);

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
	repstats_sensitive(widget, NULL);
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


static void repstats_cb_expand_all(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[repdist] expand all (data=%p)\n", data) );

	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_report));
	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_report2));

}


static void repstats_cb_collapse_all(GtkWidget *widget, gpointer user_data)
{
struct repstats_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[repdist] collapse all (data=%p)\n", data) );

	gtk_tree_view_collapse_all(GTK_TREE_VIEW(data->LV_report));
	gtk_tree_view_collapse_all(GTK_TREE_VIEW(data->LV_report2));

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
		gtk_tree_model_get(model, &iter, LST_REPORT_KEY, &key, -1);
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
		gtk_tree_model_get(model, &iter, LST_REPORT2_KEY, &key, -1);
	}

	DB( g_print(" - time active is %d\n", key) );

	repstats_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
	repstats_sensitive(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static const GActionEntry win_actions[] = {
	{ "resclip"		, repstats_export_result_clipboard, NULL, NULL, NULL, {0,0,0} },
	{ "rescsv"		, repstats_export_result_csv, NULL, NULL, NULL, {0,0,0} },
	{ "detclip"		, repstats_export_detail_clipboard, NULL, NULL, NULL, {0,0,0} },
	{ "detcsv"		, repstats_export_detail_csv, NULL, NULL, NULL, {0,0,0} },
//	{ "actioname"	, not_implemented, NULL, NULL, NULL, {0,0,0} },
};


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

	//button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_FILTER, _("Filter"), _("Edit filter"));
	//data->BT_filter = button;

	button = hbtk_toolbar_add_toolbutton(GTK_TOOLBAR(toolbar), ICONNAME_HB_REFRESH, _("Refresh"), _("Refresh results"));
	data->BT_refresh = button;

	//export button
	button = gtk_menu_button_new();
	data->BT_export = button;
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(button)), GTK_STYLE_CLASS_FLAT);
	GtkWidget *image = gtk_image_new_from_icon_name (ICONNAME_HB_FILE_EXPORT, GTK_ICON_SIZE_LARGE_TOOLBAR);
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

	GActionGroup *group = (GActionGroup*)g_simple_action_group_new ();
	data->actions = group;
	g_action_map_add_action_entries (G_ACTION_MAP (group), win_actions, G_N_ELEMENTS (win_actions), data);

	gtk_widget_insert_action_group (button, "win", G_ACTION_GROUP(group));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (button), G_MENU_MODEL (menu));

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

	//#1989211 option to include xfer by default
	/* 3.4 : make int transfer out of stats */
	if(PREFS->stat_includexfer == FALSE)
		filter_preset_type_set(data->filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);

	DB( hb_print_date(data->filter->mindate, "min:") );
	DB( hb_print_date(data->filter->maxdate, "max:") );
}


// setup default for our object/memory
static void repstats_window_setup(struct repstats_data *data)
{
	DB( g_print("\n[repdist] setup\n") );

	DB( g_print(" init data\n") );

	repstats_filter_setup(data);

	//DB( g_print(" populate\n") );

	DB( g_print(" set widgets default\n") );
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->BT_detail), PREFS->stat_showdetail);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->BT_legend), TRUE);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(data->BT_rate)  , PREFS->stat_showrate);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor), GLOBALS->minor);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_byamount), PREFS->stat_byamount);
	//5.8 EXP > TOTAL
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_type), REPORT_TYPE_TOTAL);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), PREFS->date_range_rep);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_forecast), PREFS->rep_forcast);

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail))), "minor", (gpointer)data->CM_minor);

	gtk_chart_set_smallfont (GTK_CHART(data->RE_chart), PREFS->rep_smallfont);
	gtk_chart_set_smallfont (GTK_CHART(data->RE_chart2), PREFS->rep_smallfont);

	repstats_toggle_rate(data->window, NULL);

	DB( g_print(" connect widgets signals\n") );

	g_signal_connect( ui_flt_popover_hub_get_combobox(GTK_BOX(data->PO_hubfilter), NULL), "changed", G_CALLBACK (beta_repstats_filter_cb_preset_change), NULL);
	g_signal_connect (data->BT_reset , "clicked", G_CALLBACK (beta_repstats_filter_cb_reset), NULL);

	g_signal_connect (data->CM_minor, "toggled", G_CALLBACK (repstats_toggle_minor), NULL);

	g_signal_connect (G_OBJECT (data->BT_expand), "clicked", G_CALLBACK (repstats_cb_expand_all), NULL);
	g_signal_connect (G_OBJECT (data->BT_collapse), "clicked", G_CALLBACK (repstats_cb_collapse_all), NULL);


    data->handler_id[HID_REPDIST_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (repstats_date_change), (gpointer)data);
    data->handler_id[HID_REPDIST_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (repstats_date_change), (gpointer)data);

	data->handler_id[HID_REPDIST_RANGE] = g_signal_connect (data->CY_range, "changed", G_CALLBACK (repstats_range_change), NULL);

    g_signal_connect (data->CY_src, "changed", G_CALLBACK (repstats_compute), (gpointer)data);

	g_signal_connect (data->RA_mode, "changed", G_CALLBACK (repstats_action_mode_changed), (gpointer)data);

	g_signal_connect (data->CY_intvl, "changed", G_CALLBACK (repstats_compute), (gpointer)data);

    data->handler_id[HID_REPDIST_VIEW] = g_signal_connect (data->CY_type, "changed", G_CALLBACK (repstats_compute), (gpointer)data);

	g_signal_connect (data->RG_zoomx, "value-changed", G_CALLBACK (repstats_zoomx_callback), NULL);


	g_signal_connect (data->CM_balance, "toggled", G_CALLBACK (repstats_compute), NULL);
	data->handler_id[HID_REPDIST_FORECAST] = g_signal_connect (data->CM_forecast, "toggled", G_CALLBACK (repstats_compute), NULL);
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

	repstats_update_date_widget(data->window, NULL);
	repstats_update_daterange(data->window, NULL);

	repstats_compute(data->window, NULL);

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

	if(data->txn_queue)
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
GtkWidget *window, *mainbox, *hbox, *vbox, *tbar, *bbox, *fbox, *notebook, *treeview, *vpaned, *scrollwin;
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
	mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	hb_widget_set_margin(GTK_WIDGET(mainbox), SPACING_SMALL);
	gtk_window_set_child(GTK_WINDOW(window), mainbox);

	//control part
	table = gtk_grid_new ();
	gtk_widget_set_hexpand (GTK_WIDGET(table), FALSE);
    gtk_box_pack_start (GTK_BOX (mainbox), table, FALSE, FALSE, 0);

	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);

	row = 0;
	//label = make_label_group(_("Display"));
	//gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	fbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_grid_attach (GTK_GRID (table), fbox, 0, row, 3, 1);

		label = make_label_group(_("Display"));
		//gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);
		gtk_box_pack_start (GTK_BOX (fbox), label, FALSE, FALSE, 0);

		//test button
		/*bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_style_context_add_class (gtk_widget_get_style_context (bbox), GTK_STYLE_CLASS_LINKED);
		gtk_box_pack_end (GTK_BOX (fbox), bbox, FALSE, FALSE, 0);

		group = widget = make_image_radio_button(ICONNAME_HB_VIEW_LIST, _("View results as list"));
		data->BT_list = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_radio_button(ICONNAME_HB_VIEW_COLUMN, _("View results as column"));
		gtk_radio_button_join_group(GTK_RADIO_BUTTON(widget), GTK_RADIO_BUTTON(group));
		data->BT_column = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_radio_button(ICONNAME_HB_VIEW_DONUT, _("View results as donut"));
		gtk_radio_button_join_group(GTK_RADIO_BUTTON(widget), GTK_RADIO_BUTTON(group));
		data->BT_donut = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_radio_button(ICONNAME_HB_VIEW_STACK, _("View results as stack"));
		gtk_radio_button_join_group(GTK_RADIO_BUTTON(widget), GTK_RADIO_BUTTON(group));
		data->BT_stack = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_radio_button(ICONNAME_HB_VIEW_STACK100, _("View results as stack 100%"));
		gtk_radio_button_join_group(GTK_RADIO_BUTTON(widget), GTK_RADIO_BUTTON(group));
		data->BT_stack100 = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_toggle_button(ICONNAME_HB_OPE_SHOW, _("Toggle detail"));
		data->BT_detail = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_toggle_button(ICONNAME_HB_SHOW_LEGEND, _("Toggle legend"));
		data->BT_legend = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_toggle_button(ICONNAME_HB_SHOW_RATE, _("Toggle rate"));
		data->BT_rate = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_HB_REFRESH, _("Refresh results"));
		data->BT_refresh = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);
		*/


	row++;
	label = make_label_widget(_("Mode:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_REPORT_MODE, TRUE);
	data->RA_mode = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_View by:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_SRC);
	data->CY_src = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("_Balance mode"));
	data->CM_balance = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_Type:"));
	data->LB_type = label;
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_TYPE);
	data->CY_type = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("Inter_val:"));
	data->LB_intvl = label;
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_REPORT_INTVL);
	data->CY_intvl = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_intvl), REPORT_INTVL_MONTH);

	//5.7
	row++;
	widget = gtk_check_button_new_with_mnemonic (_("_Forecast"));
	data->CM_forecast = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

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

	//-- filter
	row++;
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);
	//gtk_widget_set_margin_bottom(widget, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	//5.8 test
	row++;
	fbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_grid_attach (GTK_GRID (table), fbox, 0, row, 3, 1);

		label = make_label_group(_("Filter"));
		//gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);
		gtk_box_pack_start (GTK_BOX (fbox), label, FALSE, FALSE, 0);

		// active
		label = make_label_widget(_("Active:"));
		gtk_widget_set_margin_start(label, SPACING_MEDIUM);
		gtk_box_pack_start (GTK_BOX (fbox), label, FALSE, FALSE, 0);
		label = make_label(NULL, 0.0, 0.5);
		data->TX_fltactive = label;
		gtk_box_pack_start (GTK_BOX (fbox), label, FALSE, FALSE, 0);
		widget = gtk_image_new_from_icon_name (ICONNAME_INFO, GTK_ICON_SIZE_BUTTON);
		data->TT_fltactive = fbox;
		gtk_box_pack_start (GTK_BOX (fbox), widget, FALSE, FALSE, 0);

		//test button
		bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_style_context_add_class (gtk_widget_get_style_context (bbox), GTK_STYLE_CLASS_LINKED);
		gtk_box_pack_end (GTK_BOX (fbox), bbox, FALSE, FALSE, 0);
		widget = make_image_button(ICONNAME_HB_FILTER, _("Edit filter"));
		data->BT_filter = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);
		widget = make_image_button(ICONNAME_HB_CLEAR, _("Clear filter"));
		data->BT_reset = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		row++;
		widget = create_popover_widget(GTK_WINDOW(data->window), data->filter);
		data->PO_hubfilter = widget;
		gtk_grid_attach (GTK_GRID (table), widget, 1, row, 2, 1);



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


	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start (vbox, SPACING_SMALL);
    gtk_box_pack_start (GTK_BOX (mainbox), vbox, TRUE, TRUE, 0);

	//toolbar
	widget = repstats_toolbar_create(data);
	data->TB_bar = widget;
	gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);


	//infos + total
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	hb_widget_set_margin(GTK_WIDGET(hbox), SPACING_SMALL);
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
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	data->SW_total = scrollwin;
	gtk_box_pack_start (GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);
	treeview = lst_report_create();
	data->LV_report = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

	// list trend
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	data->SW_trend = scrollwin;
	gtk_box_pack_start (GTK_BOX(vbox), scrollwin, TRUE, TRUE, 0);
	treeview = lst_rep_time_create();
	data->LV_report2 = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

	//5.8 moved collapse/expand here
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	data->GR_listbar = tbar;
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (vbox), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_end (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_HB_BUTTON_EXPAND, _("Expand all"));
		data->BT_expand = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_HB_BUTTON_COLLAPSE, _("Collapse all"));
		data->BT_collapse = widget;
		gtk_box_pack_start (GTK_BOX (bbox), widget, FALSE, FALSE, 0);

	//detail
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	data->GR_detail = scrollwin;
	//gtk_scrolled_window_set_placement(GTK_SCROLLED_WINDOW (scrollwin), GTK_CORNER_TOP_RIGHT);
	treeview = create_list_transaction(LIST_TXN_TYPE_DETAIL, PREFS->lst_det_columns);
	data->LV_detail = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_paned_pack2 (GTK_PANED(vpaned), scrollwin, TRUE, TRUE);

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

