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

#include "ui-dialogs.h"
#include "ui-widgets.h"

#include "rep-budget.h"

#include "list-operation.h"
#include "gtk-chart-progress.h"
#include "gtk-dateentry.h"
#include "hbtk-switcher.h"

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
static void repbudget_compute(GtkWidget *widget, gpointer user_data);
static void repbudget_sensitive(GtkWidget *widget, gpointer user_data);
static void repbudget_update_daterange(GtkWidget *widget, gpointer user_data);
static void repbudget_update_chart(GtkWidget *widget, gpointer user_data);
static void repbudget_update_detail(GtkWidget *widget, gpointer user_data);
static void repbudget_detail(GtkWidget *widget, gpointer user_data);
static gchar *repbudget_compute_title(gint mode);
static void repbudget_selection(GtkTreeSelection *treeselection, gpointer user_data);


extern gchar *CYA_REPORT_MODE[];
extern HbKvData CYA_KIND[];


/* = = = = = = = = = = = = = = = = */


static void lst_repbud_to_string_row(GString *node, ToStringMode mode, HbRepBudMode uimode, GtkTreeModel *model, GtkTreeIter *iter)
{
guint32 key;
gchar sep, *name, *status;
gdouble spent, budget, result;
gint fulfilled;

	gtk_tree_model_get (model, iter,
		LST_BUDGET_KEY, &key,
		LST_BUDGET_NAME, &name,
		LST_BUDGET_SPENT, &spent,
		LST_BUDGET_BUDGET, &budget,
		LST_BUDGET_FULFILLED, &fulfilled,
		LST_BUDGET_RESULT, &result,
	    LST_BUDGET_STATUS, &status,
		-1);

	//#2033298 we get fullname for export
	if( uimode == REP_BUD_MODE_TOTAL )
	{
	Category *catitem = da_cat_get(key);

		if( catitem != NULL )
		{
			g_free(name);
			name = g_strdup( (catitem->key == 0) ? _("(no category)") : catitem->fullname );
		}
	}

	//2023696 add unbudgeted
	if( key != LST_BUDGET_POS_UNBUDGETED )
	{
		sep = (mode == HB_STRING_EXPORT) ? ';' : '\t';

		g_string_append (node, name );
		g_string_append_c(node, sep);
		_format_decimal(node, mode, spent);
		g_string_append_c(node, sep);
		_format_decimal(node, mode, budget);
		g_string_append_c(node, sep);
		g_string_append_printf(node, "%d %%", fulfilled);
		g_string_append_c(node, sep);
		_format_decimal(node, mode, result);
		g_string_append_c(node, sep);
		g_string_append (node, status );
		g_string_append_c(node, '\n');
	}

	//leak
	g_free(name);
	g_free(status);
}


static GString *lst_repbud_to_string(ToStringMode mode, GtkTreeView *treeview, HbRepBudMode uimode, gchar *title, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter, child;
gboolean valid;
gchar sep;

	node = g_string_new(NULL);
	sep = (mode == HB_STRING_EXPORT) ? ';' : '\t';

	// header
	g_string_append (node, (title == NULL) ? _("Category") : title );
	g_string_append_c (node, sep );
	g_string_append (node, _("Spent") );
	g_string_append_c (node, sep );
	g_string_append (node, _("Budget") );
	g_string_append_c (node, sep );
	g_string_append (node, _("Fulfilled") );
	g_string_append_c (node, sep );
	g_string_append (node, _("Result") );
	g_string_append_c (node, sep );
	g_string_append_c(node, '\n');

	// lines
	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		lst_repbud_to_string_row(node, mode, uimode, model, &iter);
		// children ?
		valid = gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &iter);
		while (valid)
		{
			lst_repbud_to_string_row(node, mode, uimode, model, &child);
			valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}


/*
**
** The function should return:
** a negative integer if the first value comes before the second,
** 0 if they are equal,
** or a positive integer if the first value comes after the second.
*/
static gint budget_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
gint pos1, pos2;
HbRepBudMode tmpmode;

	gint csid;
	GtkSortType cso;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &csid, &cso);
	tmpmode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(model), "mode-data"));

	//DB( g_print(" budget compare column=%d order=%s tmpmode=%d\n", sortcol, sort_order == GTK_SORT_ASCENDING ? "ASC" : "DESC" , tmpmode) );
	gtk_tree_model_get(model, a, LST_BUDGET_POS, &pos1, -1);
	gtk_tree_model_get(model, b, LST_BUDGET_POS, &pos2, -1);

	//total always at bottom
	if( pos1 == LST_BUDGET_POS_UNBUDGETED )
	{
		retval = cso == GTK_SORT_ASCENDING ? 1 : -1;
		//DB( g_print(" sort p1=%d ? p2=%d = %d\n", pos1, pos2, retval) );
	}
	else
	{
		if( pos2 == LST_BUDGET_POS_UNBUDGETED )
		{
			retval = cso == GTK_SORT_ASCENDING ? -1 : 1;
			//DB( g_print(" sort p1=%d ? p2=%d = %d\n", pos1, pos2, retval) )
		}
		else
		{

			switch( sortcol )
			{
				case LST_BUDGET_NAME:
				{
					if( tmpmode == REP_BUD_MODE_TIME )
					{
						//DB( g_print(" retval = %d - %d\n", pos1, pos2) );
						retval = pos1 - pos2;
					}
					else
					{
					gchar *entry1, *entry2;

						gtk_tree_model_get(model, a, LST_BUDGET_NAME, &entry1, -1);
						gtk_tree_model_get(model, b, LST_BUDGET_NAME, &entry2, -1);
						retval = hb_string_utf8_compare(entry1, entry2);
						//leak
						g_free(entry2);
						g_free(entry1);
					}
				}
				break;

				default:
				{
				gdouble val1, val2;

					gtk_tree_model_get(model, a, csid, &val1, -1);
					gtk_tree_model_get(model, b, csid, &val2, -1);
					//DB( g_print(" retval = %.2f - %2f\n", val1, val2) );
					retval = (gint)(val1 - val2);
				}
				break;
			}
		}
	}

	//DB( g_print(" retval = %d\n", retval) );
    return retval;
}


static void
lst_repbud_cell_data_function_name (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gchar *name;
gint tmpmode;

	tmpmode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(model), "mode-data"));

	if( tmpmode == REP_BUD_MODE_TOTAL )
	{
	Category *item;
	gint pos;
	guint32 key;
	gchar *markup = NULL;

		gtk_tree_model_get(model, iter,
			LST_BUDGET_POS, &pos,
			LST_BUDGET_KEY, &key,
			-1);

		if( pos != LST_BUDGET_POS_UNBUDGETED )
		{
			item = da_cat_get(key);
			if(item)
			{
				markup = item->typename;
			}
		}
		else
		{
			goto libname;
		}
		g_object_set(renderer, "markup", markup, NULL);
	}
	else
libname:
	{
	gchar *name;

		gtk_tree_model_get(model, iter,
			LST_BUDGET_NAME, &name,
			-1);

		g_object_set(renderer, "text", name, NULL);
		g_free(name);
	}
}


static void lst_repbud_cell_data_function_amount (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
   {
gdouble  value;
gchar *color;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gint column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter, column_id, &value, -1);

	if( value )
	{
		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, GLOBALS->kcur, GLOBALS->minor);

		if( column_id == LST_BUDGET_RESULT)
			color = get_minimum_color_amount (value, 0.0);
		else
			color = get_normal_color_amount(value);

		g_object_set(renderer,
			"foreground",  color,
			"text", buf,
			NULL);
	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}
}



static void lst_repbud_cell_data_function_result (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
   {
gdouble  value;
gchar *color;
gchar *status;
gint column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter,
		column_id, &value,
		LST_BUDGET_STATUS, &status,
		-1);

	if( value )
	{
		color = get_minimum_color_amount (value, 0.0);
		g_object_set(renderer,
			"foreground",  color,
			"text", status,
			NULL);
	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}

	//leak
	g_free(status);
}


static void lst_repbud_cell_data_function_fulfilled (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gdouble budget;
gint rawrate;
gchar buf[16];

	gtk_tree_model_get(model, iter,
		LST_BUDGET_BUDGET, &budget,
		LST_BUDGET_FULFILLED, &rawrate,
		-1);

	if( hb_amount_cmp(budget, 0.0) != 0 )
	{
		g_snprintf(buf, sizeof(buf), "%d %%", rawrate);
		g_object_set(renderer, "text", buf, NULL);
	}
	else
		g_object_set(renderer, "text", "", NULL);

}


static GtkTreeViewColumn *lst_repbud_column_create_amount(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbud_cell_data_function_amount, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}


/*
** create our statistic list
*/
static GtkWidget *lst_repbud_create(void)
{
GtkTreeStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	DB( g_print("\n[repbudget] create list\n") );

	/* create list store */
	store = gtk_tree_store_new(
	  	NUM_LST_BUDGET,
		G_TYPE_INT,		//pos
		G_TYPE_INT,		//key
		G_TYPE_STRING,	//name
		G_TYPE_DOUBLE,	//spent
		G_TYPE_DOUBLE,	//budget
		G_TYPE_INT,		//fulfilled
		G_TYPE_DOUBLE,	//result
		G_TYPE_STRING	//status
		);

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (view), PREFS->grid_lines);

	/* column: Name */
	renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer,
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Category"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbud_cell_data_function_name, GINT_TO_POINTER(LST_BUDGET_NAME), NULL);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_BUDGET_NAME);
	gtk_tree_view_column_set_sort_column_id (column, LST_BUDGET_NAME);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Expense */
	column = lst_repbud_column_create_amount(_("Spent"), LST_BUDGET_SPENT);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Income */
	column = lst_repbud_column_create_amount(_("Budget"), LST_BUDGET_BUDGET);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Fulfilled */
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Fulfilled"));
	gtk_tree_view_column_set_sort_column_id (column, LST_BUDGET_FULFILLED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbud_cell_data_function_fulfilled, GINT_TO_POINTER(LST_BUDGET_FULFILLED), NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Result */
	column = lst_repbud_column_create_amount(_("Result"), LST_BUDGET_RESULT);
	//right part
	renderer = gtk_cell_renderer_text_new ();
	//g_object_set(renderer, "xalign", 0.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_repbud_cell_data_function_result, GINT_TO_POINTER(LST_BUDGET_RESULT), NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* sort */
	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), budget_listview_compare_func, NULL, NULL);
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_BUDGET_NAME, GTK_SORT_ASCENDING);

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_NAME  , budget_listview_compare_func, GINT_TO_POINTER(LST_BUDGET_NAME), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_SPENT , budget_listview_compare_func, GINT_TO_POINTER(LST_BUDGET_SPENT), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_BUDGET, budget_listview_compare_func, GINT_TO_POINTER(LST_BUDGET_BUDGET), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_FULFILLED, budget_listview_compare_func, GINT_TO_POINTER(LST_BUDGET_FULFILLED), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_BUDGET_RESULT, budget_listview_compare_func, GINT_TO_POINTER(LST_BUDGET_RESULT), NULL);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(view), FALSE);

	return(view);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */



static gchar *budget_mode_label(gint mode)
{
gchar *retval = (mode == REP_BUD_MODE_TIME) ? _("Month") : _("Category");
	return (gchar *)retval;
}


/* action functions -------------------- */
static void repbudget_action_viewlist(GtkWidget *toolbutton, gpointer user_data)
{
struct repbudget_data *data = user_data;

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 0);
	repbudget_sensitive(data->window, NULL);
}

static void repbudget_action_viewstack(GtkWidget *toolbutton, gpointer user_data)
{
struct repbudget_data *data = user_data;

	//#1860905 we redraw chart in case a sort changed
	repbudget_update_chart(data->window, NULL);

	gtk_notebook_set_current_page(GTK_NOTEBOOK(data->GR_result), 1);
	repbudget_sensitive(data->window, NULL);
}

static void repbudget_action_print(GtkWidget *toolbutton, gpointer user_data)
{
struct repbudget_data *data = user_data;
gint tmpmode, page;
gchar *name, *coltitle, *title;

	tmpmode = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mode));
	page    = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));

	name  = g_strdup_printf("hb-repbudget_%s", budget_mode_label(tmpmode) );

	if( page == 0 )
	{
	GString *node;

		title = repbudget_compute_title(tmpmode);

		coltitle = budget_mode_label(tmpmode);
		node = lst_repbud_to_string(HB_STRING_PRINT, GTK_TREE_VIEW(data->LV_report), tmpmode, coltitle, TRUE);
		gint8 leftcols[4] = { 0, 5, -1 };
		hb_print_listview(GTK_WINDOW(data->window), node->str, leftcols, title, name, FALSE);
		g_string_free(node, TRUE);
		g_free(title);
	}
	else
	{
		gtk_chart_progress_print(GTK_CHARTPROGRESS(data->RE_progress), GTK_WINDOW(data->window), PREFS->path_export, name);
	}

	g_free(name);
}


/* ======================== */


static guint _date_getmonth(guint date)
{
GDate *date1;
guint month;

	date1 = g_date_new_julian(date);
	month = g_date_get_month(date1);

	/*#if MYDEBUG == 1
		g_print("\n[repbudget] getmonth\n");
		gchar buffer1[128];
		g_date_strftime (buffer1, 128-1, "%x", date1);
		g_print("  date is '%s', month=%d\n", buffer1, month);
	#endif*/

	g_date_free(date1);

	return(month);
}


//#
static void _date_clamp_today(Filter *flt, gboolean today)
{
	if(today)
	{
		if( (GLOBALS->today > flt->mindate) && (GLOBALS->today < flt->maxdate) )
			flt->maxdate = GLOBALS->today;
	}
}


static guint _date_countmonth(guint32 mindate, guint32 maxdate)
{
GDate *date1, *date2;
guint nbmonth;

	if( mindate > maxdate )
		return 0;

	date1 = g_date_new_julian(mindate);
	date2 = g_date_new_julian(maxdate);

	nbmonth = ((g_date_get_year(date2) - g_date_get_year(date1)) * 12) + g_date_get_month(date2) - g_date_get_month(date1) + 1;

	g_date_free(date2);
	g_date_free(date1);

	return(nbmonth);
}


static gdouble budget_compute_result(gdouble budget, gdouble spent)
{
gdouble retval;

	//original formula
	//result = ABS(budget) - ABS(spent);

	retval = spent - budget;

	return hb_amount_round(retval, 2);
}


static void repbudget_cb_date_monthyear(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gboolean tmptoday;
gdouble minval, maxval;

	DB( g_print("\n[repbudget] monthyear change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmptoday = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_untiltoday));

	//todo
	g_signal_handler_block(data->SB_mindate, data->handler_id[HID_REPBUDGET_MINMONTHYEAR]);
	g_signal_handler_block(data->SB_maxdate, data->handler_id[HID_REPBUDGET_MAXMONTHYEAR]);

	//ensure always minval < maxval
	minval = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->SB_mindate));
	maxval = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->SB_maxdate));

	if( minval > maxval)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->SB_mindate), minval);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->SB_maxdate), minval);
	}

	if( maxval < minval)
	{
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->SB_mindate), maxval);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->SB_maxdate), maxval);
	}

	data->filter->mindate = hbtk_monthyear_getmin(GTK_SPIN_BUTTON(data->SB_mindate));
	data->filter->maxdate = hbtk_monthyear_getmax(GTK_SPIN_BUTTON(data->SB_maxdate));

	_date_clamp_today(data->filter, tmptoday);

	g_signal_handler_unblock(data->SB_maxdate, data->handler_id[HID_REPBUDGET_MAXMONTHYEAR]);
	g_signal_handler_unblock(data->SB_mindate, data->handler_id[HID_REPBUDGET_MINMONTHYEAR]);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPBUDGET_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPBUDGET_RANGE]);

	repbudget_sensitive(widget, user_data);
	repbudget_compute(widget, NULL);
	repbudget_update_daterange(widget, NULL);
}


static gchar *
repbudget_compute_title(gint mode)
{
gchar *title;

	if(	mode == REP_BUD_MODE_TOTAL )
		title = g_strdup(_("Budget by category"));
	else
		title = g_strdup(_("Budget by month"));

	return title;
}


/*static void repbudget_date_change(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	DB( g_print("\n[repbudget] date change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->filter->mindate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_mindate));
	data->filter->maxdate = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_maxdate));

	// set min/max date for both widget
	gtk_date_entry_set_maxdate(GTK_DATE_ENTRY(data->PO_mindate), data->filter->maxdate);
	gtk_date_entry_set_mindate(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->mindate);

	g_signal_handler_block(data->CY_range, data->handler_id[HID_REPBUDGET_RANGE]);
	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_MISC_CUSTOM);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_REPBUDGET_RANGE]);


	repbudget_compute(widget, NULL);
	repbudget_update_daterange(widget, NULL);

}*/


static void repbudget_cb_date_range(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gboolean tmptoday;
gint range;

	DB( g_print("\n[repbudget] range change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_range));
	tmptoday = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_untiltoday));

	if(range != FLT_RANGE_MISC_CUSTOM)
	{
		filter_preset_daterange_set(data->filter, range, 0);
		if( range == FLT_RANGE_MISC_ALLDATE )
		{
			data->filter->mindate = hb_date_get_jbound(data->filter->mindate, HB_DATE_BOUND_FIRST);
			data->filter->maxdate = hb_date_get_jbound(data->filter->maxdate, HB_DATE_BOUND_LAST);
		}

		_date_clamp_today(data->filter, tmptoday);

		//5.7
		g_signal_handler_block(data->SB_mindate, data->handler_id[HID_REPBUDGET_MINMONTHYEAR]);
		g_signal_handler_block(data->SB_maxdate, data->handler_id[HID_REPBUDGET_MAXMONTHYEAR]);
		hbtk_monthyear_set(GTK_SPIN_BUTTON(data->SB_mindate), data->filter->mindate);
		hbtk_monthyear_set(GTK_SPIN_BUTTON(data->SB_maxdate), data->filter->maxdate);
		g_signal_handler_unblock(data->SB_maxdate, data->handler_id[HID_REPBUDGET_MAXMONTHYEAR]);
		g_signal_handler_unblock(data->SB_mindate, data->handler_id[HID_REPBUDGET_MINMONTHYEAR]);

		repbudget_compute(widget, NULL);
		repbudget_update_daterange(widget, NULL);
	}
}


static void repbudget_update_daterange(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gchar *daterange;

	DB( g_print("\n[repbudget] update daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	daterange = filter_daterange_text_get(data->filter);
	gtk_label_set_markup(GTK_LABEL(data->TX_daterange), daterange);
	g_free(daterange);
}


static void repbudget_toggle_detail(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbudget] toggle detail\n") );

	data->detail ^= 1;

	repbudget_update_detail(widget, user_data);
	repbudget_sensitive(widget, NULL);

}


static void repbudget_detail_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
{
struct repbudget_data *data;
Transaction *active_txn;
gboolean result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print ("\n[repbudget] A detail row has been double-clicked!\n") );

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
			repbudget_compute(data->window, NULL);

			if( path != NULL )
			{
				gtk_tree_selection_select_path(treeselection, path);
				gtk_tree_path_free(path);
			}

		}

		da_transaction_free (old_txn);
	}
}


static void repbudget_update_detail(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbudget] update detail\n") );

	if(GTK_IS_TREE_VIEW(data->LV_report))
	{

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
				gtk_tree_model_get(model, &iter, LST_BUDGET_KEY, &key, -1);

				//DB( g_print(" active is %d\n", key) );

				repbudget_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
			}


			gtk_widget_show(data->GR_detail);
		}
		else
			gtk_widget_hide(data->GR_detail);


	}
}


static void repbudget_export_result_clipboard(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct repbudget_data *data;
GtkClipboard *clipboard;
GString *node;
gint tmpmode;
gchar *coltitle;

	DB( g_print("\n[repbudget] export result clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpmode = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_mode));
	coltitle = budget_mode_label(tmpmode);

	node = lst_repbud_to_string(HB_STRING_CLIPBOARD, GTK_TREE_VIEW(data->LV_report), tmpmode, coltitle, TRUE);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void repbudget_export_result_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct repbudget_data *data;
gchar *filename = NULL;
GString *node;
GIOChannel *io;
gchar *name;
gint tmpmode;
gchar *coltitle;

	DB( g_print("\n[repbudget] export result csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpmode = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_mode));
	name = g_strdup_printf("hb-repbudget_%s.csv", budget_mode_label(tmpmode) );

	if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, name) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );
		io = g_io_channel_new_file(filename, "w", NULL);
		if(io != NULL)
		{
			coltitle = budget_mode_label(tmpmode);
			node = lst_repbud_to_string(HB_STRING_EXPORT, GTK_TREE_VIEW(data->LV_report), tmpmode, coltitle, FALSE);
			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
		g_free( filename );
	}
	g_free(name);
}


static void repbudget_export_detail_clipboard(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct repbudget_data *data;
GtkClipboard *clipboard;
GString *node;
guint flags;

	DB( g_print("\n[repbudget] export detail clipboard\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	flags = LST_TXN_EXP_CLR | LST_TXN_EXP_PMT | LST_TXN_EXP_CAT | LST_TXN_EXP_TAG;
	node = list_txn_to_string(GTK_TREE_VIEW(data->LV_detail), TRUE, FALSE, FALSE, flags);

	clipboard = gtk_clipboard_get_default(gdk_display_get_default());
	gtk_clipboard_set_text(clipboard, node->str, node->len);

	g_string_free(node, TRUE);
}


static void repbudget_export_detail_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct repbudget_data *data;
gchar *filepath = NULL;
GString *node;
GIOChannel *io;
gchar *name;
gint tmpmode;
gboolean hassplit, hasstatus;

	DB( g_print("\n[repbudget] export detail csv\n") );

	data = user_data;
	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpmode = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_mode));
	name = g_strdup_printf("hb-repbudget-detail_%s.csv", budget_mode_label(tmpmode));
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


static void repbudget_detail(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gint active = GPOINTER_TO_INT(user_data);
GList *list;
guint tmpmode, tmptype;
GtkTreeModel *model;
GtkTreeIter  iter, child;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[repbudget] detail\n") );

	DB( g_print(" active: %d\n", active) );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

	if(data->detail && data->txn_queue && active != -1)
	{
		/* clear and detach our model */
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), NULL); /* Detach model from view */

		tmpmode = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_mode));
		tmptype = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));

		/* fill in the model */
		list = g_queue_peek_head_link(data->txn_queue);
		while (list != NULL)
		{
		Transaction *ope = list->data;
		gdouble dtlamt = ope->amount;
		gint pos = 0;
		gboolean match = FALSE;
		GPtrArray *matchsplit = NULL;

			DB( g_print(" ope: %s :: acc=%d, cat=%d, mnt=%.2f\n", ope->memo, ope->kacc, ope->kcat, ope->amount) );

			//filter here
			//#2039995 filter txn with type
			// flt: expense
			if( tmptype == REPORT_TYPE_EXPENSE && (ope->flags & GF_INCOME) )
				goto txnnext;
			// flt: income
			if( tmptype == REPORT_TYPE_INCOME && !(ope->flags & GF_INCOME) )
				goto txnnext;

			//time: month
			if( tmpmode == REP_BUD_MODE_TIME )
			{
				pos = report_interval_get_pos(REPORT_INTVL_MONTH, data->filter->mindate, ope);
				if( pos == active )
				{
					if( !(ope->flags & OF_SPLIT) )
					{
						match = category_key_budget_active(ope->kcat);
					}
					else
					{
					guint nbsplit = da_splits_length(ope->splits);

						dtlamt = 0.0;
						matchsplit = g_ptr_array_new();
						for(guint i=0;i<nbsplit;i++)
						{
						Split *split = da_splits_get(ope->splits, i);

							if( category_key_budget_active(split->kcat) )
							{
								match = TRUE;
								dtlamt += split->amount;
								g_ptr_array_add(matchsplit, split);
							}
						}
					}
				}
			}
			//total: category
			else
			{
				if( active != LST_BUDGET_POS_UNBUDGETED )
				{
				Category *active_cat;
				gboolean is_subcat = FALSE;

					//get cat/subcat
					active_cat = da_cat_get(active);
					if( active_cat )
						is_subcat = (active_cat->parent == 0) ? FALSE : TRUE;

					//category
					if( !(ope->flags & OF_SPLIT) )
					{
						pos = category_report_id(ope->kcat, is_subcat);
						if( pos == active )
							match = TRUE;
					}
					else
					{
					guint nbsplit = da_splits_length(ope->splits);

						dtlamt = 0.0;
						matchsplit = g_ptr_array_new();
						for(guint i=0;i<nbsplit;i++)
						{
						Split *split = da_splits_get(ope->splits, i);

							pos = category_report_id(split->kcat, is_subcat);
							if( pos == active )
							{
								match = TRUE;
								dtlamt += split->amount;
								g_ptr_array_add(matchsplit, split);
							}
						}
					}
				}
				// LST_BUDGET_POS_UNBUDGETED
				else
				{
					DB( g_print(" unbudgeted test\n") );
						// we match txn cat with no budget set
					if( !(ope->flags & OF_SPLIT) )
					{
						match = category_key_unbudgeted(ope->kcat);
						DB( g_print(" unbudgeted match: %d\n", match) );
					}
					//splits
					else
					{
					guint nbsplit = da_splits_length(ope->splits);

						dtlamt = 0.0;
						matchsplit = g_ptr_array_new();
						for(guint i=0;i<nbsplit;i++)
						{
						Split *split = da_splits_get(ope->splits, i);

							match = category_key_unbudgeted(split->kcat);
							dtlamt += split->amount;
							g_ptr_array_add(matchsplit, split);
						}

					}
				}
			}

			//insert
			if( match == TRUE )
			{
				gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &iter, NULL, -1,
					MODEL_TXN_POINTER, ope,
			        MODEL_TXN_SPLITAMT, dtlamt,
					-1);

				if( matchsplit != NULL )
				{
					for(guint i=0;i<matchsplit->len;i++)
					{
						gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &child, &iter, -1,
							MODEL_TXN_POINTER, ope,
							MODEL_TXN_SPLITPTR, g_ptr_array_index(matchsplit, i),
							-1);
					}

					DB( g_print(" free matchsplit\n") );
					g_ptr_array_free(matchsplit, TRUE);
					matchsplit = NULL;
				}

			}
txnnext:
			list = g_list_next(list);
		}

		/* Re-attach model to view */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_detail), model);
		g_object_unref(model);

		gtk_tree_view_columns_autosize( GTK_TREE_VIEW(data->LV_detail) );

	}
}


static void repbudget_update_total(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	DB( g_print("\n[repbudget] update total\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	hb_label_set_colvalue(GTK_LABEL(data->TX_total[0]), data->total_spent, GLOBALS->kcur, GLOBALS->minor);
	hb_label_set_colvalue(GTK_LABEL(data->TX_total[1]), data->total_budget, GLOBALS->kcur, GLOBALS->minor);
	hb_label_set_colvalue(GTK_LABEL(data->TX_total[2]), budget_compute_result(data->total_budget, data->total_spent), GLOBALS->kcur, GLOBALS->minor);
}


static void repbudget_update_chart(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
GtkTreeModel *model;
gint tmpmode;
gchar *title;

	DB( g_print("\n[repbudget] update chart\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmpmode = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_mode));
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));

	/* update stack chart */
	title = repbudget_compute_title(tmpmode);

	ui_chart_progress_set_currency(GTK_CHARTPROGRESS(data->RE_progress), GLOBALS->kcur);

	/* set chart color scheme */
	ui_chart_progress_set_color_scheme(GTK_CHARTPROGRESS(data->RE_progress), PREFS->report_color_scheme);
	ui_chart_progress_set_dualdatas(GTK_CHARTPROGRESS(data->RE_progress), model, _("Budget"), _("Result"), title, NULL);

	g_free(title);
}


static void repbudget_fill_budget_for_category(Category *catitem, gdouble *tmp_budget, gint startmonth, gint nbmonth)
{

	if( catitem == NULL)
		return;

	if( (catitem->flags & GF_BUDGET) == FALSE )
		return;

	//debug
	/*#if MYDEBUG == 1
	gint k;
	g_print("    budget vector: ");
	for(k=0;k<13;k++)
		g_print( " %d:[%.2f]", k, entry->budget[k]);
	g_print("\n");
	#endif*/

	DB( g_print(" %d:'%s' issub=%d hasbudget=%d custom=%d\n",
		catitem->key, catitem->name, (catitem->flags & GF_SUB), (catitem->flags & GF_BUDGET), (catitem->flags & GF_CUSTOM)) );

	// same value each month ?
	if(!(catitem->flags & GF_CUSTOM))
	{
		DB( g_print("    - monthly %.2f\n", catitem->budget[0]) );
		tmp_budget[catitem->key] += catitem->budget[0]*nbmonth;
		if( catitem->flags & GF_SUB )
		{
			tmp_budget[catitem->parent] += catitem->budget[0]*nbmonth;
		}
	}
	//otherwise	sum each month from mindate month
	else
	{
	gint j, month = startmonth;

		DB( g_print("    - custom each month for %d months\n", nbmonth) );
		for(j=0;j<nbmonth;j++)
		{
			DB( g_print("      j=%d month=%d budg=%.2f\n", j, month, catitem->budget[month]) );
			tmp_budget[catitem->key] += catitem->budget[month];
			if( catitem->flags & GF_SUB )
			{
				tmp_budget[catitem->parent] += catitem->budget[month];
			}
			month++;
			if(month > 12) { month = 1; }
		}
	}

}


static void _repbudget_compute_cat_spent(guint32 key, gint tmptype, gdouble amount, gdouble *tmp_spent, gdouble *unbudgeted)
{
Category *cat;

	//5.7.3 filter on type
	if( (tmptype == REPORT_TYPE_EXPENSE && amount > 0) ||  (tmptype == REPORT_TYPE_INCOME && amount< 0) )
		return;

	cat = da_cat_get(key);
	if(cat)
	{
		DB( g_print(" cat %02d:%02d (sub=%d)\n", cat->parent, cat->key, (cat->flags & GF_SUB)) );

		if( (cat->flags & (GF_FORCED|GF_BUDGET)) )
		{
			DB( g_print("  + spend %.2f to cat %d\n", amount, cat->key) );
			tmp_spent[cat->key] += amount;
		}
		//#2023696
		else
		{
			//here if prefs we ignore if this is subcat but aprent have budget
			if( category_key_unbudgeted(key) == TRUE )
				*unbudgeted += amount;
		}

		//#1825653 subcat without budget must be computed
		if( (cat->flags & GF_SUB) )
		{
		Category *pcat = da_cat_get(cat->parent);

			if(pcat)
			{
				if( (cat->flags & (GF_FORCED|GF_BUDGET)) || (pcat->flags & (GF_FORCED|GF_BUDGET)) )
				{
					DB( g_print("  + spend %.2f to parent %d\n", amount, cat->parent) );
					tmp_spent[pcat->key] += amount;
				}

			}
		}
	}

}


static void budget_compute_category(struct repbudget_data *data, GtkTreeModel *model, gdouble *tmp_spent, gdouble *tmp_budget, guint startmonth, guint nbmonth, gboolean tmptype, gboolean tmponlyout)
{
GList *lcat, *list;
guint id, i;
gdouble unbudgeted;

	DB( g_print("\n+ compute budget for category\n") );

	// fill tmp_budget
	//get ordered cat/subcat
	DB( g_print(" + cat fill budget\n") );
	lcat = category_glist_sorted(HB_GLIST_SORT_KEY);
	list = lcat;
	while (list != NULL)
	{
	Category *catitem = list->data;

		repbudget_fill_budget_for_category(catitem, tmp_budget, startmonth, nbmonth);
		list = g_list_next(list);
	}

	// compute spent for each transaction
	// fill tmp_spent
	DB( g_print(" + cat fill spent\n") );

	unbudgeted = 0.0;
	list = g_queue_peek_head_link(data->txn_queue);
	while (list != NULL)
	{
	Transaction *ope = list->data;

		DB( g_print(" ope: %s :: acc=%d, cat=%d, mnt=%.2f\n", ope->memo, ope->kacc, ope->kcat, ope->amount) );

		if( ope->flags & OF_SPLIT )
		{
		guint nbsplit = da_splits_length(ope->splits);
		Split *split;

			for(i=0;i<nbsplit;i++)
			{
				split = da_splits_get(ope->splits, i);
				_repbudget_compute_cat_spent(split->kcat, tmptype, hb_amount_base(split->amount, ope->kcur), tmp_spent, &unbudgeted);
			}
		}
		else
		{
			_repbudget_compute_cat_spent(ope->kcat, tmptype, hb_amount_base(ope->amount, ope->kcur), tmp_spent, &unbudgeted);
		}

		list = g_list_next(list);
	}


	DB( g_print("\n -- populate budget listview --\n") );

	DB( g_printf(" type=%d, onlyout=%d\n", tmptype, tmponlyout) );
	DB( g_printf(" unbudgeted=%.2f\n\n", unbudgeted) );

	id = 0;
	list = lcat;
	while (list != NULL)
	{
	Category *catitem = list->data;
	gchar *name;
	gboolean outofbudget;
	gboolean doinsert;
	gdouble result, rawrate;
	gint fulfilled;
	gchar *status;

		if( catitem == NULL)
			continue;

		name = catitem->key == 0 ? "(None)" : catitem->name;
		guint pos = catitem->key;

		DB( g_print(" eval %d: %d '%s' b:%d f:%d : spen=%.2f bud=%.2f \n",
			id, pos, name, (catitem->flags & GF_BUDGET), (catitem->flags & GF_FORCED),
			tmp_spent[pos], tmp_budget[pos] ) );

		doinsert = FALSE;
		if( (hb_amount_cmp(tmp_budget[pos], 0.0) != 0) || (hb_amount_cmp(tmp_spent[pos], 0.0) != 0) )
			doinsert = TRUE;

		//filter expense/income
		if( tmptype != REPORT_TYPE_ALL )
		{
		Category *parent = catitem;
		gint cattype = 0;

			if(catitem->parent > 0)
				parent = da_cat_get(catitem->parent);

			cattype = category_type_get(parent);
			if( (tmptype == REPORT_TYPE_EXPENSE) && (cattype > 0) )
			{
				doinsert = FALSE;
				DB( g_printf("  >skip: expense filter\n") );
			}
			if( (tmptype == REPORT_TYPE_INCOME) && (cattype < 0) )
			{
				doinsert = FALSE;
				DB( g_printf("  >skip: income filter\n") );
			}
		}

		result = budget_compute_result(tmp_budget[pos], tmp_spent[pos]);
		rawrate = 0.0;
		if(ABS(tmp_budget[pos]) > 0)
		{
			rawrate = tmp_spent[pos] / tmp_budget[pos];
		}
		else if(tmp_budget[pos] == 0.0)
			rawrate = ABS(tmp_spent[pos]);

		status = "";
		outofbudget = FALSE;
		if( result )
		{
			if(rawrate > 1.0)
			{
				status = _(" over");
				outofbudget = TRUE;
			}
			else
			{
				if(tmp_budget[pos] < 0.0)
					status = _(" left");
				else if(tmp_budget[pos] > 0.0)
				{
					status = _(" under");
					outofbudget = TRUE;
				}
			}
		}

		if((tmponlyout == TRUE && outofbudget == FALSE) && !(catitem->flags & GF_MIXED) )
		{
			DB( g_printf("  >skip: only out filter is on\n") );
			doinsert = FALSE;
		}

		if( doinsert )
		{
		GtkTreeIter parent;
		GtkTreeIter	*tmpparent = NULL;

			Category *tmpcat = da_cat_get(pos);
			if( tmpcat != NULL)
			{
				//if( lst_repbud_get_top_level (GTK_TREE_MODEL(model), tmpcat->parent, &parent) == TRUE )
				if( hbtk_tree_store_get_top_level(GTK_TREE_MODEL(model), LST_BUDGET_KEY, tmpcat->parent, &parent) )
				{
					tmpparent = &parent;
				}
				else
				{
					//DB( g_print(" !! no parent %d found for %d '%s'\n", entry->parent, entry->key, entry->fullname) );
				}
			}

			DB( g_print("  >insert '%s' s:%.2f b:%.2f r:%.2f (%%%.2f) '%s' '%d'\n", name, tmp_spent[pos], tmp_budget[pos], result, rawrate, status, outofbudget ) );

			//5.7.3 dont fulfill if no budget
			fulfilled = 0;
			if( hb_amount_cmp(tmp_budget[pos], 0.0) != 0 )
				fulfilled = (gint)hb_amount_round(rawrate*100, 0);

			gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), NULL, tmpparent, -1,
				LST_BUDGET_POS, id++,
				LST_BUDGET_KEY, pos,
				LST_BUDGET_NAME, name,
				LST_BUDGET_SPENT, tmp_spent[pos],
				LST_BUDGET_BUDGET, tmp_budget[pos],
				//#2043223
				//LST_BUDGET_FULFILLED, (gint)(rawrate*100),
				LST_BUDGET_FULFILLED, fulfilled,
				LST_BUDGET_RESULT, result,
				LST_BUDGET_STATUS, status,
				-1);

			//#2036703 only sum level1, as level are already into the sum
			if( catitem->parent == 0 )
			{
				data->total_spent  += tmp_spent[pos];
				data->total_budget += tmp_budget[pos];
			}
		}

		list = g_list_next(list);
	}

	//2023696 add unbudgeted
	if( hb_amount_cmp(unbudgeted, 0.0) != 0 )
	{
		gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), NULL, NULL, -1,
			LST_BUDGET_POS, LST_BUDGET_POS_UNBUDGETED,
			LST_BUDGET_KEY, LST_BUDGET_POS_UNBUDGETED,
			LST_BUDGET_NAME, _("(unbudgeted)"),
			LST_BUDGET_SPENT, unbudgeted,
			-1);
	}

	DB( g_print(" -- end populate category\n") );

	// free cat list
	g_list_free(lcat);

}


static void budget_compute_month(struct repbudget_data *data, GtkTreeModel *model, gdouble *tmp_spent, gdouble *tmp_budget, guint startmonth, guint nbmonth, gboolean tmptype, gboolean tmponlyout)
{
GList *lcat, *list;
GtkTreeIter  iter;
guint pos, i;

	DB( g_print("\n+ compute budget for month\n") );

	// fill tmp_budget
	//get ordered cat/subcat
	DB( g_print(" + month compute budget, startmonth %d\n", startmonth) );
	lcat = category_glist_sorted(HB_GLIST_SORT_KEY);
	list = lcat;
	while (list != NULL)
	{
	Category *catitem = list->data;

		DB( g_print(" %d:'%s' issub=%d hasbudget=%d custom=%d\n",
			catitem->key, catitem->name, (catitem->flags & GF_SUB), (catitem->flags & GF_BUDGET), (catitem->flags & GF_CUSTOM)) );

		// flt: expense
		if( tmptype == REPORT_TYPE_EXPENSE && (catitem->flags & GF_INCOME) )
			goto budnext;
		// flt: income
		if( tmptype == REPORT_TYPE_INCOME && !(catitem->flags & GF_INCOME) )
			goto budnext;

		if( (catitem->flags & GF_BUDGET) )
		{
			// same value each month ?
			if(!(catitem->flags & GF_CUSTOM))
			{
				DB( g_print(" add same %.2f for '%s'\n =>", catitem->budget[0], catitem->fullname) );
				//we add same amount each month
				for(pos=0 ; pos<nbmonth ; pos++)
				{
					DB( g_print(" %d", pos) );
					tmp_budget[pos] += catitem->budget[0];
				}
				DB( g_print("\n") );
			}
			// different value each month
			else
			{
			guint month = startmonth;

				DB( g_print(" add custom for '%s'\n =>", catitem->fullname) );
				for(pos=0 ; pos<nbmonth ; pos++)
				{
					DB( g_print(" + add bud[%3d] month=%2d %.2f\n", pos, month, catitem->budget[month]) );
					tmp_budget[pos] += catitem->budget[month];
					month++;
					if(month > 12) { month = 1; }
				}
			}
		}
budnext:
		list = g_list_next(list);
	}

	// free cat list
	g_list_free(lcat);


	// compute spent for each transaction
	// fill tmp_spent
	DB( g_print(" + month compute spent\n") );

	list = g_queue_peek_head_link(data->txn_queue);
	while (list != NULL)
	{
	Transaction *ope = list->data;

		pos = report_interval_get_pos(REPORT_INTVL_MONTH, data->filter->mindate, ope);

		DB( g_print(" ope: %s :: acc=%d, cat=%d, mnt=%.2f, pos=%d\n", ope->memo, ope->kacc, ope->kcat, ope->amount, pos) );

		//#2039995 filter txn with type
		if(
			(tmptype == REPORT_TYPE_EXPENSE &&  (ope->flags & GF_INCOME)) // flt: expense
			||
			(tmptype == REPORT_TYPE_INCOME && !(ope->flags & GF_INCOME)) // flt: income txn
		)
		{
			DB( g_print("  skipped by type filter\n") );
			goto txnnext;
		}

		if( ope->flags & OF_SPLIT )
		{
		guint nbsplit = da_splits_length(ope->splits);
		Split *split;

			for(i=0;i<nbsplit;i++)
			{
				split = da_splits_get(ope->splits, i);
				if( category_key_budget_active(split->kcat) == TRUE )
				{
					tmp_spent[pos] += hb_amount_base(split->amount, ope->kcur);
				}
				else
				{
					DB( g_print("  skipped not budget|forced\n") );
				}
			}
		}
		else
		{
			if( category_key_budget_active(ope->kcat) == TRUE )
			{
				tmp_spent[pos] += hb_amount_base(ope->amount, ope->kcur);
			}
			else
			{
				DB( g_print("  skipped not budget|forced\n") );
			}
		}

		DB( g_print(" spent[%d]=%.2f\n", pos, tmp_spent[pos]) );

txnnext:
		list = g_list_next(list);
	}


	DB( g_print("\n -- populate budget listview --\n") );

	for( i=0 ; i < nbmonth ; i++ )
	{
	guint pos = i;
	gboolean outofbudget;
	gchar intvlname[64];

		report_interval_snprint_name(intvlname, sizeof(intvlname)-1, REPORT_INTVL_MONTH, data->filter->mindate, i);


		DB( g_print(" eval %d '%s' : spen=%.2f bud=%.2f \n", pos, intvlname, tmp_spent[pos], tmp_budget[pos] ) );

		if( tmp_budget[pos] /*|| tmp_spent[pos]*/)
		{
		gdouble result, rawrate;
		gchar *status;

			result = budget_compute_result(tmp_budget[pos], tmp_spent[pos]);
			rawrate = 0.0;
			if(ABS(tmp_budget[pos]) > 0)
			{
				rawrate = tmp_spent[pos] / tmp_budget[pos];
			}
			else if(tmp_budget[pos] == 0.0)
				rawrate = ABS(tmp_spent[pos]);

			status = "";
			outofbudget = FALSE;
			if( result )
			{
				if(rawrate > 1.0)
				{
					status = _(" over");
					outofbudget = TRUE;
				}
				else
				{
					if(tmp_budget[pos] < 0.0)
						status = _(" left");
					else if(tmp_budget[pos] > 0.0)
					{
						status = _(" under");
						outofbudget = TRUE;
					}
				}
			}

			if(tmponlyout == TRUE && outofbudget == FALSE)
				goto nextins;

			DB( g_print(" => insert '%s' s:%.2f b:%.2f r:%.2f (%%%.2f) '%s' '%d'\n\n", intvlname, tmp_spent[pos], tmp_budget[pos], result, rawrate, status, outofbudget ) );

			gtk_tree_store_insert_with_values (GTK_TREE_STORE(model), &iter, NULL, -1,
				LST_BUDGET_POS, pos,
				LST_BUDGET_KEY, pos,
				LST_BUDGET_NAME, intvlname,
				LST_BUDGET_SPENT, tmp_spent[pos],
				LST_BUDGET_BUDGET, tmp_budget[pos],
				LST_BUDGET_FULFILLED, (gint)(rawrate*100),
				LST_BUDGET_RESULT, result,
				LST_BUDGET_STATUS, status,
				-1);

nextins:
			data->total_spent  += tmp_spent[pos];
			data->total_budget += tmp_budget[pos];
		}

	}

	DB( g_print(" -- end populate month\n") );

}


static void repbudget_compute(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
GtkTreeModel *model;
gint tmpmode, tmptype, tmponlyout;
guint32 mindate, maxdate;
guint nbkeycat, nbmonth, n_result;
gdouble *tmp_spent, *tmp_budget;

	DB( g_print("\n[repbudget] compute\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	mindate = data->filter->mindate;
	maxdate = data->filter->maxdate;

	//0=Total / 1=Time
	tmpmode    = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_mode));
	tmptype    = hbtk_combo_box_get_active_id(GTK_COMBO_BOX(data->CY_type));
	tmponlyout = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_onlyout));

	DB( g_print(" mode:%d type:%d only:%d\n", tmpmode, tmptype, tmponlyout) );

	//#2019876 return is invalid date range
	if(maxdate < mindate) return;
	if( data->filter->maxdate < data->filter->mindate )
		return;

	//TODO: not necessary until date range change
	//free previous txn
	if(data->txn_queue != NULL)
		g_queue_free (data->txn_queue);
	data->txn_queue = hbfile_transaction_get_partial_budget(mindate, maxdate);

	nbkeycat = da_cat_get_max_key();
	nbmonth  = _date_countmonth(mindate, maxdate);
	DB( g_print(" date: min=%d max=%d nbcat=%d nbmonth=%d\n", mindate, maxdate, nbkeycat, nbmonth) );

	// allocate some memory
	n_result = (tmpmode == REP_BUD_MODE_TIME) ? nbmonth : nbkeycat;
	tmp_spent  = g_malloc0((n_result+1) * sizeof(gdouble));
	tmp_budget = g_malloc0((n_result+1) * sizeof(gdouble));

	if(tmp_spent && tmp_budget)
	{
	guint startmonth;

		// compute the results
		data->total_spent = 0.0;
		data->total_budget = 0.0;
		startmonth = _date_getmonth(mindate);

		DB( g_print("\nblock handlers\n") );
		g_signal_handlers_block_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report))), G_CALLBACK (repbudget_selection), NULL);
		g_signal_handlers_block_by_func (G_OBJECT (data->LV_detail), G_CALLBACK (repbudget_detail_onRowActivated), NULL);

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail));
		gtk_tree_store_clear (GTK_TREE_STORE(model));

		DB( g_print("\nclear and detach model\n") );
		// clear and detach our model
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report));
		gtk_tree_store_clear (GTK_TREE_STORE(model));
		g_object_ref(model); // Make sure the model stays with us after the tree view unrefs it
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), NULL); // Detach model from view


		// compute budget for each category
		g_object_set_data(G_OBJECT(model), "mode-data", GINT_TO_POINTER(tmpmode));
		if( tmpmode == REP_BUD_MODE_TIME )
		{
			budget_compute_month(data, model, tmp_spent, tmp_budget, startmonth, nbmonth, tmptype, tmponlyout);
		}
		else
		{
			budget_compute_category(data, model, tmp_spent, tmp_budget, startmonth, nbmonth, tmptype, tmponlyout);
		}

		// update column 0 title
		GtkTreeViewColumn *column = gtk_tree_view_get_column( GTK_TREE_VIEW(data->LV_report), 0);
		gtk_tree_view_column_set_title(column, budget_mode_label(tmpmode) );

		// Re-attach model to view
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_report), model);
		g_object_unref(model);

		gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_report));
		gtk_tree_view_columns_autosize(GTK_TREE_VIEW(data->LV_report));

		repbudget_update_total(widget, NULL);
		repbudget_update_chart(widget, NULL);

		DB( g_print("\nunblock handlers\n") );
		g_signal_handlers_unblock_by_func (G_OBJECT (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report))), G_CALLBACK (repbudget_selection), NULL);
		g_signal_handlers_unblock_by_func (G_OBJECT (data->LV_detail), G_CALLBACK (repbudget_detail_onRowActivated), NULL);

	}

	//DB( g_print(" inserting %i, %f %f\n", i, total_expense, total_income) );

	// free our memory
	g_free(tmp_spent);
	g_free(tmp_budget);

}


/*
** update sensitivity
*/
static void repbudget_sensitive(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gboolean visible, sensitive;
gint tmppage, tmpmode, tmptoday, count;

	DB( g_print("\n[repbudget] sensitive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	tmppage  = gtk_notebook_get_current_page(GTK_NOTEBOOK(data->GR_result));
	tmpmode  = hbtk_switcher_get_active(HBTK_SWITCHER(data->RA_mode));
	tmptoday = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_untiltoday));

	visible = (tmppage == 0) ? TRUE : FALSE;
	hb_widget_visible (data->BT_detail, visible);
	hb_widget_visible (data->BT_export, visible);

	//5.7
	visible = (tmpmode == 0) ? TRUE : FALSE;
	//hb_widget_visible (data->BT_print, !visible);
	hb_widget_visible (data->GR_listbar, visible);

	//5.9
	sensitive = tmptoday ? FALSE : TRUE;
	gtk_widget_set_sensitive(data->LB_maxdate, sensitive);
	gtk_widget_set_sensitive(data->SB_maxdate, sensitive);

	count = gtk_tree_model_iter_n_children(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail)), NULL);

	sensitive = ((count > 0) && data->detail) ? TRUE : FALSE;
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "detclip")), sensitive);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(g_action_map_lookup_action (G_ACTION_MAP (data->actions), "detcsv")), sensitive);

}


static void repbudget_toggle(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;
gboolean minor;

	DB( g_print("\n[repbudget] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	repbudget_update_total(widget, NULL);

	//hbfile_update(data->LV_acc, (gpointer)4);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_report));

	minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));
	ui_chart_progress_show_minor(GTK_CHARTPROGRESS(data->RE_progress), minor);

}

static void repbudget_cb_expand_all(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[repdist] expand all (data=%p)\n", data) );

	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_report));
}


static void repbudget_cb_collapse_all(GtkWidget *widget, gpointer user_data)
{
struct repbudget_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[repdist] collapse all (data=%p)\n", data) );

	gtk_tree_view_collapse_all(GTK_TREE_VIEW(data->LV_report));
}


static void repbudget_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
GtkTreeModel *model;
GtkTreeIter iter;
gint key = -1;

	DB( g_print("\n[repbudget] selection\n") );

	if (gtk_tree_selection_get_selected(treeselection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_BUDGET_KEY, &key, -1);
	}

	DB( g_print(" - active is %d\n", key) );

	repbudget_detail(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(key));
	repbudget_sensitive(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static const GActionEntry win_actions[] = {
	{ "resclip"		, repbudget_export_result_clipboard, NULL, NULL, NULL, {0,0,0} },
	{ "rescsv"		, repbudget_export_result_csv, NULL, NULL, NULL, {0,0,0} },
	{ "detclip"		, repbudget_export_detail_clipboard, NULL, NULL, NULL, {0,0,0} },
	{ "detcsv"		, repbudget_export_detail_csv, NULL, NULL, NULL, {0,0,0} },
//	{ "actioname"	, not_implemented, NULL, NULL, NULL, {0,0,0} },
};


static GtkWidget *
repbudget_toolbar_create(struct repbudget_data *data)
{
GtkWidget *toolbar, *button;

	toolbar = gtk_toolbar_new();

	button = (GtkWidget *)gtk_radio_tool_button_new(NULL);
	data->BT_list = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_LIST, "label", _("List"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as list"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	button = (GtkWidget *)gtk_radio_tool_button_new_from_widget(GTK_RADIO_TOOL_BUTTON(button));
	data->BT_progress = button;
	g_object_set (button, "icon-name", ICONNAME_HB_VIEW_PROGRESS, "label", _("Stack"), NULL);
	gtk_widget_set_tooltip_text(button, _("View results as stack bars"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), gtk_separator_tool_item_new(), -1);

	button = gtk_widget_new(GTK_TYPE_TOGGLE_TOOL_BUTTON,
		"icon-name", ICONNAME_HB_OPE_SHOW,
	    "label", _("Detail"),
		"tooltip-text", _("Toggle detail"),
	    "active", PREFS->budg_showdetail,
		NULL);
	data->BT_detail = button;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(button), -1);

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
static void repbudget_filter_setup(struct repbudget_data *data)
{
	DB( g_print("\n[repbudget] reset filter\n") );

	filter_reset(data->filter);
	filter_preset_daterange_set(data->filter, PREFS->date_range_rep, 0);
	/* 3.4 : make int transfer out of stats */
	filter_preset_type_set(data->filter, FLT_TYPE_INTXFER, FLT_EXCLUDE);

	if( PREFS->date_range_rep == FLT_RANGE_MISC_ALLDATE )
	{
		data->filter->mindate = hb_date_get_jbound(data->filter->mindate, HB_DATE_BOUND_FIRST);
		data->filter->maxdate = hb_date_get_jbound(data->filter->maxdate, HB_DATE_BOUND_LAST);
	}

	//g_signal_handler_block(data->PO_mindate, data->handler_id[HID_REPBUDGET_MINDATE]);
	//g_signal_handler_block(data->PO_maxdate, data->handler_id[HID_REPBUDGET_MAXDATE]);

	//gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_mindate), data->filter->mindate);
	//gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_maxdate), data->filter->maxdate);

	//5.7
	hbtk_monthyear_set(GTK_SPIN_BUTTON(data->SB_mindate), data->filter->mindate);
	hbtk_monthyear_set(GTK_SPIN_BUTTON(data->SB_maxdate), data->filter->maxdate);

	//g_signal_handler_unblock(data->PO_mindate, data->handler_id[HID_REPBUDGET_MINDATE]);
	//g_signal_handler_unblock(data->PO_maxdate, data->handler_id[HID_REPBUDGET_MAXDATE]);

}


// setup default for our object/memory
static void repbudget_window_setup(struct repbudget_data *data)
{
	DB( g_print("\n[repbudget] setup\n") );

	DB( g_print(" init data\n") );

	repbudget_filter_setup(data);


	DB( g_print(" set widgets default\n") );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->BT_detail), PREFS->budg_showdetail);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor),GLOBALS->minor);

	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_report))), "minor", (gpointer)data->CM_minor);
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_detail))), "minor", (gpointer)data->CM_minor);


	DB( g_print(" connect widgets signals\n") );

	g_signal_connect (data->CM_untiltoday, "toggled", G_CALLBACK (repbudget_cb_date_monthyear), NULL);

	g_signal_connect (data->CM_onlyout, "toggled", G_CALLBACK (repbudget_compute), NULL);
	g_signal_connect (data->CM_minor, "toggled", G_CALLBACK (repbudget_toggle), NULL);

	g_signal_connect (G_OBJECT (data->BT_expand), "clicked", G_CALLBACK (repbudget_cb_expand_all), NULL);
	g_signal_connect (G_OBJECT (data->BT_collapse), "clicked", G_CALLBACK (repbudget_cb_collapse_all), NULL);

	data->handler_id[HID_REPBUDGET_RANGE] = g_signal_connect (data->CY_range, "changed", G_CALLBACK (repbudget_cb_date_range), NULL);

    //g_signal_connect (data->CY_for , "changed", G_CALLBACK (repbudget_compute), (gpointer)data);
    g_signal_connect (data->RA_mode, "changed", G_CALLBACK (repbudget_compute), (gpointer)data);
    g_signal_connect (data->CY_type, "changed", G_CALLBACK (repbudget_compute), (gpointer)data);

    //data->handler_id[HID_REPBUDGET_MINDATE] = g_signal_connect (data->PO_mindate, "changed", G_CALLBACK (repbudget_date_change), (gpointer)data);
    //data->handler_id[HID_REPBUDGET_MAXDATE] = g_signal_connect (data->PO_maxdate, "changed", G_CALLBACK (repbudget_date_change), (gpointer)data);

    data->handler_id[HID_REPBUDGET_MINMONTHYEAR] = g_signal_connect (data->SB_mindate, "value-changed", G_CALLBACK (repbudget_cb_date_monthyear), NULL);
    data->handler_id[HID_REPBUDGET_MAXMONTHYEAR] = g_signal_connect (data->SB_maxdate, "value-changed", G_CALLBACK (repbudget_cb_date_monthyear), NULL);

	g_signal_connect (G_OBJECT (data->BT_list), "clicked", G_CALLBACK (repbudget_action_viewlist), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_progress), "clicked", G_CALLBACK (repbudget_action_viewstack), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_detail), "clicked", G_CALLBACK (repbudget_toggle_detail), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_refresh), "clicked", G_CALLBACK (repbudget_compute), (gpointer)data);
	g_signal_connect (G_OBJECT (data->BT_print), "clicked", G_CALLBACK (repbudget_action_print), (gpointer)data);
	//export is a menu


	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_report)), "changed", G_CALLBACK (repbudget_selection), NULL);

	g_signal_connect (GTK_TREE_VIEW(data->LV_detail), "row-activated", G_CALLBACK (repbudget_detail_onRowActivated), NULL);

}


static gboolean repbudget_window_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbudget_data *data;
guint32 rid;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[repbudget] window mapped\n") );

	//setup, init and show window
	repbudget_window_setup(data);

	//5.9 ensure we will trigger a valid id
	rid = FLT_RANGE_MISC_ALLDATE;
	if( rid == FLT_RANGE_LAST_MONTH || rid == FLT_RANGE_THIS_MONTH || rid == FLT_RANGE_NEXT_MONTH
	 || rid == FLT_RANGE_LAST_QUARTER || rid == FLT_RANGE_THIS_QUARTER || rid == FLT_RANGE_NEXT_QUARTER
	 || rid == FLT_RANGE_LAST_YEAR || rid == FLT_RANGE_THIS_YEAR || rid == FLT_RANGE_NEXT_YEAR
	 || rid == FLT_RANGE_LAST_12MONTHS || rid == FLT_RANGE_LAST_6MONTHS || rid == FLT_RANGE_MISC_ALLDATE
	 || rid == FLT_RANGE_TODATE_YEAR || rid == FLT_RANGE_TODATE_MONTH || rid == FLT_RANGE_TODATE_ALL
	)
	{
		rid = PREFS->date_range_rep;
	}

	hbtk_combo_box_set_active_id(GTK_COMBO_BOX(data->CY_range), rid);

	//hb_window_set_busy(GTK_WINDOW(data->window), FALSE);

	//check for any account included into the budget or warn
	{
	guint count =0;
	GList *lacc, *list;

		lacc = list = g_hash_table_get_values(GLOBALS->h_acc);

		while (list != NULL)
		{
		Account *acc;
			acc = list->data;
			//#1674045 ony rely on nosummary
			//if((acc->flags & (AF_CLOSED|AF_NOREPORT))) goto next1;
			if((acc->flags & (AF_NOREPORT))) goto next1;
			if(!(acc->flags & AF_NOBUDGET))
				count++;
		next1:
			list = g_list_next(list);
		}
		g_list_free(lacc);

		if(count <= 0)
		{
			ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_WARNING,
				_("No account is defined to be part of the budget."),
				_("You should include some accounts from the account dialog.")
				);
		}



	}

	data->mapped_done = TRUE;

	return FALSE;
}


static gboolean repbudget_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbudget_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[repbudget] start dispose\n") );

	if(data->txn_queue != NULL)
		g_queue_free (data->txn_queue);

	da_flt_free(data->filter);

	g_free(data);

	//store position and size
	wg = &PREFS->bud_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);

	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d\n", wg->l, wg->t, wg->w, wg->h) );

	//enable define windows
	GLOBALS->define_off--;
	ui_wallet_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	//unref window to our open window list
	GLOBALS->openwindows = g_slist_remove(GLOBALS->openwindows, widget);

	DB( g_print("\n[repbudget] end dispose\n") );

	return FALSE;
}


//allocate our object/memory
static void repbudget_window_acquire(struct repbudget_data *data)
{
	DB( g_print("\n[repbudget] acquire\n") );

	data->txn_queue = g_queue_new ();
	data->filter = da_flt_malloc();
	data->detail = PREFS->budg_showdetail;
	data->legend = 1;
}


// the window creation
GtkWidget *repbudget_window_new(void)
{
struct repbudget_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainbox, *hbox, *vbox, *tbar, *bbox, *notebook, *treeview, *vpaned, *scrollwin;
GtkWidget *label, *widget, *table, *entry;
gint row;

	DB( g_print("\n[repbudget] new\n") );


	data = g_malloc0(sizeof(struct repbudget_data));
	if(!data) return NULL;

	repbudget_window_acquire(data);

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

	gtk_window_set_title (GTK_WINDOW (window), _("Budget report"));

	//window contents
	mainbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	hb_widget_set_margin(GTK_WIDGET(mainbox), SPACING_SMALL);
	gtk_window_set_child(GTK_WINDOW(window), mainbox);

	//control part
	table = gtk_grid_new ();
    gtk_box_prepend (GTK_BOX (mainbox), table);

	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);

	row = 0;
	label = make_label_group(_("Display"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	label = make_label_widget(_("Mode:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_REPORT_MODE, TRUE);
	data->RA_mode = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_Type:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = hbtk_combo_box_new_with_data(label, CYA_KIND);
	data->CY_type = widget;
	gtk_grid_attach (GTK_GRID (table), data->CY_type, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Only out of budget"));
	data->CM_onlyout = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);


	//-- filter
	row++;
	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (table), widget, 0, row, 3, 1);

	row++;
	label = make_label_group(_("Filter"));
	gtk_grid_attach (GTK_GRID (table), label, 0, row, 3, 1);

	row++;
	label = make_label_group(_("Date"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 2, 1);

	row++;
	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	data->CY_range = make_daterange(label, DATE_RANGE_FLAG_BUDGET_MODE | DATE_RANGE_FLAG_CUSTOM_DISABLE);
	gtk_grid_attach (GTK_GRID (table), data->CY_range, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_From:"));
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = make_monthyear(NULL);
	data->SB_mindate = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget(_("_To:"));
	data->LB_maxdate = label;
	gtk_grid_attach (GTK_GRID (table), label, 1, row, 1, 1);
	widget = make_monthyear(NULL);
	data->SB_maxdate = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("U_ntil today"));
	data->CM_untiltoday = widget;
	gtk_grid_attach (GTK_GRID (table), widget, 2, row, 1, 1);


	//part: info + report
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_start (vbox, SPACING_SMALL);
    hbtk_box_prepend (GTK_BOX (mainbox), vbox);

	//toolbar
	widget = repbudget_toolbar_create(data);
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


	entry = gtk_label_new(NULL);
	data->TX_total[2] = entry;
	gtk_box_append (GTK_BOX (hbox), entry);
	label = gtk_label_new(_("Result:"));
	gtk_box_append (GTK_BOX (hbox), label);

	entry = gtk_label_new(NULL);
	data->TX_total[1] = entry;
	gtk_box_append (GTK_BOX (hbox), entry);
	label = gtk_label_new(_("Budget:"));
	gtk_box_append (GTK_BOX (hbox), label);

	entry = gtk_label_new(NULL);
	data->TX_total[0] = entry;
	gtk_box_append (GTK_BOX (hbox), entry);
	label = gtk_label_new(_("Spent:"));
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
	treeview = lst_repbud_create();
	data->LV_report = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
    gtk_paned_pack1 (GTK_PANED(vpaned), scrollwin, TRUE, TRUE);

	//5.8 moved collapse/expand here
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	data->GR_listbar = tbar;
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_prepend (GTK_BOX (vbox), tbar);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (GTK_BOX (tbar), bbox);

		widget = make_image_button(ICONNAME_HB_BUTTON_EXPAND, _("Expand all"));
		data->BT_expand = widget;
		gtk_box_prepend (GTK_BOX (bbox), widget);

		widget = make_image_button(ICONNAME_HB_BUTTON_COLLAPSE, _("Collapse all"));
		data->BT_collapse = widget;
		gtk_box_prepend (GTK_BOX (bbox), widget);

	//detail
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	data->GR_detail = scrollwin;
	treeview = create_list_transaction(LIST_TXN_TYPE_DETAIL, PREFS->lst_det_columns);
	data->LV_detail = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);

    gtk_paned_pack2 (GTK_PANED(vpaned), scrollwin, TRUE, TRUE);

	list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);

	//page: 2d bar
	//widget = gtk_chart_new(CHART_TYPE_COL);
	widget = ui_chart_progress_new();
	data->RE_progress = widget;
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, NULL);


	// connect dialog signals
    g_signal_connect (window, "delete-event", G_CALLBACK (repbudget_window_dispose), (gpointer)data);
	g_signal_connect (window, "map-event"   , G_CALLBACK (repbudget_window_mapped), NULL);

	// setup, init and show window
	wg = &PREFS->bud_wg;
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
	repbudget_sensitive(window, NULL);
	repbudget_update_detail(window, NULL);

	return(window);
}



