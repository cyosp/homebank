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

#include "list-report.h"

/****************************************************************************/
/* Debug macros																*/
/****************************************************************************/
#define MYDEBUG 0


#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

// our global datas
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;


static void lst_report_to_string_row(GString *node, gint src, gboolean clipboard, GtkTreeModel *model, GtkTreeIter *iter)
{
const gchar *format;
guint32 key;
gchar *name;
gdouble exp, inc, bal;

	gtk_tree_model_get (model, iter,
		LST_REPORT_KEY    , &key,
		LST_REPORT_LABEL   , &name,
		LST_REPORT_EXPENSE, &exp,
		LST_REPORT_INCOME , &inc,
		LST_REPORT_TOTAL, &bal,
		-1);

	//#2033298 we get fullname for export
	if( src == REPORT_GRPBY_CATEGORY )
	{
	Category *catitem = da_cat_get(key);
		if( catitem != NULL )
		{
			g_free(name);
			name = g_strdup(catitem->fullname);
		}
	}

	format = (clipboard == TRUE) ? "%s\t%.2f\t%.2f\t%.2f\n" : "%s;%.2f;%.2f;%.2f\n";
	g_string_append_printf(node, format, name, exp, inc, bal);

	//leak
	g_free(name);
}


GString *lst_report_to_string(GtkTreeView *treeview, gint src, gchar *title, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter, child;
gboolean valid;
const gchar *format;

	DB( g_print("\n[list-report] to string\n") );

	node = g_string_new(NULL);

	// header
	format = (clipboard == TRUE) ? "%s\t%s\t%s\t%s\n" : "%s;%s;%s;%s\n";
	g_string_append_printf(node, format, (title == NULL) ? _("Result") : title, _("Expense"), _("Income"), _("Total"));

	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		lst_report_to_string_row(node, src, clipboard, model, &iter);
		
		if( gtk_tree_model_iter_has_child(model, &iter) )
		{
			valid = gtk_tree_model_iter_children(model, &child, &iter);
			while (valid)
			{
				lst_report_to_string_row(node, src, clipboard, model, &child);
				
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
			}		
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}


static void 
lst_report_cell_data_func_label (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gchar *label, *overlabel;
gint pos;

	gtk_tree_model_get(model, iter, 
		LST_REPORT_POS, &pos,
		LST_REPORT_LABEL, &label,
		LST_REPORT_OVERLABEL, &overlabel,
		-1);

	if( overlabel != NULL )
	{
		g_object_set(renderer, 
			"weight", PANGO_WEIGHT_NORMAL,
			"markup", overlabel, 
			NULL);
	}
	else
	{
		g_object_set(renderer, 
			"weight", (pos == LST_REPORT_POS_TOTAL) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
			"text"  , label,
			NULL);
	}

	g_free(label);
	g_free(overlabel);
}


static void lst_report_cell_data_func_rate (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
struct lst_report_data *lst_data = NULL;
GtkWidget *widget;
gint pos, colid = GPOINTER_TO_INT(user_data);
gdouble amount, rate = 0.0;
gchar buf[16], *retval = "";

	widget = gtk_tree_view_column_get_tree_view(col);
	if( widget )
		lst_data = g_object_get_data(G_OBJECT(widget), "inst_data");

	if(lst_data != NULL)
	{
		gtk_tree_model_get(model, iter, 
			LST_REPORT_POS, &pos,
			colid, &amount,
			-1);
	
		//don't display total/total	
		if( pos != LST_REPORT_POS_TOTAL )
		{
			switch(colid)
			{
				case LST_REPORT_EXPENSE:
					rate = hb_rate(amount, lst_data->tot_exp);
					break;
				case LST_REPORT_INCOME:
					rate = hb_rate(amount, lst_data->tot_inc);
					break;
				case LST_REPORT_TOTAL:
					rate = hb_rate(amount, -lst_data->tot_exp + lst_data->tot_inc);
					break;		
			}

			if( !hb_amount_equal(rate, 0.0) )
			{
				g_snprintf(buf, sizeof(buf), "%.2f %%", rate);
				retval = buf;
			}
		}
	}

	g_object_set(renderer, "text", retval, NULL);
}


static void
lst_report_cell_data_func_amount (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gdouble  value;
gchar *color;
gint pos, colid = GPOINTER_TO_INT(user_data);
gint weight = PANGO_WEIGHT_NORMAL;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

	gtk_tree_model_get(model, iter,
		LST_REPORT_POS, &pos,
		GPOINTER_TO_INT(user_data), &value,
		-1);

	//#2026184
	value = hb_amount_round(value, 2);

	if( (value != 0.0) || (colid == LST_REPORT_TOTAL) )
	{
		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, GLOBALS->kcur, GLOBALS->minor);

		color = get_normal_color_amount(value);

		if( pos == LST_REPORT_POS_TOTAL )
		{
			weight = PANGO_WEIGHT_BOLD;
		}

		g_object_set(renderer,
			"foreground",  color,
			"weight", weight,
			"text", buf,
			NULL);
	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}
}


static GtkTreeViewColumn *lst_report_amount_column(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_report_cell_data_func_amount, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//#1933164
	gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}

static GtkTreeViewColumn *lst_report_rate_column(gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "%");
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, "yalign", 1.0, "scale", 0.8, "scale-set", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", id);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_report_cell_data_func_rate, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	//gtk_tree_view_column_set_sort_column_id (column, id);

	//gtk_tree_view_column_set_visible(column, FALSE);

	return column;
}


static gint lst_report_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
gint pos1, pos2;
gdouble val1, val2;

	//#1933164
	gint csid;
	GtkSortType cso;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &csid, &cso);
	//DB( g_print(" csid=%d cso=%s\n", csid, cso == GTK_SORT_ASCENDING ? "asc" : "desc") );

	gtk_tree_model_get(model, a,
		LST_REPORT_POS, &pos1,
		sortcol, &val1,
		-1);
	gtk_tree_model_get(model, b,
		LST_REPORT_POS, &pos2,
		sortcol, &val2,
		-1);

	//#1933164 should return
	// > 0 if a sorts before b 
	// = 0 if a sorts with b
	// < 0 if a sorts after b

	//total always at bottom
	if( pos1 == LST_REPORT_POS_TOTAL )
	{
		retval = cso == GTK_SORT_ASCENDING ? 1 : -1;
		//DB( g_print(" sort p1=%d ? p2=%d = %d\n", pos1, pos2, retval) );
	}
	else
	{
		if( pos2 == LST_REPORT_POS_TOTAL )
		{
			retval = cso == GTK_SORT_ASCENDING ? -1 : 1;
			//DB( g_print(" sort p1=%d ? p2=%d = %d\n", pos1, pos2, retval) )
		}
		else
		{
			switch(sortcol)
			{
				case LST_REPORT_POS:
					retval = pos1 - pos2;
					//DB( g_print(" sort %3d = %3d :: %d\n", pos1, pos2, retval) );
					break;
				case LST_REPORT_EXPENSE:
				case LST_REPORT_INCOME:
					retval = (ABS(val1) - ABS(val2)) > 0 ? -1 : 1;
					break;
				default: // should be LST_REPORT_TOTAL
					//#1956060 sort with sign (no abs), option is possible but complex
					//retval = (ABS(val1) - ABS(val2)) > 0 ? -1 : 1;
					retval = (val1 - val2) > 0 ? -1 : 1;
					//DB( g_print(" sort %.2f = %.2f :: %d\n", val1, val2, retval) );
					break;
			}
		}
	}

	return retval;
}


static gboolean 
lst_report_selectionfunc(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
GtkTreeIter iter;
gint pos;

	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter,
			LST_REPORT_POS, &pos,
			-1);

		if( pos == LST_REPORT_POS_TOTAL )
			return FALSE;
	}

	return TRUE;
}


static void lst_report_destroy( GtkWidget *widget, gpointer user_data )
{
struct lst_report_data *lst_data;

	lst_data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print ("\n[list-report] destroy event occurred\n") );

	DB( g_print(" - view=%p, inst_data=%p\n", widget, lst_data) );
	g_free(lst_data);
}


void lst_report_add_columns(GtkTreeView *treeview, GtkTreeModel *model)
{
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	// column: Name
	renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Result"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_report_cell_data_func_label, NULL, NULL);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_REPORT_LABEL);
	//#1933164
	gtk_tree_view_column_set_sort_column_id (column, LST_REPORT_POS);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column: Expense
	column = lst_report_amount_column(_("Expense"), LST_REPORT_EXPENSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	column = lst_report_rate_column(LST_REPORT_EXPENSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column: Income
	column = lst_report_amount_column(_("Income"), LST_REPORT_INCOME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	column = lst_report_rate_column(LST_REPORT_INCOME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column: Total
	column = lst_report_amount_column(_("Total"), LST_REPORT_TOTAL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	column = lst_report_rate_column(LST_REPORT_TOTAL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column last: empty
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


}


GtkTreeStore *lst_report_new(void)
{
GtkTreeStore *store;

	store = gtk_tree_store_new(
	  	NUM_LST_REPORT,
		G_TYPE_INT,		//POS keep for compatibility with chart
	    G_TYPE_INT,		//KEY
	    //G_TYPE_POINTER,	//ROW
		G_TYPE_STRING,	//ROWLABEL
		G_TYPE_DOUBLE,	//EXP
		G_TYPE_DOUBLE,	//INC
		G_TYPE_DOUBLE,	//TOT
		G_TYPE_STRING	//OVERRIDELABEL
		);

	return store;
}


/*
** create our statistic list
*/
GtkWidget *lst_report_create(void)
{
struct lst_report_data *lst_data;
GtkTreeStore *store;
GtkWidget *treeview;


	DB( g_print("\n[list-report] create\n") );

	lst_data = g_malloc0(sizeof(struct lst_report_data));
	if(!lst_data) 
		return NULL;

	// create list store
	store = lst_report_new();

	// treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	// store our window private data
	g_object_set_data(G_OBJECT(treeview), "inst_data", (gpointer)lst_data);
	DB( g_print(" - treeview=%p, inst_data=%p\n", treeview, lst_data) );

	// connect our dispose function
	g_signal_connect (treeview, "destroy", G_CALLBACK (lst_report_destroy), NULL);


	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);

	lst_report_add_columns(GTK_TREE_VIEW(treeview), GTK_TREE_MODEL(store));


	// prevent selection of total
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), lst_report_selectionfunc, NULL, NULL);

	// sort
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPORT_POS    , lst_report_compare_func, GINT_TO_POINTER(LST_REPORT_POS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPORT_EXPENSE, lst_report_compare_func, GINT_TO_POINTER(LST_REPORT_EXPENSE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPORT_INCOME , lst_report_compare_func, GINT_TO_POINTER(LST_REPORT_INCOME), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPORT_TOTAL  , lst_report_compare_func, GINT_TO_POINTER(LST_REPORT_TOTAL), NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_REPORT_POS, GTK_SORT_ASCENDING);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	return(treeview);
}


/*
** ============================================================================
*/


static void lst_rep_time_to_string_row(GString *node, gint src, gchar sep, GtkTreeModel *model, GtkTreeIter *iter)
{
guint i;
guint32 key;
gchar *name;
DataRow *dr;
gdouble value;

	gtk_tree_model_get (model, iter,
		LST_REPORT2_KEY,   &key,
		LST_REPORT2_LABEL, &name,
		LST_REPORT2_ROW,   &dr,
		-1);

	//#2033298 we get fullname for export
	if( src == REPORT_GRPBY_CATEGORY )
	{
	Category *catitem = da_cat_get(key);
		if( catitem != NULL )
		{
			g_free(name);
			name = g_strdup(catitem->fullname);
		}
	}

	g_string_append(node, name);
	g_string_append_c(node, sep);

	//iterate row cells
	for( i=0 ; i < dr->nbcols ; i++ )
	{
		value = da_datarow_get_cell_sum(dr, i);
		DB( g_print(" %2d %.2f\n", i, value) );
		g_string_append_printf(node, "%.2f", value);
		if( i < dr->nbcols )
			g_string_append_c(node, sep);
	}

	//average
	value = (dr->rowexp + dr->rowinc) / dr->nbcols;
	g_string_append_printf(node, "%.2f", value);
	g_string_append_c(node, sep);

	//total
	value = (dr->rowexp + dr->rowinc);
	g_string_append_printf(node, "%.2f", value);

	//newline
	g_string_append_c(node, '\n');

	//leak
	g_free(name);
}


GString *lst_rep_time_to_string(GtkTreeView *treeview, gint src, gchar *title, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter, child;
gboolean valid;
guint32 nbcols, r, i;
gchar sep;

	DB( g_print("\n[list_rep_time] to string\n") );

	node = g_string_new(NULL);

	sep = (clipboard == TRUE) ? '\t' : ';';

	// header (nbcols-1 for empty column)
	nbcols = gtk_tree_view_get_n_columns (treeview) - 1;
	for( i=0 ; i < nbcols ; i++ )
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column (treeview, i);
	
		if( GTK_IS_TREE_VIEW_COLUMN(column) )
		{
			g_string_append(node, gtk_tree_view_column_get_title (column));
			if( i < nbcols-1 )
			{
				g_string_append_c(node, sep);
			}
		}
	}
	g_string_append_c(node, '\n');

	// data rows
	r = 0;
	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		lst_rep_time_to_string_row(node, src, sep, model, &iter);
		if( gtk_tree_model_iter_has_child(model, &iter) )
		{
			valid = gtk_tree_model_iter_children(model, &child, &iter);
			while (valid)
			{
				lst_rep_time_to_string_row(node, src, sep, model, &child);
				valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
			}		
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
		r++;
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}


static gboolean
lst_rep_time_cb_tooltip_query (GtkWidget  *widget,
				    gint        x,
				    gint        y,
				    gboolean    keyboard_tip,
				    GtkTooltip *tooltip,
				    gpointer    data)
{
GtkTreeIter iter;
GtkTreePath *path;
GtkTreeModel *model;
GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
gchar *label = NULL;

	if (!gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (widget),
					  &x, &y,
					  keyboard_tip,
					  &model, &path, &iter))
	return FALSE;

	gtk_tree_model_get(model, &iter, 
		LST_REPORT2_LABEL, &label,
		-1);


	gtk_tooltip_set_text (tooltip, label);

	gtk_tree_view_set_tooltip_row (tree_view, tooltip, path);

	gtk_tree_path_free (path);

	return TRUE;
}




static void 
lst_rep_time_cell_data_func_label (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gchar *label, *overlabel;
gint pos;

	gtk_tree_model_get(model, iter, 
		LST_REPORT2_POS, &pos,
		LST_REPORT2_LABEL, &label,
		LST_REPORT2_OVERLABEL, &overlabel,
		-1);

	if( overlabel != NULL )
	{
		g_object_set(renderer, 
			"weight", PANGO_WEIGHT_NORMAL,
			"markup", overlabel, 
			NULL);
	}
	else
	{
		g_object_set(renderer, 
			"weight", (pos == LST_REPORT_POS_TOTAL) ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL,
			"text"  , label,
			NULL);
	}

	g_free(label);
	g_free(overlabel);
}


static void
lst_rep_time_cell_data_func_amount (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
DataRow *dr;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gint colid = GPOINTER_TO_INT(user_data);
gint pos;

	gtk_tree_model_get(model, iter, 
		LST_REPORT2_POS, &pos,
		LST_REPORT2_ROW, &dr,
	    -1);
	
	if( dr != NULL )
	{
	gdouble exp, inc, value;
	gboolean dodisplay = FALSE;
	gint weight = PANGO_WEIGHT_NORMAL;

		if( colid==LST_REP_COLID_AVERAGE || colid==LST_REP_COLID_TOTAL)
		{
			exp = dr->rowexp;
			inc = dr->rowinc;
		}
		else
		{
			exp = dr->colexp[colid];
			inc = dr->colinc[colid];
		}

		value = exp + inc;
		if( hb_amount_compare(value, 0.0) != 0 )
		{
			dodisplay = TRUE;
			if(colid==LST_REP_COLID_AVERAGE)
				value /= dr->nbcols;
		}
		else 
		{
			//#2091004 we have exact 0.0, do we force display ?
			if( pos == LST_REPORT_POS_TOTAL )
			{
				weight = PANGO_WEIGHT_BOLD;
				if( hb_amount_compare(exp, 0.0) != 0 ) // test exp is enough
					dodisplay = TRUE;
			}
		}

		if( dodisplay )
		{
			hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, GLOBALS->kcur, GLOBALS->minor);
			g_object_set(renderer,
				"foreground", get_normal_color_amount(value),
				"weight", weight,
				"text", buf,
				NULL);
			return;
		}

	}

	g_object_set(renderer, "text", "", NULL);
}


static GtkTreeViewColumn *lst_rep_time_column_create_amount(gchar *name, gint id, gboolean forecast)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);

	if( forecast )
	{
		g_object_set(renderer, 
			"style-set", TRUE,
			"style",	PANGO_STYLE_OBLIQUE,
		NULL);
	}

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_time_cell_data_func_amount, GINT_TO_POINTER(id), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}



static gint lst_rep_time_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
//gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
DataRow *dr1, *dr2;

	//#2034625
	gint csid;
	GtkSortType cso;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &csid, &cso);

	//#2034625 should return
	// > 0 if a sorts before b 
	// = 0 if a sorts with b
	// < 0 if a sorts after b
	
	gtk_tree_model_get(model, a,
		LST_REPORT2_ROW, &dr1,
		-1);
	gtk_tree_model_get(model, b,
		LST_REPORT2_ROW, &dr2,
		-1);

	//total always at bottom
	if( dr1->pos == LST_REPORT_POS_TOTAL )
	{
		retval = cso == GTK_SORT_ASCENDING ? 1 : -1;
	}
	else
	{
		if( dr2->pos == LST_REPORT_POS_TOTAL )
		{
			retval = cso == GTK_SORT_ASCENDING ? -1 : 1;
		}
		else
		{
		gdouble val1, val2;
		
			switch(csid)
			{
				case LST_REP_COLID_POS:
					retval = dr1->pos - dr2->pos;
					break;
				case LST_REP_COLID_AVERAGE:
					val1 = (dr1->rowexp + dr1->rowinc) / dr1->nbcols;
					val2 = (dr2->rowexp + dr2->rowinc) / dr2->nbcols;
					retval = hb_amount_compare(val1, val2);
					break;
				case LST_REP_COLID_TOTAL:
					val1 = (dr1->rowexp + dr1->rowinc);
					val2 = (dr2->rowexp + dr2->rowinc);
					retval = hb_amount_compare(val1, val2);
					break;
				default:
					val1 = dr1->colexp[csid] + dr1->colinc[csid];
					val2 = dr2->colexp[csid] + dr2->colinc[csid];
					retval = hb_amount_compare(val1, val2);
					break;					
			}
		}
	}

	//DB( g_print(" sort %d=%d or %.2f=%.2f :: %d\n", pos1,pos2, val1, val2, ret) );

	return retval;
}


static gboolean 
lst_rep_time_selectionfunc(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
GtkTreeIter iter;
gint pos;

	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter,
			LST_REPORT2_POS, &pos,
			-1);

		if( pos == LST_REPORT_POS_TOTAL )
			return FALSE;
	}

	return TRUE;
}


// test new listview
//TODO: optimise params
void lst_rep_time_renewcol(GtkTreeView *treeview, GtkTreeModel *model, DataTable *dt, gboolean avg)
{
GtkTreeViewColumn *column;
GtkCellRenderer *renderer;
GList *columns, *list;
guint i;

	DB( g_print("\n[list_rep_time] renewcol\n") );

	// remove all columns
	columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(treeview));
	i = 0;
	list = g_list_first(columns);
	while(list != NULL)
	{
		if( GTK_IS_TREE_VIEW_COLUMN(list->data) )
		{
			gtk_tree_view_remove_column(treeview, GTK_TREE_VIEW_COLUMN(list->data));
		}
		i++;
		list = g_list_next(list);
	}
	DB( g_print(" removed %d columns\n", i) );
	g_list_free(columns);

	//adding columns

	// column: Name
	column = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(column, _("Acc/Cat/Pay"));
	//gtk_tree_view_column_set_title(column, "ItemsType (todo)");
	renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);

	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_time_cell_data_func_label, NULL, NULL);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_REPORT2_LABEL);
	gtk_tree_view_column_set_sort_column_id (column, LST_REP_COLID_POS);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	DB( g_print (" create column id:%4d '<name>'\n", LST_REP_COLID_POS) );

	// column: Amount * x
	//TODO: useless with DataCol
	for(i=0;i<dt->nbcols;i++)
	{
	//gchar intvlname[64];
	//guint32 jdate = report_interval_snprint_name(intvlname, sizeof(intvlname)-1, intvl, jfrom, i);
	//gboolean forecast = (jdate > GLOBALS->today) ? TRUE : FALSE;
	DataCol *dtcol = report_data_get_col(dt, i);

		if ( dtcol )
		{
			column = lst_rep_time_column_create_amount(dtcol->label, i, (dtcol->flags & RF_FORECAST) );
			gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
			DB( g_print (" create column id:%4d '%s' forecast:%d\n", i, dtcol->label, (dtcol->flags & RF_FORECAST)) );
			gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(model), i, lst_rep_time_compare_func, NULL, NULL);
		}
	}

	column = lst_rep_time_column_create_amount(_("Average"), LST_REP_COLID_AVERAGE, FALSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	DB( g_print (" create column id:%4d '<average>'\n", LST_REP_COLID_AVERAGE) );
	//#2012576 keep column but hide it
	gtk_tree_view_column_set_visible(column, avg);

	column = lst_rep_time_column_create_amount(_("Total"), LST_REP_COLID_TOTAL, FALSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	DB( g_print (" create column id:%4d '<total>'\n", LST_REP_COLID_TOTAL) );
	
	// column last: empty
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
}


static void lst_rep_time_destroy( GtkWidget *widget, gpointer user_data )
{
struct lst_report_data *lst_data;

	lst_data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print ("\n[list_rep_time] destroy event occurred\n") );

	DB( g_print(" - view=%p, inst_data=%p\n", widget, lst_data) );
	g_free(lst_data);
}


GtkTreeStore *lst_rep_time_new(void)
{
GtkTreeStore *store;

	// create list store
	store = gtk_tree_store_new(
	  	NUM_LST_REPORT2,
		G_TYPE_INT,		//POS
	   	G_TYPE_INT,		//KEY
		G_TYPE_STRING,	//ROWLABEL
		G_TYPE_POINTER,	//ROWDATA	(pointer to DataRow)
		G_TYPE_STRING	//OVERRIDELABEL
		);

	return store;
}


GtkWidget *lst_rep_time_create(void)
{
struct lst_report_data *lst_data;
GtkTreeStore *store;
GtkWidget *treeview;

	DB( g_print("\n[list_rep_time] create\n") );

	lst_data = g_malloc0(sizeof(struct lst_report_data));
	if(!lst_data) 
		return NULL;

	// create list store
	store = lst_rep_time_new();

	//treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	// store our window private data
	g_object_set_data(G_OBJECT(treeview), "inst_data", (gpointer)lst_data);
	DB( g_print(" - treeview=%p, inst_data=%p\n", treeview, lst_data) );

	// connect our dispose function
	g_signal_connect (treeview, "destroy", G_CALLBACK (lst_rep_time_destroy), NULL);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);

	//prevent selection of total
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), lst_rep_time_selectionfunc, NULL, NULL);

	// sort 
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REP_COLID_POS, lst_rep_time_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REP_COLID_AVERAGE, lst_rep_time_compare_func, NULL, NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REP_COLID_TOTAL, lst_rep_time_compare_func, NULL, NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE(store), LST_REP_COLID_POS, GTK_SORT_ASCENDING);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	gtk_widget_set_has_tooltip (GTK_WIDGET (treeview), TRUE);

	g_signal_connect (treeview, "query-tooltip",
		            G_CALLBACK (lst_rep_time_cb_tooltip_query), NULL);

	
	return(treeview);
}

