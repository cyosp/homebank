/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2022 Maxime DOYEN
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

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;



GString *lst_rep_total_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
const gchar *format;

	DB( g_print("\n[list_rep_total] to string\n") );

	node = g_string_new(NULL);

	// header
	format = (clipboard == TRUE) ? "%s\t%s\t%s\t%s\n" : "%s;%s;%s;%s\n";
	g_string_append_printf(node, format, (title == NULL) ? _("Result") : title, _("Expense"), _("Income"), _("Total"));

	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	gchar *name;
	gdouble exp, inc, bal;

		gtk_tree_model_get (model, &iter,
			//LST_REPDIST_KEY, i,
			LST_REPDIST_NAME   , &name,
			LST_REPDIST_EXPENSE, &exp,
			LST_REPDIST_INCOME , &inc,
			LST_REPDIST_TOTAL, &bal,
			-1);

		format = (clipboard == TRUE) ? "%s\t%.2f\t%.2f\t%.2f\n" : "%s;%.2f;%.2f;%.2f\n";
		g_string_append_printf(node, format, name, exp, inc, bal);

		//leak
		g_free(name);
		
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	//DB( g_print("text is:\n%s", node->str) );

	return node;
}



static void 
lst_rep_total_name_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gint pos;
gchar *text = NULL;
gint weight = PANGO_WEIGHT_NORMAL;

	gtk_tree_model_get(model, iter, 
		LST_REPDIST_POS, &pos,
		LST_REPDIST_NAME, &text,
		-1);

	if( pos == LST_REPDIST_POS_TOTAL )
	{
		weight = PANGO_WEIGHT_BOLD;
	}

	g_object_set(renderer, "weight", weight, "text", text, NULL);

	g_free(text);
}



static void lst_rep_total_rate_cell_data_function (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gdouble  tmp;
gchar   buf[128];

	gtk_tree_model_get(model, iter, GPOINTER_TO_INT(user_data), &tmp, -1);

	if(tmp != 0.0)
	{
		g_snprintf(buf, sizeof(buf), "%.2f %%", tmp);
		g_object_set(renderer, "text", buf, NULL);
	}
	else
		g_object_set(renderer, "text", "", NULL);

}


static void lst_rep_total_amount_cell_data_function (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gdouble  value;
gchar *color;
gint pos;
gint weight = PANGO_WEIGHT_NORMAL;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

	gtk_tree_model_get(model, iter,
		LST_REPDIST_POS, &pos,
		GPOINTER_TO_INT(user_data), &value,
		-1);

	if( value )
	{
		hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, GLOBALS->kcur, GLOBALS->minor);

		color = get_normal_color_amount(value);

		if( pos == LST_REPDIST_POS_TOTAL )
		{
			weight = PANGO_WEIGHT_BOLD;
		}

		g_object_set(renderer,
			"foreground",  color,
			"weight", weight,
			"text", buf,
			NULL);	}
	else
	{
		g_object_set(renderer, "text", "", NULL);
	}
}


static GtkTreeViewColumn *lst_rep_total_amount_column(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_total_amount_cell_data_function, GINT_TO_POINTER(id), NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	//#1933164
	gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}

static GtkTreeViewColumn *lst_rep_total_rate_column(gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "%");
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, "yalign", 1.0, "scale", 0.8, "scale-set", TRUE, NULL);
	
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", id);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_total_rate_cell_data_function, GINT_TO_POINTER(id), NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	//gtk_tree_view_column_set_sort_column_id (column, id);

	//gtk_tree_view_column_set_visible(column, FALSE);

	return column;
}


static gint lst_rep_total_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
gint pos1, pos2;
gdouble val1, val2;

	//#1933164
	gint csid;
	GtkSortType cso;

	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &csid, &cso);
	DB( g_print(" csid=%d cso=%s\n", csid, cso == GTK_SORT_ASCENDING ? "asc" : "desc") );

	gtk_tree_model_get(model, a,
		LST_REPDIST_POS, &pos1,
		sortcol, &val1,
		-1);
	gtk_tree_model_get(model, b,
		LST_REPDIST_POS, &pos2,
		sortcol, &val2,
		-1);

	//#1933164 should return
	// > 0 if a sorts before b 
	// = 0 if a sorts with b
	// < 0 if a sorts after b

	//total always at bottom
	if( pos1 == LST_REPDIST_POS_TOTAL )
	{
		retval = cso == GTK_SORT_ASCENDING ? 1 : -1;
		DB( g_print(" sort p1=%d ? p2=%d = %d\n", pos1, pos2, retval) );
	}
	else
	{
		if( pos2 == LST_REPDIST_POS_TOTAL )
		{
			retval = cso == GTK_SORT_ASCENDING ? -1 : 1;
			DB( g_print(" sort p1=%d ? p2=%d = %d\n", pos1, pos2, retval) )
		}
		else
		{
			switch(sortcol)
			{
				case LST_REPDIST_POS:
					retval = pos1 - pos2;
					//DB( g_print(" sort %3d = %3d :: %d\n", pos1, pos2, retval) );
					break;
				default:
					retval = (ABS(val1) - ABS(val2)) > 0 ? -1 : 1;
					//DB( g_print(" sort %.2f = %.2f :: %d\n", val1, val2, retval) );
					break;
			}
		}
	}


	return retval;
}


static gboolean 
lst_rep_total_selectionfunc(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
GtkTreeIter iter;
gint pos;

	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter,
			LST_REPDIST_POS, &pos,
			-1);

		if( pos == LST_REPDIST_POS_TOTAL )
			return FALSE;
	}

	return TRUE;
}



/*
** create our statistic list
*/
GtkWidget *lst_rep_total_create(void)
{
GtkListStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	DB( g_print("\n[list_rep_total] create\n") );

	/* create list store */
	store = gtk_list_store_new(
	  	NUM_LST_REPDIST,
		G_TYPE_INT,		//POS keep for compatibility with chart
	    G_TYPE_INT,		//KEY
		G_TYPE_STRING,	//ROWLABEL
		G_TYPE_DOUBLE,	//EXP
		G_TYPE_DOUBLE,	//EXPRATE
		G_TYPE_DOUBLE,	//INC
		G_TYPE_DOUBLE,	//INCRATE
		G_TYPE_DOUBLE,	//TOT
		G_TYPE_DOUBLE	///TOTRATE
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
	gtk_tree_view_column_set_title(column, _("Result"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_total_name_cell_data_function, NULL, NULL);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_REPDIST_NAME);
	//#1933164
	gtk_tree_view_column_set_sort_column_id (column, LST_REPDIST_POS);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Expense */
	column = lst_rep_total_amount_column(_("Expense"), LST_REPDIST_EXPENSE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);
	column = lst_rep_total_rate_column(LST_REPDIST_EXPRATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Income */
	column = lst_rep_total_amount_column(_("Income"), LST_REPDIST_INCOME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);
	column = lst_rep_total_rate_column(LST_REPDIST_INCRATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* column: Total */
	column = lst_rep_total_amount_column(_("Total"), LST_REPDIST_TOTAL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);
	column = lst_rep_total_rate_column(LST_REPDIST_TOTRATE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

  /* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* prevent selection of total */
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), lst_rep_total_selectionfunc, NULL, NULL);

	/* sort */
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPDIST_POS    , lst_rep_total_compare_func, GINT_TO_POINTER(LST_REPDIST_POS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPDIST_EXPENSE, lst_rep_total_compare_func, GINT_TO_POINTER(LST_REPDIST_EXPENSE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPDIST_INCOME , lst_rep_total_compare_func, GINT_TO_POINTER(LST_REPDIST_INCOME), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPDIST_TOTAL  , lst_rep_total_compare_func, GINT_TO_POINTER(LST_REPDIST_TOTAL), NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_REPDIST_POS, GTK_SORT_ASCENDING);

	return(view);
}


/*
** ============================================================================
*/


GString *lst_rep_time_to_string(GtkTreeView *treeview, gchar *title, gboolean clipboard)
{
GString *node;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean valid;
guint32 nbcols, r, i;
gchar sep;

	DB( g_print("\n[list_rep_time] to string\n") );

	node = g_string_new(NULL);

	sep = (clipboard == TRUE) ? '\t' : ';';

	// header
	nbcols = gtk_tree_view_get_n_columns (treeview);
	for( i=0 ; i < nbcols ; i++ )
	{
	GtkTreeViewColumn *column = gtk_tree_view_get_column (treeview, i);
	
		if( GTK_IS_TREE_VIEW_COLUMN(column) )
		{
			g_string_append(node, gtk_tree_view_column_get_title (column));
			if( i < nbcols-1 )
				g_string_append_c(node, sep);
		}
	}
	g_string_append_c(node, '\n');

	// data rows
	r = 0;
	nbcols -= 2; // because -label -1 (as we start a 0)
	model = gtk_tree_view_get_model(treeview);
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	gchar *name;
	DataRow *dr;

		gtk_tree_model_get (model, &iter,
			LST_REPDIST2_LABEL, &name,
			LST_REPDIST2_ROW, &dr,
			-1);

		g_string_append(node, name);
		g_string_append_c(node, sep);

		DB( g_print(" %2d '%s' %p\n", r, name, dr) );

		// iterate row cells
		for( i=0 ; i < nbcols ; i++ )
		{
		gdouble value = da_datarow_get_cell_sum(dr, i);

			DB( g_print(" %2d %.2f\n", i, value) );
		
			g_string_append_printf(node, "%.2f", value);
			if( i < nbcols )
				g_string_append_c(node, sep);
		}

		g_string_append_c(node, '\n');

		//leak
		g_free(name);
		
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
		LST_REPDIST2_LABEL, &label,
		-1);


	gtk_tooltip_set_markup (tooltip, label);

	gtk_tree_view_set_tooltip_row (tree_view, tooltip, path);

	gtk_tree_path_free (path);

	return TRUE;
}





static gint lst_rep_time_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
//gint sortcol = GPOINTER_TO_INT(userdata);
gint retval = 0;
DataRow *dr1, *dr2;
	
	
	gtk_tree_model_get(model, a,
		LST_REPDIST2_ROW, &dr1,
		-1);
	gtk_tree_model_get(model, b,
		LST_REPDIST2_ROW, &dr2,
		-1);

	//total always at bottom
	if( dr1->pos == LST_REPDIST_POS_TOTAL )
	{
		retval = -1;
	}
	else
	{
		if( dr2->pos == LST_REPDIST_POS_TOTAL )
		{
			retval = 1;
		}
		else
		{
			retval = dr2->pos - dr1->pos;
		}
	}
	
	//DB( g_print(" sort %d=%d or %.2f=%.2f :: %d\n", pos1,pos2, val1, val2, ret) );

    return retval;
  }

static void 
lst_rep_time_name_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gint pos;
gchar *text = NULL;
gint weight = PANGO_WEIGHT_NORMAL;

	gtk_tree_model_get(model, iter, 
		LST_REPDIST2_POS, &pos,
		LST_REPDIST2_LABEL, &text,
		-1);

	if( pos == LST_REPDIST_POS_TOTAL )
	{
		weight = PANGO_WEIGHT_BOLD;
	}

	g_object_set(renderer, "weight", weight, "text", text, NULL);

	g_free(text);
}


static void lst_rep_time_amount_cell_data_function (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
DataRow *dr;
gchar *color;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gint colid = GPOINTER_TO_INT(user_data);
gint weight = PANGO_WEIGHT_NORMAL;
gint pos;


	gtk_tree_model_get(model, iter, 
		LST_REPDIST2_POS, &pos,
		LST_REPDIST2_ROW, &dr,
	    -1);
	
	if( dr != NULL )
	{
	gdouble value = dr->expense[colid]+dr->income[colid];

		if( value )
		{
			hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, GLOBALS->kcur, GLOBALS->minor);
			color = get_normal_color_amount(value);

			if( pos == LST_REPDIST_POS_TOTAL )
			{
				weight = PANGO_WEIGHT_BOLD;
			}

			g_object_set(renderer,
				"foreground",  color,
				"weight", weight,
				"text", buf,
				NULL);
			return;
		}
	}

	g_object_set(renderer, "text", "", NULL);
	
}


static GtkTreeViewColumn *lst_rep_time_column_create_amount(gchar *name, gint id)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, name);
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_time_amount_cell_data_function, GINT_TO_POINTER(id), NULL);
	gtk_tree_view_column_set_alignment (column, 0.5);
	//gtk_tree_view_column_set_sort_column_id (column, id);
	return column;
}


static gboolean 
lst_rep_time_selectionfunc(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path, gboolean path_currently_selected, gpointer data)
{
GtkTreeIter iter;
gint pos;

	if(gtk_tree_model_get_iter(model, &iter, path))
	{
		gtk_tree_model_get(model, &iter,
			LST_REPDIST2_POS, &pos,
			-1);

		if( pos == LST_REPDIST_POS_TOTAL )
			return FALSE;
	}

	return TRUE;
}



/* test new listview */
void lst_rep_time_renewcol(GtkTreeView *treeview, guint32 nbintvl, guint32 jfrom, gint intvl, gboolean avg)
{
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;
GList *columns, *list;
guint i;

	DB( g_print("\n[list_rep_time] renewcol\n") );

	/* remove all columns */
	columns = gtk_tree_view_get_columns(GTK_TREE_VIEW(treeview));

	//g_printf(" removing %d columns\n", g_list_length (columns));

	list = g_list_first(columns);
	while(list != NULL)
	{
	column = list->data;

		//g_printf(" removing %p\n", column);
		if(column)
			gtk_tree_view_remove_column(treeview, column);

		list = g_list_next(list);
	}
	g_list_free(columns);
	
	/* column: Name */
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
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_time_name_cell_data_function, NULL, NULL);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_REPDIST2_LABEL);
	//gtk_tree_view_column_set_sort_column_id (column, LST_REPTIME_NAME);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	/* column: Amount */
	for(i=0;i<nbintvl;i++)
	{
	gchar intvlname[64];		

		report_interval_snprint_name(intvlname, sizeof(intvlname)-1, intvl, jfrom, i);

		column = lst_rep_time_column_create_amount(intvlname, i);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	}

	if( avg == TRUE )
	{
		column = lst_rep_time_column_create_amount(_("Average"), i++);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	}

	column = lst_rep_time_column_create_amount(_("Total"), i++);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	
  /* column last: empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
}


/*
GtkWidget *lst_rep_time_createtype(GtkListStore *store)
{
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (view), PREFS->grid_lines);

	// column: Name
	column = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(column, _("Acc/Cat/Pay"));
	//gtk_tree_view_column_set_title(column, "ItemsType (todo)");
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, lst_rep_time_name_cell_data_function, NULL, NULL);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", LST_REPDIST2_LABEL);
	//gtk_tree_view_column_set_sort_column_id (column, LST_REPTIME_NAME);
	//gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);


	// prevent selection of total
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), lst_rep_time_selectionfunc, NULL, NULL);


	return view;
}*/


GtkWidget *lst_rep_time_create(void)
{
GtkTreeStore *store;
GtkWidget *view;

	DB( g_print("\n[list_rep_time] create\n") );

	/* create list store */
	store = gtk_tree_store_new(
	  	NUM_LST_REPDIST2,
		G_TYPE_INT,		//POS
	   	G_TYPE_INT,		//KEY
		G_TYPE_STRING,	//ROWLABEL
		G_TYPE_POINTER	//ROWDATA	(pointer to DataRow)
		);

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (view), PREFS->grid_lines);

	/* prevent selection of total */
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), lst_rep_time_selectionfunc, NULL, NULL);

	/* sort */
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPDIST2_POS    , lst_rep_time_compare_func, GINT_TO_POINTER(LST_REPDIST2_POS), NULL);
	//gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_REPTIME_AMOUNT, ui_list_reptime_compare_func, GINT_TO_POINTER(LST_REPTIME_AMOUNT), NULL);

	gtk_widget_set_has_tooltip (GTK_WIDGET (view), TRUE);

	g_signal_connect (view, "query-tooltip",
		            G_CALLBACK (lst_rep_time_cb_tooltip_query), NULL);

	
	return(view);
}


