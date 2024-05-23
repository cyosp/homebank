/*  HomeBank -- Free, easy, personal ruleing for everyone.
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

#include "ui-assign.h"
#include "hbtk-switcher.h"

#include "ui-category.h"
#include "ui-payee.h"
#include "ui-tag.h"


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


extern gchar *CYA_ASG_FIELD[];


static gchar *CYA_ASG_ACTION[] = {
	N_("Disabled"), 
	N_("If empty"), 
	N_("Overwrite"), 
	NULL
};


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void
ui_asg_listview_toggled_cb (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data)
{
  GtkTreeModel *model = (GtkTreeModel *)data;
  GtkTreeIter  iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, LST_DEFASG_TOGGLE, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  gtk_list_store_set (GTK_LIST_STORE (model), &iter, LST_DEFASG_TOGGLE, fixed, -1);

  /* clean up */
  gtk_tree_path_free (path);
}


static void ui_asg_listview_sort_force(GtkTreeSortable *sortable, gpointer user_data)
{
gint sort_column_id;
GtkSortType order;

	DB( g_print("\n[ui-asg-listview] sort force\n") );

	gtk_tree_sortable_get_sort_column_id(sortable, &sort_column_id, &order);
	DB( g_print(" id %d\n order %d\n", sort_column_id, order) );

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, order);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), sort_column_id, order);
}


/*
static gint
ui_asg_listview_compare_default_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
Assign *item1, *item2;

    gtk_tree_model_get(model, a, LST_DEFASG_DATAS, &item1, -1);
    gtk_tree_model_get(model, b, LST_DEFASG_DATAS, &item2, -1);

	return item1->pos - item2->pos;
}
*/


static gint
ui_asg_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
Assign *item1, *item2;
gint retval = 0;

    gtk_tree_model_get(model, a, LST_DEFASG_DATAS, &item1, -1);
    gtk_tree_model_get(model, b, LST_DEFASG_DATAS, &item2, -1);

    switch (sortcol)
    {
		case LST_DEFASG_SORT_POS:
			retval = item1->pos - item2->pos;
			break;
		case LST_DEFASG_SORT_SEARCH:
			retval = hb_string_utf8_compare(item1->search, item2->search);
			break;
		case LST_DEFASG_SORT_PAYEE:
			{
			gchar *name1 = assign_get_target_payee(item1);
			gchar *name2 = assign_get_target_payee(item2);
			retval = hb_string_utf8_compare(name1, name2);
			}
			break;

		case LST_DEFASG_SORT_CATEGORY:
			{
			gchar *name1 = assign_get_target_category(item1);
			gchar *name2 = assign_get_target_category(item2);
			retval = hb_string_utf8_compare(name1, name2);
			}
			break;
		case LST_DEFASG_SORT_PAYMENT:
			retval = item1->paymode - item2->paymode;
			break;			
		case LST_DEFASG_SORT_NOTES:
			retval = hb_string_utf8_compare(item1->notes, item2->notes);
			break;			
		default:
			g_return_val_if_reached(0);
	}
	if( retval == 0 )
		retval = item1->pos - item2->pos;
	
    return retval;
}


static void 
ui_asg_listview_cell_data_func_icons (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Assign *asgitem;
gchar *iconname = NULL;

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &asgitem, -1);

	switch(GPOINTER_TO_INT(user_data))
	{
		case 1:
			iconname = ( asgitem->flags & ASGF_PREFILLED  ) ? ICONNAME_HB_OPE_PREFILLED : NULL;
			break;
	}

	g_object_set(renderer, "icon-name", iconname, NULL);
}


static void
ui_asg_listview_cell_data_function_pos (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Assign *asgitem;
gchar buffer[256];

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &asgitem, -1);
	g_snprintf(buffer, 256-1, "%d", asgitem->pos);
	g_object_set(renderer, "text", buffer, NULL);
}


static void
ui_asg_listview_cell_data_function_search (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Assign *asgitem;
gchar *name;

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &asgitem, -1);
	if(asgitem->search == NULL)
		name = _("(none)");		// can never occurs also
	else
		name = asgitem->search;

	g_object_set(renderer, "text", name, NULL);
}



static void
ui_asg_listview_cell_data_function_payee (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Assign *asgitem;
gchar *text;

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &asgitem, -1);
	text = assign_get_target_payee(asgitem);
	g_object_set(renderer, "text", text, NULL);
}


static void
ui_asg_listview_cell_data_function_category (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Assign *asgitem;
gchar *text;

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &asgitem, -1);
	text = assign_get_target_category(asgitem);
	g_object_set(renderer, "text", text, NULL);
}


static void
ui_asg_listview_cell_data_function_notes (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Assign *asgitem;

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &asgitem, -1);
	g_object_set(renderer, "text", asgitem->notes != NULL ? asgitem->notes : "", NULL);
}


static void
ui_asg_listview_cell_data_function_payment (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Assign *asgitem;

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &asgitem, -1);
	g_object_set(renderer, "icon-name", get_paymode_icon_name(asgitem->paymode), NULL);	
}


#if MYDEBUG == 1
static void
ui_asg_listview_cell_data_function_debugkey (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Payee *item;
gchar *string;

	gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &item, -1);
	string = g_strdup_printf ("[%d]", item->key );
	g_object_set(renderer, "text", string, NULL);
	g_free(string);
}
#endif	



/* = = = = = = = = = = = = = = = = */

/**
 * rul_list_add:
 *
 * Add a single element (useful for dynamics add)
 *
 * Return value: --
 *
 */
void
ui_asg_listview_add(GtkTreeView *treeview, Assign *item)
{

	DB( g_print("\n[ui-asg-listview] add\n") );

	if( item->search != NULL )
	{
	GtkTreeModel *model;
	GtkTreeIter	iter;
	GtkTreePath *path;

		model = gtk_tree_view_get_model(treeview);

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DEFASG_TOGGLE, FALSE,
			LST_DEFASG_DATAS, item,
			-1);

		gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), &iter);
		//5.6 add scroll to active item
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_scroll_to_cell(treeview, path, NULL, FALSE, 0, 0);
		gtk_tree_path_free(path);
	}
}


guint32
ui_asg_listview_get_selected_key(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("\n[ui-asg-listview] get selected key\n") );

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Assign *item;

		gtk_tree_model_get(model, &iter, LST_DEFASG_DATAS, &item, -1);

		if( item!= NULL	 )
			return item->key;
	}
	return 0;
}

void
ui_asg_listview_remove_selected(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("\n[ui-asg-listview] remove selected\n") );

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}
}

/*
static gint ui_asg_glist_compare_func(Assign *a, Assign *b)
{
	return 0; //((gint)a->pos - b->pos);
}
*/

void ui_asg_listview_populate(GtkWidget *view, gchar *needle)
{
GtkTreeModel *model;
GtkTreeIter	iter;
GList *lrul, *list;
gboolean hastext = FALSE;
gboolean insert = TRUE;

	DB( g_print("\n[ui-asg-listview] populate\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

	gtk_list_store_clear (GTK_LIST_STORE(model));

	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view */

	if( needle != NULL )
		hastext = (strlen(needle) >= 2) ? TRUE : FALSE;

	/* populate */
	//g_hash_table_foreach(GLOBALS->h_rul, (GHFunc)ui_asg_listview_populate_ghfunc, model);
	//lrul = list = g_hash_table_get_values(GLOBALS->h_rul);

	lrul = list = assign_glist_sorted(HB_GLIST_SORT_POS);
	while (list != NULL)
	{
	Assign *item = list->data;

		if(hastext)
			insert = hb_string_utf8_strstr(item->search, needle, FALSE);

		if( insert == TRUE )
		{
			DB( g_print(" populate: k%d p%d '%s'\n", item->key, item->pos, item->notes) );

			gtk_list_store_insert_with_values (GTK_LIST_STORE(model), &iter, -1,
				LST_DEFASG_TOGGLE	, FALSE,
				LST_DEFASG_DATAS, item,
				-1);
		}
		list = g_list_next(list);
	}
	g_list_free(lrul);

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view */
	g_object_unref(model);
}


/*
static gboolean ui_asg_listview_search_equal_func (GtkTreeModel *model,
                               gint column,
                               const gchar *key,
                               GtkTreeIter *iter,
                               gpointer search_data)
{
  gboolean retval = TRUE;
  gchar *normalized_string;
  gchar *normalized_key;
  gchar *case_normalized_string = NULL;
  gchar *case_normalized_key = NULL;
  Assign *item;
	
  //gtk_tree_model_get_value (model, iter, column, &value);
  gtk_tree_model_get(model, iter, LST_DEFASG_DATAS, &item, -1);

  if(item !=  NULL)
  {
	  normalized_string = g_utf8_normalize (item->search, -1, G_NORMALIZE_ALL);
	  normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);

	  if (normalized_string && normalized_key)
		{
		  case_normalized_string = g_utf8_casefold (normalized_string, -1);
		  case_normalized_key = g_utf8_casefold (normalized_key, -1);

		  if (strncmp (case_normalized_key, case_normalized_string, strlen (case_normalized_key)) == 0)
		    retval = FALSE;
		}

	  g_free (normalized_key);
	  g_free (normalized_string);
	  g_free (case_normalized_key);
	  g_free (case_normalized_string);
  }
  return retval;
}
*/


static GtkTreeViewColumn *
ui_asg_listview_column_text_create(gchar *title, gint sortcolumnid, GtkTreeCellDataFunc func, gpointer user_data)
{
GtkTreeViewColumn  *column;
GtkCellRenderer    *renderer;

	DB( g_print("\n[ui-asg-listview] text create\n") );

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);
	
	column = gtk_tree_view_column_new_with_attributes(title, renderer, NULL);

	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);

	gtk_tree_view_column_set_sort_column_id (column, sortcolumnid);
	//gtk_tree_view_column_set_reorderable(column, TRUE);
	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	//gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_GROW_ONLY);
	//gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, func, user_data, NULL);

	return column;
}

/*
enum
{
  TARGET_GTK_TREE_MODEL_ROW
};

static GtkTargetEntry row_targets[] = {
  { "GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_APP,
    TARGET_GTK_TREE_MODEL_ROW }
};


static void
data_received (GtkWidget *widget,
               GdkDragContext *context,
               gint x, gint y,
               GtkSelectionData *selda,
               guint info, guint time,
               gpointer dada)
{

	g_print(" drag data received\n");	
}
*/



GtkWidget *
ui_asg_listview_new(gboolean withtoggle)
{
GtkListStore *store;
GtkWidget *treeview;
GtkCellRenderer		*renderer;
GtkTreeViewColumn	*column;

	DB( g_print("\n[ui-asg-listview] new\n") );

	// create list store
	store = gtk_list_store_new(NUM_LST_DEFASG,
		G_TYPE_BOOLEAN,
		G_TYPE_POINTER
		);

	// treeview
	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);


	#if MYDEBUG == 1
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_asg_listview_cell_data_function_debugkey, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	#endif

	// column: toggle
	if( withtoggle == TRUE )
	{
		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Visible"),
							     renderer,
							     "active", LST_DEFASG_TOGGLE,
							     NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

		g_signal_connect (renderer, "toggled",
			    G_CALLBACK (ui_asg_listview_toggled_cb), store);

	}

	// column: icons
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_asg_listview_cell_data_func_icons, GINT_TO_POINTER(1), NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column: position
	renderer = gtk_cell_renderer_text_new ();
	//#2004631 date and column title alignement
	g_object_set(renderer, "xalign", 1.0, NULL);
	
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, "#");
	gtk_tree_view_column_set_sort_column_id (column, LST_DEFASG_SORT_POS);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_asg_listview_cell_data_function_pos, GINT_TO_POINTER(LST_DEFASG_DATAS), NULL);
	//#2004631 date and column title alignement
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	// column : Search
	column = ui_asg_listview_column_text_create(_("Search"), LST_DEFASG_SORT_SEARCH, ui_asg_listview_cell_data_function_search, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column : Notes
	column = ui_asg_listview_column_text_create(_("Notes"), LST_DEFASG_SORT_NOTES, ui_asg_listview_cell_data_function_notes, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column : Payee
	column = ui_asg_listview_column_text_create(_("Payee"), LST_DEFASG_SORT_PAYEE, ui_asg_listview_cell_data_function_payee, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	// column : Category
	column = ui_asg_listview_column_text_create(_("Category"), LST_DEFASG_SORT_CATEGORY, ui_asg_listview_cell_data_function_category, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	// column : Payment
	renderer = gtk_cell_renderer_pixbuf_new ();
	g_object_set(renderer, "xalign", 0.0, NULL);
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Payment"));
	gtk_tree_view_column_set_sort_column_id (column, LST_DEFASG_SORT_PAYMENT);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_asg_listview_cell_data_function_payment, 0, NULL);
	gtk_tree_view_column_set_min_width (column, HB_MINWIDTH_COLUMN);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

  	/* column : empty */
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	
	// treeviewattribute
	//gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), FALSE);

	//5.6 drag n drop is no more possible easily when sort
	//gtk_tree_view_set_reorderable (GTK_TREE_VIEW(treeview), TRUE);

	/*
	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (treeview),
					  GDK_BUTTON1_MASK,
					  row_targets,
					  G_N_ELEMENTS (row_targets),
					  GDK_ACTION_MOVE | GDK_ACTION_COPY);

	gtk_tree_view_enable_model_drag_dest (GTK_TREE_VIEW (treeview),
					row_targets,
					G_N_ELEMENTS (row_targets),
					GDK_ACTION_MOVE | GDK_ACTION_COPY);
	
	g_signal_connect (treeview, "drag-data-received", G_CALLBACK (data_received), NULL);
	*/
			
	//gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_asg_listview_compare_default_func, NULL, NULL);
	//gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);

	//sortable
	
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFASG_SORT_POS, ui_asg_listview_compare_func, GINT_TO_POINTER(LST_DEFASG_SORT_POS), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFASG_SORT_SEARCH, ui_asg_listview_compare_func, GINT_TO_POINTER(LST_DEFASG_SORT_SEARCH), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFASG_SORT_PAYEE, ui_asg_listview_compare_func, GINT_TO_POINTER(LST_DEFASG_SORT_PAYEE), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFASG_SORT_CATEGORY, ui_asg_listview_compare_func, GINT_TO_POINTER(LST_DEFASG_SORT_CATEGORY), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFASG_SORT_PAYMENT, ui_asg_listview_compare_func, GINT_TO_POINTER(LST_DEFASG_SORT_PAYMENT), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFASG_SORT_NOTES, ui_asg_listview_compare_func, GINT_TO_POINTER(LST_DEFASG_SORT_NOTES), NULL);

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_DEFASG_SORT_POS, GTK_SORT_ASCENDING);
	



	//#1897810 add quicksearch
	//gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(treeview), ui_asg_listview_search_equal_func, NULL, NULL);

	return treeview;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void ui_asg_dialog_update(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_dialog_data *data;
gboolean sensitive;

	//ignore event triggered from inactive radiobutton
	//if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE )
	//	return;

	DB( g_print("\n[ui-asg-dialog] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");


	sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_amount));
	gtk_widget_set_sensitive(data->LB_amount, sensitive);
	gtk_widget_set_sensitive(data->ST_amount, sensitive);


	sensitive = (hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_pay)) > 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->LB_pay, sensitive);
	gtk_widget_set_sensitive(data->PO_pay, sensitive);
	
	sensitive = (hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_cat)) > 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->LB_cat, sensitive);
	gtk_widget_set_sensitive(data->PO_cat, sensitive);
	
	sensitive = (hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mod)) > 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->LB_mod, sensitive);
	gtk_widget_set_sensitive(data->NU_mod, sensitive);

	sensitive = (hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_tags)) > 0) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->LB_tags, sensitive);
	gtk_widget_set_sensitive(data->ST_tags, sensitive);
	gtk_widget_set_sensitive(data->CY_tags, sensitive);

}


/*
** rename the selected assign to our treeview and temp GList
*/

static void ui_asg_dialog_rename(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_dialog_data *data;
gboolean error;
gchar *txt;
GString *errstr;
Assign *item;

	DB( g_print("\n[ui-asg-dialog] rename\n") );


	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	errstr = g_string_new(NULL);

	error = FALSE;
	gtk_label_set_text(GTK_LABEL(data->LB_wrntxt), "");
	gtk_style_context_remove_class (gtk_widget_get_style_context (GTK_WIDGET(data->ST_search)), GTK_STYLE_CLASS_ERROR);

	item = data->asgitem;

	txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));
	if( txt == NULL || *txt == '\0' )
	{
		//#2042035
		g_string_append_printf(errstr, _("Search cannot be empty"));
		error = TRUE;
		goto end;
	}

	if( strcmp(txt, item->search) )
	{
	//#2042035
	Assign *existitem = da_asg_get_by_name(txt);

		if( existitem != NULL )
		{
			g_string_append_printf(errstr, _("This search text already exists at position %d"), existitem->pos);
			error = TRUE;
		}
	}

	//#1842897 lead/trail visible if detected
	if( txt != NULL && hb_string_has_leading_trailing(txt) == TRUE )
	{
	gchar *wrntxt;
	gchar **split;

		split = g_strsplit(txt, " ", -1);
		wrntxt = g_strjoinv("\xE2\x90\xA3", split);
		if( errstr->len > 0 )
			g_string_append(errstr, "\n");
		g_string_append(errstr, wrntxt);
		g_free(wrntxt);
		g_strfreev(split);
	}
	
end:
	gtk_label_set_text(GTK_LABEL(data->LB_wrntxt), errstr->str);

	if( errstr->len > 0 )
		gtk_widget_show(data->GR_wrntxt);
	else
		gtk_widget_hide(data->GR_wrntxt);


	g_string_free(errstr, TRUE);

	if( error == TRUE )
	{
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(data->ST_search)), GTK_STYLE_CLASS_ERROR);
	}
	
	//5.7.2 disable OK
	gtk_dialog_set_response_sensitive(GTK_DIALOG (data->dialog), GTK_RESPONSE_ACCEPT, !error);
	
}


static void ui_asg_dialog_get(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_dialog_data *data;
Assign *item;
gint active;
gchar *txt;

	DB( g_print("\n[ui-asg-dialog] get\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	item = data->asgitem;
	if(item != NULL)
	{
		data->change++;

		item->field = hbtk_switcher_get_active (HBTK_SWITCHER(data->CY_field));

		//#2042035		
		txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));
		if (txt && *txt)
		{
			hbtk_entry_replace_text(GTK_ENTRY(data->ST_search), &item->search);
		}

		item->flags = 0;

		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_exact));
		if(active == 1) item->flags |= ASGF_EXACT;

		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_re));
		if(active == 1) item->flags |= ASGF_REGEX;

		//#1710085 assignment based on amount 
		active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_amount));
		if(active == 1) item->flags |= ASGF_AMOUNT;
		
		active = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_pay));
		if(active == 1) item->flags |= ASGF_DOPAY;
		else 
			if(active == 2) item->flags |= ASGF_OVWPAY;
		
		active = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_cat));
		if(active == 1) item->flags |= ASGF_DOCAT;
		else 
			if(active == 2) item->flags |= ASGF_OVWCAT;
		
		active = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_mod));
		if(active == 1) item->flags |= ASGF_DOMOD;
		else 
			if(active == 2) item->flags |= ASGF_OVWMOD;

		active = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_tags));
		if(active == 1) item->flags |= ASGF_DOTAG;
		else 
			if(active == 2) item->flags |= ASGF_OVWTAG;


		item->amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));

		//item->kcat    = ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_cat));
		item->kcat    = ui_cat_entry_popover_get_key_add_new(GTK_BOX(data->PO_cat));
		//item->kpay    = ui_pay_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_pay));
		item->kpay    = ui_pay_entry_popover_get_key_add_new(GTK_BOX(data->PO_pay));
		item->paymode = paymode_combo_box_get_active(GTK_COMBO_BOX(data->NU_mod));

		gchar *txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_tags));
		DB( g_print(" tags: '%s'\n", txt) );
		g_free(item->tags);
		item->tags = tags_parse(txt);

		hbtk_entry_replace_text(GTK_ENTRY(data->ST_notes), &item->notes);

	}

}



/*
** set widgets contents from the selected assign
*/
static void ui_asg_dialog_set(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_dialog_data *data;
Assign *item;
gint active;

	DB( g_print("\n[ui-asg-dialog] set\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	item = data->asgitem;


		DB( g_print(" -> set rul id=%d\n", item->key) );

		hbtk_entry_set_text(GTK_ENTRY(data->ST_search), item->search);
		
		hbtk_switcher_set_active (HBTK_SWITCHER(data->CY_field), item->field);
		
		hbtk_entry_set_text(GTK_ENTRY(data->ST_notes), item->notes);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_exact), (item->flags & ASGF_EXACT) ? 1 : 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_re), (item->flags & ASGF_REGEX) ? 1 : 0);

		//#1710085 assignment based on amount
		//g_signal_handlers_block_by_func(G_OBJECT(data->CM_amount), G_CALLBACK(ui_asg_manage_update_amount), NULL);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_amount), (item->flags & ASGF_AMOUNT ? 1 : 0));
		//g_signal_handlers_unblock_by_func(G_OBJECT(data->CM_amount), G_CALLBACK(ui_asg_manage_update_amount), NULL);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), item->amount);

		active = 0;
		if(item->flags & ASGF_DOPAY) active = 1;
		else if(item->flags & ASGF_OVWPAY) active = 2;
		hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_pay), active);
		//ui_pay_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_pay), item->kpay);
		ui_pay_entry_popover_set_active(GTK_BOX(data->PO_pay), item->kpay);

		active = 0;
		if(item->flags & ASGF_DOCAT) active = 1;
		else if(item->flags & ASGF_OVWCAT) active = 2;
		hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_cat), active);
		//ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_cat), item->kcat);
		ui_cat_entry_popover_set_active(GTK_BOX(data->PO_cat), item->kcat);

		active = 0;
		if(item->flags & ASGF_DOMOD) active = 1;
		else if(item->flags & ASGF_OVWMOD) active = 2;
		hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_mod), active);
		paymode_combo_box_set_active(GTK_COMBO_BOX(data->NU_mod), item->paymode);

		active = 0;
		if(item->flags & ASGF_DOTAG) active = 1;
		else if(item->flags & ASGF_OVWTAG) active = 2;
		hbtk_switcher_set_active (HBTK_SWITCHER(data->RA_tags), active);
		
		gchar *tagstr = tags_tostring(item->tags);
		hbtk_entry_set_text(GTK_ENTRY(data->ST_tags), tagstr);
		g_free(tagstr);

		hbtk_entry_set_text(GTK_ENTRY(data->ST_notes), item->notes);

	//}

}


static void ui_asg_dialog_cb_destroy_cb_destroy(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_dialog_data *data;

	DB( g_print("\n[ui-asg-dialog] destroy cb\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" free data\n") );
	g_free(data);
}


static GtkWidget *ui_asg_dialog_new(GtkWindow *parent, Assign *item)
{
struct ui_asg_dialog_data *data;
GtkWidget *dialog, *content;
GtkWidget *content_grid, *group_grid;
GtkWidget *label, *entry1;
GtkWidget *wbox, *bbox;
GtkWidget *widget;
gint crow, row;
gint w, h, dw, dh;

	DB( g_print("\n[ui-asg-dialog] new\n") );

	data = g_malloc0(sizeof(struct ui_asg_dialog_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("Assignment"),
				GTK_WINDOW(parent),
				0,
			    _("_Cancel"),
			    GTK_RESPONSE_REJECT,
			    _("_OK"),
			    GTK_RESPONSE_ACCEPT,
				NULL);

	data->dialog = dialog;
	data->asgitem = item;

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 4:3
	dw = (dh * 4) / 3;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);


	//store our window private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" window=%p, inst_data=%p\n", dialog, data) );

	//window contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));	 	// return a vbox
	
	/* right area */
	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	hb_widget_set_margin(GTK_WIDGET(content_grid), SPACING_LARGE);
	gtk_box_pack_start(GTK_BOX(content), content_grid, TRUE, TRUE, 0);

	// group :: Rule
	crow = 0;
    group_grid = gtk_grid_new ();
	data->GR_condition = group_grid; 
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	
	row = 0;
	label = make_label_group(_("Condition"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 3, 1);

	row++;
	//label = make_label_widget(_("Con_tains:"));
	label = make_label_widget(_("_Search:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(bbox)), GTK_STYLE_CLASS_LINKED);
	gtk_grid_attach (GTK_GRID (group_grid), bbox, 2, row, 2, 1);
		entry1 = make_string(label);
		data->ST_search = entry1;
		gtk_widget_set_hexpand(entry1, TRUE);
		gtk_box_pack_start(GTK_BOX(bbox), entry1, TRUE, TRUE, 0);
		widget = make_image_toggle_button(ICONNAME_HB_TEXT_CASE, _("Case sensitive"));
		//widget = gtk_toggle_button_new_with_label("Aa");
		//gtk_widget_set_tooltip_text(widget, _("Case sensitive"));
		data->CM_exact = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);
		widget = make_image_toggle_button(ICONNAME_HB_TEXT_REGEX, _("Regular expression"));
		//widget = gtk_toggle_button_new_with_label(".*");
		//gtk_widget_set_tooltip_text(widget, _("Regular expression"));
		data->CM_re = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	row++;
	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	data->GR_wrntxt = bbox;
	gtk_grid_attach (GTK_GRID (group_grid), bbox, 2, row, 2, 1);
	
		widget = gtk_image_new_from_icon_name (ICONNAME_WARNING, GTK_ICON_SIZE_BUTTON);
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);
		widget = make_label(NULL, 0.0, 0.5);
		data->LB_wrntxt = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, TRUE, TRUE, 0);
	
	row++;
	label = make_label_widget(_("_In:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_ASG_FIELD, FALSE);
	data->CY_field = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

	/*row++;
	widget = gtk_check_button_new_with_mnemonic (_("Match _case"));
	data->CM_exact = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic (_("Use _regular expressions"));
	data->CM_re = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);
	*/

	row++;
	widget = gtk_check_button_new_with_mnemonic(_("_AND"));
	data->CM_amount = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	
	bbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (group_grid), bbox, 2, row, 2, 1);
	
		//gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

		label = make_label_widget(_("Amou_nt:"));
		data->LB_amount = label;
		gtk_box_pack_start(GTK_BOX(bbox), label, FALSE, FALSE, 0);

		widget = make_amount(label);
		data->ST_amount = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);


	// group :: Assignments
	crow++;
    group_grid = gtk_grid_new ();
	data->GR_assignment = group_grid;
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	row = 0;
	//label = make_label_group(_("Assign payee"));
	label = make_label_group(_("Assignments"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 3, 1);

	//payee
	row++;
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_ASG_ACTION, FALSE);
	data->RA_pay = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label_widget (_("_Payee:"));
	data->LB_pay = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	//widget = ui_pay_comboboxentry_new(label);
	widget = ui_pay_entry_popover_new(label);
	data->PO_pay = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	//category
	row++;
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_ASG_ACTION, FALSE);
	data->RA_cat = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);

	
	row++;
	label = make_label_widget (_("_Category:"));
	data->LB_cat = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	//widget = ui_cat_comboboxentry_new(label);
	widget = ui_cat_entry_popover_new(label);
	data->PO_cat = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	//payment
	row++;
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_ASG_ACTION, FALSE);
	data->RA_mod = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);

	row++;
	label = make_label_widget (_("Pay_ment:"));
	data->LB_mod = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	widget = make_paymode (label);
	data->NU_mod = widget;
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	//tags
	row++;
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_ASG_ACTION, FALSE);
	data->RA_tags = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	gtk_widget_set_margin_top(widget, SPACING_MEDIUM);

	row++;
	label = make_label_widget (_("_Tags:"));
	data->LB_tags = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	wbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(wbox)), GTK_STYLE_CLASS_LINKED);
	gtk_grid_attach (GTK_GRID (group_grid), wbox, 2, row, 1, 1);

		widget = make_string(label);
		data->ST_tags = widget;
		gtk_box_pack_start (GTK_BOX (wbox), widget, TRUE, TRUE, 0);

		widget = ui_tag_popover_list(data->ST_tags);
		data->CY_tags = widget;
		gtk_box_pack_start (GTK_BOX (wbox), widget, FALSE, FALSE, 0);

	//misc
	crow++;
    group_grid = gtk_grid_new ();
	data->GR_misc = group_grid;
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);

	gtk_widget_set_margin_top(group_grid, SPACING_MEDIUM);
	
	row = 0;
	label = make_label_widget(_("Notes:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	entry1 = make_string(label);
	data->ST_notes = entry1;
	gtk_widget_set_hexpand(entry1, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), entry1, 2, row, 2, 1);


	// connect dialog signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (ui_asg_dialog_cb_destroy_cb_destroy), NULL);
	//g_signal_connect (dialog, "map-event", G_CALLBACK (ui_asg_manage_mapped), &dialog);


	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all (dialog);
	gtk_widget_hide(data->GR_wrntxt);



	g_signal_connect (G_OBJECT (data->ST_search), "changed", G_CALLBACK (ui_asg_dialog_rename), NULL);

	g_signal_connect (data->CM_amount, "toggled", G_CALLBACK (ui_asg_dialog_update), NULL);

	g_signal_connect (G_OBJECT (data->RA_pay), "changed", G_CALLBACK (ui_asg_dialog_update), NULL);

	g_signal_connect (G_OBJECT (data->RA_cat), "changed", G_CALLBACK (ui_asg_dialog_update), NULL);

	g_signal_connect (G_OBJECT (data->RA_mod), "changed", G_CALLBACK (ui_asg_dialog_update), NULL);

	g_signal_connect (G_OBJECT (data->RA_tags), "changed", G_CALLBACK (ui_asg_dialog_update), NULL);


	return dialog;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static void
ui_asg_manage_refilter(struct ui_asg_manage_data *data)
{
gchar *needle;

	DB( g_print("[ui-asg-manage] refilter\n") );

	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));
	ui_asg_listview_populate(data->LV_rul, needle);
}


/*
** update the widgets status and contents from action/selection value
*/
static void ui_asg_manage_update(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
gboolean selected, sensitive, canup, candw, canto;
gint sort_column_id;
//gint searchlen;
GtkSortType sort_order;

	DB( g_print("\n[ui-asg-manage] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//if true there is a selected node
	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_rul)), &model, &iter);

	DB( g_print(" selected = %d\n", selected) );

	sensitive = (selected == TRUE) ? TRUE : FALSE;
	//gtk_widget_set_sensitive(data->BT_mod, sensitive);
	gtk_widget_set_sensitive(data->BT_rem, sensitive);
	gtk_widget_set_sensitive(data->BT_edit, sensitive);
	gtk_widget_set_sensitive(data->BT_dup, sensitive);

	//#1999243/2000629 rewrite up/down/to button sensitivity
	canup = candw = canto = selected;

	DB( g_print(" is_sortable= %d\n", GTK_IS_TREE_SORTABLE(model)) );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_rul));
	sort_column_id = GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID;
	sort_order = GTK_SORT_DESCENDING;
	gtk_tree_sortable_get_sort_column_id(GTK_TREE_SORTABLE(model), &sort_column_id, &sort_order);
	DB( g_print(" sort is colid=%d order=%d (ok is %d %d)\n", sort_column_id, sort_order, LST_DEFASG_SORT_POS, GTK_SORT_ASCENDING) );

	if( !((sort_column_id == LST_DEFASG_SORT_POS) && (sort_order == GTK_SORT_ASCENDING)) )
	{
		canup = candw = FALSE;
		DB( g_print(" sort is not by position ASC\n") );
		goto end;
	}

	if( selected == TRUE )
	{
	Assign *item;

		gtk_tree_model_get(model, &iter, LST_DEFASG_DATAS, &item, -1);
		
		DB( g_print(" item pos is %d\n", item->pos) );

		canup = (item->pos <= 1) ? FALSE : TRUE;
		candw = (item->pos >= da_asg_length()) ? FALSE : TRUE;
	}

end:
	DB( g_print(" can up=%d dw=%d to=%d\n", canup, candw, canto) );
	gtk_widget_set_sensitive(data->BT_up  , canup);
	gtk_widget_set_sensitive(data->BT_down, candw);
	gtk_widget_set_sensitive(data->BT_move, canto);
}


/*
static gboolean ui_asg_manage_focus_out(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
	ui_asg_manage_get(widget, user_data);
	return FALSE;
}
*/


static void ui_asg_manage_popmove(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;
Assign *curitem;
guint32 curpos, newpos;
GList *lrul, *list;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-asg-manage] moveto apply (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_rul));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_poppos));
		newpos = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->ST_poppos));

		gtk_tree_model_get(model, &iter, LST_DEFASG_DATAS, &curitem, -1);
		curpos = curitem->pos;

		if( curpos == newpos )
			goto end;

		lrul = list = assign_glist_sorted(HB_GLIST_SORT_POS);
		while (list != NULL)
		{
		Assign *item = list->data;

			if( item != curitem )
			{
				//move is before
				if( newpos < curpos )
				{
					if( item->pos >= newpos && item->pos < curpos )
						item->pos++;
				}
				//move is after
				else
				{
					if( item->pos > curpos && item->pos <= newpos )
						item->pos--;
				}
			}
			list = g_list_next(list);
		}
		g_list_free(lrul);

		curitem->pos = newpos;
		//#1999243 add change
		data->change++;
		
		ui_asg_listview_sort_force(GTK_TREE_SORTABLE(model), NULL);
	}
end:
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->MB_moveto), FALSE);
}


//#1999243 set maximum position when popover open
static void ui_asg_manage_cb_move_to(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
gint maxpos;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-asg-manage] moveto init (data=%p)\n", data) );

	maxpos = da_asg_length();
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(data->ST_poppos), 1.0, (gdouble)maxpos);
}


static void ui_asg_manage_cb_move_updown(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
GtkDirectionType direction = GPOINTER_TO_INT(user_data);
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;
gboolean hasprvnxt;
Assign *curitem, *prvnxtitem;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-asg-manage] up/down (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_rul));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DEFASG_DATAS, &curitem, -1);
		hasprvnxt = FALSE;
		if( direction == GTK_DIR_UP )
			hasprvnxt = gtk_tree_model_iter_previous(model, &iter);
		else if( direction == GTK_DIR_DOWN )
			hasprvnxt = gtk_tree_model_iter_next(model, &iter);
		
		if( hasprvnxt == TRUE )
		{
		gushort tmp = curitem->pos;
		
			gtk_tree_model_get(model, &iter, LST_DEFASG_DATAS, &prvnxtitem, -1);
			//swap position
			curitem->pos = prvnxtitem->pos;
			prvnxtitem->pos = tmp;
			//#1999243 add change
			data->change++;
			
			ui_asg_listview_sort_force(GTK_TREE_SORTABLE(model), NULL);
		}
	}
}


static void ui_asg_manage_edit(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;
Assign *item;
GtkWidget *dialog;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-asg-manage] edit (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_rul));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DEFASG_DATAS, &item, -1);

		dialog = ui_asg_dialog_new(GTK_WINDOW(data->dialog), item);
		ui_asg_dialog_set(dialog, NULL);
		ui_asg_dialog_update(dialog, NULL);

		gtk_dialog_set_default_response(GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT);

		//wait for the user
		gint result = gtk_dialog_run (GTK_DIALOG (dialog));

		if(result == GTK_RESPONSE_ACCEPT)
		{
			ui_asg_dialog_get(dialog, NULL);
			data->change++;
		}

		// cleanup and destroy
		//ui_asg_dialog_cleanup(dialog);
		gtk_window_destroy (GTK_WINDOW(dialog));
		
		ui_asg_listview_sort_force(GTK_TREE_SORTABLE(model), NULL);
	}
}


static void ui_asg_manage_dup(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;
Assign *item, *newitem;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-asg-manage] dup (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_rul));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DEFASG_DATAS, &item, -1);

		newitem = da_asg_duplicate(item);
		if( newitem )
		{
			ui_asg_listview_add(GTK_TREE_VIEW(data->LV_rul), newitem);
			ui_asg_listview_sort_force(GTK_TREE_SORTABLE(model), NULL);
		}
		else
		{
		gchar *newsearch = g_strdup_printf("%s %s", item->search, _("(copy)") );

			ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
				_("Error"),
				_("Cannot duplicate this Assignment,\n"
				"'%s' already exists."),
				newsearch
			    );
			g_free(newsearch);
		}
	}
}


/*
** add an empty new assign to our temp GList and treeview
*/
static void ui_asg_manage_add(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
GtkWidget *dialog;
Assign *item;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-asg-manage] add (data=%p)\n", data) );

	item = da_asg_malloc();
	item->search = g_strdup_printf( _("(rule %d)"), da_asg_length()+1);

	dialog = ui_asg_dialog_new(GTK_WINDOW(data->dialog), item);
	ui_asg_dialog_set(dialog, NULL);
	ui_asg_dialog_update(dialog, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if(result == GTK_RESPONSE_ACCEPT)
	{
		ui_asg_dialog_get(dialog, NULL);
		da_asg_append(item);
		ui_asg_listview_add(GTK_TREE_VIEW(data->LV_rul), item);
		data->change++;
	}
	else
		da_asg_free(item);

	// cleanup and destroy
	//ui_asg_dialog_cleanup(dialog);
	gtk_window_destroy (GTK_WINDOW(dialog));
}


/*
** delete the selected assign to our treeview and temp GList
*/
static void ui_asg_manage_delete(GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data;
guint32 key;
gint result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui-asg-manage] delete) data=%p\n", data) );

	key = ui_asg_listview_get_selected_key(GTK_TREE_VIEW(data->LV_rul));
	if( key > 0 )
	{
	Assign *item = da_asg_get(key);
	gchar *title = NULL;
	gchar *secondtext;

		title = g_strdup_printf (
			_("Are you sure you want to permanently delete '%s'?"), item->search);

		secondtext = _("If you delete an assignment, it will be permanently lost.");
		
		result = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->dialog),
				title,
				secondtext,
				_("_Delete"),
				TRUE
			);

		g_free(title);
		
		if( result == GTK_RESPONSE_OK )
		{
			da_asg_remove(key);
			ui_asg_listview_remove_selected(GTK_TREE_VIEW(data->LV_rul));
			data->change++;
		}
		
		da_asg_update_position();
	}
}


static gboolean ui_asg_manage_cb_on_key_press(GtkWidget *source, GdkEvent *event, gpointer user_data)
{
struct ui_asg_manage_data *data = user_data;
GdkModifierType state;
guint keyval;

	gdk_event_get_state (event, &state);
	gdk_event_get_keyval(event, &keyval);

	DB( g_printf("\n[ui-asg-manage] cb key press\n") );
	
	// On Control-f enable search entry
	if (state & GDK_CONTROL_MASK && keyval == GDK_KEY_f)
	{
		gtk_widget_grab_focus(data->ST_search);
	}
	else
	if (keyval == GDK_KEY_Escape && gtk_widget_has_focus(data->ST_search))
	{
		hbtk_entry_set_text(GTK_ENTRY(data->ST_search), NULL);
		gtk_widget_grab_focus(data->LV_rul);
		return TRUE;
	}

	return GDK_EVENT_PROPAGATE;
}



static void
ui_asg_manage_cb_row_activated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
	DB( g_printf("\n[ui-asg-manage] row activated\n") );
	ui_asg_manage_edit(GTK_WIDGET(treeview), userdata);
}


static void
ui_asg_manage_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	DB( g_printf("\n[ui-asg-manage] selection\n") );

	ui_asg_manage_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}


static void
ui_asg_manage_sort_changed(GtkTreeSortable *sortable, gpointer user_data)
{
struct ui_asg_manage_data *data = user_data;

	DB( g_printf("\n[ui-asg-manage] sort changed\n") );

	ui_asg_manage_update(data->dialog, NULL);
}


static void
ui_asg_manage_search_changed_cb (GtkWidget *widget, gpointer user_data)
{
struct ui_asg_manage_data *data = user_data;

	DB( g_printf("\n[ui-asg-manage] search_changed_cb\n") );

	ui_asg_manage_refilter(data);
}



static gboolean
ui_asg_manage_cleanup(struct ui_asg_manage_data *data, gint result)
{
//GtkTreeModel *tree_model;
gboolean doupdate = FALSE;

	DB( g_print("\n[ui-asg-manage] cleanup data=%p\n", data) );

	//tree_model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_rul));
	//data->change += ui_asg_manage_persist_position(tree_model);

	GLOBALS->changes_count += data->change;

	return doupdate;
}


static void ui_asg_manage_setup(struct ui_asg_manage_data *data)
{

	DB( g_print("\n[ui-asg-manage] setup\n") );

	DB( g_print(" init data\n") );

	//init GList
	data->tmp_list = NULL; //hb-glist_clone_list(GLOBALS->rul_list, sizeof(struct _Assign));
	data->change = 0;

	DB( g_print(" populate\n") );
	
	da_asg_update_position();

	ui_asg_listview_populate(data->LV_rul, NULL);
	//5.5 done in popover
	//ui_pay_comboboxentry_populate(GTK_COMBO_BOX(data->PO_pay), GLOBALS->h_pay);
	//ui_cat_comboboxentry_populate(GTK_COMBO_BOX(data->PO_cat), GLOBALS->h_cat);

	//DB( g_print(" set widgets default\n") );
	
	DB( g_print(" connect widgets signals\n") );

	//gtk_tree_view_set_search_entry(GTK_TREE_VIEW(data->LV_rul), GTK_ENTRY(data->ST_search));
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(data->LV_rul), FALSE);

	g_signal_connect (data->ST_search, "search-changed", G_CALLBACK (ui_asg_manage_search_changed_cb), data);
	
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_rul)), "changed", G_CALLBACK (ui_asg_manage_selection), NULL);
	g_signal_connect (data->LV_rul, "row-activated", G_CALLBACK (ui_asg_manage_cb_row_activated), NULL);
	g_signal_connect (gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_rul)), "sort-column-changed", G_CALLBACK (ui_asg_manage_sort_changed), data);

	g_signal_connect (G_OBJECT (data->BT_add), "clicked", G_CALLBACK (ui_asg_manage_add), NULL);
	g_signal_connect (G_OBJECT (data->BT_rem), "clicked", G_CALLBACK (ui_asg_manage_delete), NULL);
	
	g_signal_connect (G_OBJECT (data->BT_edit), "clicked", G_CALLBACK (ui_asg_manage_edit), NULL);
	g_signal_connect (G_OBJECT (data->BT_dup), "clicked", G_CALLBACK (ui_asg_manage_dup), NULL);

	g_signal_connect (G_OBJECT (data->BT_up  ), "clicked", G_CALLBACK (ui_asg_manage_cb_move_updown), GUINT_TO_POINTER(GTK_DIR_UP));
	g_signal_connect (G_OBJECT (data->BT_down), "clicked", G_CALLBACK (ui_asg_manage_cb_move_updown), GUINT_TO_POINTER(GTK_DIR_DOWN));
	g_signal_connect (G_OBJECT (data->BT_move), "clicked", G_CALLBACK (ui_asg_manage_cb_move_to), NULL);

	// popover signals
	g_signal_connect (G_OBJECT (data->ST_poppos), "activate", G_CALLBACK (ui_asg_manage_popmove), NULL);
	g_signal_connect (G_OBJECT (data->BT_popmove), "clicked", G_CALLBACK (ui_asg_manage_popmove), NULL);

}


static gboolean
ui_asg_manage_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct ui_asg_manage_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[ui-asg-manage] mapped\n") );

	ui_asg_manage_setup(data);
	ui_asg_manage_update(data->LV_rul, NULL);
	gtk_widget_grab_focus(GTK_WIDGET(data->LV_rul));

	data->mapped_done = TRUE;

	return FALSE;
}


static GtkWidget *ui_asg_popover_move_after_new(struct ui_asg_manage_data *data)
{
GtkWidget *box, *menubutton, *label, *widget, *image;
GtkWidget *pop_content;

	DB( g_print("\n[ui-asg-manage] create popmove\n") );

	menubutton = gtk_menu_button_new ();
	data->MB_moveto = menubutton;
	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	//label = gtk_label_new_with_mnemonic (_("Use a _template"));
	//gtk_box_pack_start (GTK_BOX(box), label, FALSE, FALSE, 0);
	image = gtk_image_new_from_icon_name (ICONNAME_LIST_MOVE_AFTER, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start (GTK_BOX(box), image, FALSE, FALSE, 0);
	gtk_container_add(GTK_CONTAINER(menubutton), box);
	gtk_widget_set_tooltip_text(menubutton, _("Move to..."));

	gtk_menu_button_set_direction (GTK_MENU_BUTTON(menubutton), GTK_ARROW_DOWN );
	gtk_widget_set_halign (menubutton, GTK_ALIGN_END);
	//gtk_widget_set_hexpand (menubutton, TRUE);
	gtk_widget_show_all(menubutton);

	pop_content = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_SMALL);

	widget = make_label_group(_("Move rule"));
	gtk_box_pack_start(GTK_BOX(pop_content), widget, FALSE, FALSE, 0);

	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(pop_content), widget, TRUE, TRUE, 0);

	box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_box_pack_start(GTK_BOX(pop_content), box, TRUE, TRUE, 0);

		label = make_label_widget(_("_To:"));
		gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);

		widget = make_numeric(label, 1, 99);
		data->ST_poppos = widget;
		gtk_entry_set_width_chars(GTK_ENTRY(widget), 10);
		gtk_box_pack_start(GTK_BOX(box), widget, FALSE, FALSE, 0);

	widget = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(pop_content), widget, TRUE, TRUE, 0);

	box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_END);
	gtk_box_pack_start(GTK_BOX(pop_content), box, TRUE, TRUE, 0);
	
		widget = gtk_button_new_with_mnemonic(_("Move"));
		data->BT_popmove = widget;
		gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(widget)), GTK_STYLE_CLASS_SUGGESTED_ACTION);
		gtk_box_pack_end(GTK_BOX(box), widget, TRUE, TRUE, 0);
	
	gtk_widget_show_all(pop_content);
	
	GtkWidget *popover = create_popover (menubutton, pop_content, GTK_POS_TOP);

	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), popover);

	return menubutton;
}



GtkWidget *ui_asg_manage_dialog (void)
{
struct ui_asg_manage_data *data;
GtkWidget *dialog, *content_area, *bbox, *hbox, *vbox, *tbar;
GtkWidget *box, *treeview, *scrollwin;
GtkWidget *widget, *content;
gint w, h, dw, dh;

	DB( g_print("\n[ui-asg-manage] dialog\n") );

	data = g_malloc0(sizeof(struct ui_asg_manage_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("Manage Assignments"),
				GTK_WINDOW(GLOBALS->mainwindow),
				0,
			    _("_Close"),
			    GTK_RESPONSE_ACCEPT,
				NULL);

	data->dialog = dialog;

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 3:2
	dw = (dh * 3) / 2;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);


	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" dialog=%p, inst_data=%p\n", dialog, data) );

	//dialog content
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));	 	// return a vbox

	content = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	hb_widget_set_margin(GTK_WIDGET(content), SPACING_LARGE);
	gtk_box_pack_start (GTK_BOX (content_area), content, TRUE, TRUE, 0);
	
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (content), hbox, FALSE, FALSE, 0);

		box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_box_pack_start (GTK_BOX (hbox), box, TRUE, TRUE, 0);

	widget = make_search ();
	data->ST_search = widget;
	gtk_widget_set_size_request(widget, HB_MINWIDTH_SEARCH, -1);
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	// list + toolbar
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start (GTK_BOX (content), vbox, TRUE, TRUE, 0);
	
	// listview
	scrollwin = make_scrolled_window(GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	treeview = ui_asg_listview_new(FALSE);
	data->LV_rul = treeview;
	gtk_widget_set_size_request(treeview, HB_MINWIDTH_LIST, -1);
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_box_pack_start (GTK_BOX (vbox), scrollwin, TRUE, TRUE, 0);

	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_pack_start (GTK_BOX (vbox), tbar, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_ADD, _("Add"));
		data->BT_add = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		data->BT_rem = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_EDIT, _("Edit"));
		data->BT_edit = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_DUPLICATE, _("Duplicate"));
		data->BT_dup = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (tbar), bbox, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_MOVE_UP, _("Move up"));
		data->BT_up = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

		widget = make_image_button(ICONNAME_LIST_MOVE_DOWN, _("Move down"));
		data->BT_down = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

		widget = ui_asg_popover_move_after_new(data);
		data->BT_move = widget;
		gtk_box_pack_start(GTK_BOX(bbox), widget, FALSE, FALSE, 0);

	// connect dialog signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);
	g_signal_connect (dialog, "map-event", G_CALLBACK (ui_asg_manage_mapped), &dialog);
	g_signal_connect (dialog, "key-press-event", G_CALLBACK (ui_asg_manage_cb_on_key_press), (gpointer)data);

	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all(content);
	gtk_widget_show (dialog);

	// wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup and destroy
	ui_asg_manage_cleanup(data, result);
	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(data);
	
	return NULL;
}


