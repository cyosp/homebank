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
#include "hbtk-switcher.h"
#include "ui-category.h"

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


extern gchar *CYA_CAT_TYPE[];


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static GtkWidget *
container_get_nth(GtkBox *container, gint nth)
{
GList *lchild, *list;
GtkWidget *child;

	if(!GTK_IS_CONTAINER(container))
		return NULL;

	lchild = list = gtk_container_get_children (GTK_CONTAINER(container));
	child = g_list_nth_data (list, nth);
	g_list_free(lchild);
	
	return child;
}


GtkWidget *
ui_cat_entry_popover_get_entry(GtkBox *box)
{
	return container_get_nth(box, 0);
}


Category *
ui_cat_entry_popover_get(GtkBox *box)
{
GtkWidget *entry;
gchar *name;
Category *item = NULL;

	DB( g_print ("ui_cat_entry_popover_get()\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
		name = (gchar *)gtk_entry_get_text(GTK_ENTRY (entry));
		item = da_cat_get_by_fullname(name);
	}
	return item;
}


guint32
ui_cat_entry_popover_get_key_add_new(GtkBox *box)
{
Category *item;
GtkWidget *entry;
GtkTreeModel *store;

	DB( g_print ("ui_cat_entry_popover_get_key_add_new()\n") );

	/* automatic add */
	//todo: check prefs + ask the user here 1st time
	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
	gchar *name = (gchar *)gtk_entry_get_text(GTK_ENTRY (entry));

		item = da_cat_get_by_fullname(name);
		if(item != NULL)
			return item->key;

		item = da_cat_append_ifnew_by_fullname(name);
		if( item != NULL )
		{
			store = gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(entry)));
			if( store )
				gtk_list_store_insert_with_values(GTK_LIST_STORE(store), NULL, -1,
					STO_CAT_DATA, item,
					STO_CAT_FULLNAME, item->fullname,
					-1);
			return item->key;
		}
	}

	return 0;
}


guint32
ui_cat_entry_popover_get_key(GtkBox *box)
{
Category *item = ui_cat_entry_popover_get(box);

	return ((item != NULL) ? item->key : 0);
}


void
ui_cat_entry_popover_set_active(GtkBox *box, guint32 key)
{
GtkWidget *entry;

	DB( g_print ("[cat entry popover] setactive\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
	Category *item = da_cat_get(key);

		hbtk_entry_set_text(GTK_ENTRY(entry), item != NULL ? item->fullname : "");
	}
}


void
ui_cat_entry_popover_add(GtkBox *box, Category *item)
{
GtkWidget *entry;

    DB( g_print ("[cat entry popover] add\n") );

    DB( g_print (" -> try to add: '%s'\n", item->name) );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
		if( item->name != NULL )
		{
		GtkTreeModel *model;
		GtkTreeIter  iter;

			model = gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(entry)));

			gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			gtk_list_store_set (GTK_LIST_STORE(model), &iter,
					STO_CAT_DATA, item,
					STO_CAT_FULLNAME, item->fullname,
					-1);
		}
	}

}


static void 
ui_cat_entry_popover_cb_row_activated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
GtkTreeSelection *treeselection;
GtkTreeModel *model;
GtkTreeIter iter;
GtkEntry *entry = user_data;

	if( GTK_IS_ENTRY(entry) )
	{
		treeselection = gtk_tree_view_get_selection(tree_view);
		if( gtk_tree_selection_get_selected(treeselection, &model, &iter) )
		{
		Category *item;

			gtk_tree_model_get(model, &iter, STO_CAT_DATA, &item, -1);
			if(item)
				gtk_entry_set_text(GTK_ENTRY(user_data), item->fullname);
		}
	}
}


static void
ui_cat_entry_popover_text_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Category *item;
gchar *markup;

	gtk_tree_model_get(model, iter, 
		STO_CAT_DATA, &item, 
		-1);

	markup = (item->key == 0) ? _("(no category)") : item->typename;

	g_object_set(renderer, "markup", markup, NULL);
}


static void
ui_cat_entry_popover_populate(GtkListStore *store)
{
GHashTableIter hiter;
gpointer key, value;
	
	g_hash_table_iter_init (&hiter, GLOBALS->h_cat);
	while (g_hash_table_iter_next (&hiter, &key, &value))
	{
	Category *item = value;

		//#1826360 wish: archive payee/category to lighten the lists
		if( !(item->flags & GF_HIDDEN) )
		{
			gtk_list_store_insert_with_values(GTK_LIST_STORE(store), NULL, -1,
				STO_CAT_DATA, item,
				STO_CAT_FULLNAME, item->fullname,
				-1);
		}
	}

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
}


static void
ui_cat_entry_popover_function (GtkEditable *editable, gpointer user_data)
{

	DB( g_print("text changed to %s\n", gtk_entry_get_text(GTK_ENTRY(editable)) ) );
	


}


static void
ui_cat_entry_popover_cb_toggled (GtkToggleButton *togglebutton, gpointer user_data)
{
GtkWidget *entry = user_data;
GtkAllocation allocation;
GtkPopover *popover;

	if(GTK_IS_ENTRY(entry))
	{
		gtk_widget_get_allocation (entry, &allocation);
		popover = gtk_menu_button_get_popover(GTK_MENU_BUTTON(togglebutton));
		if(GTK_IS_POPOVER(popover))
		{
			gtk_widget_set_size_request (GTK_WIDGET(popover), allocation.width + (2*SPACING_POPOVER), -1);	
			DB( g_print("should set width to %d\n", allocation.width + (2*SPACING_POPOVER)) );	
		}
	}
}


void
ui_cat_entry_popover_clear(GtkBox *box)
{
GtkWidget *entry;
GtkTreeModel *store;

    DB( g_print ("[cat entry popover] clear\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
		store = gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(entry)));
		gtk_list_store_clear (GTK_LIST_STORE(store));
	}
}


void
ui_cat_entry_popover_sort_type(GtkBox *box, guint type)
{
GtkWidget *entry;
GtkTreeModel *store;

    DB( g_print ("[cat entry popover] sort type\n") );

	entry = container_get_nth(box, 0);
	if( entry != NULL && GTK_IS_ENTRY(entry) )
	{
		store = gtk_entry_completion_get_model(gtk_entry_get_completion(GTK_ENTRY(entry)));
		g_object_set_data(G_OBJECT(store), "type", GUINT_TO_POINTER(type));
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
	}
}


static gint
ui_cat_entry_popover_compare_func (GtkTreeModel *model, GtkTreeIter  *a, GtkTreeIter  *b, gpointer      userdata)
{
gint retval = 0;
Category *cat1, *cat2;
Category *pcat1, *pcat2;
guint type;

    gtk_tree_model_get(model, a, STO_CAT_DATA, &cat1, -1);
    gtk_tree_model_get(model, b, STO_CAT_DATA, &cat2, -1);

	if(cat1->key == 0)
		return -1;
	
	if(cat2->key == 0)
		return 1;

	//#2042484 exp/inc sort should be done on lvl1 only
	pcat1 = cat1->parent == 0 ? cat1 : da_cat_get(cat1->parent);
	pcat2 = cat2->parent == 0 ? cat2 : da_cat_get(cat2->parent);

	//#1882456
	type = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(model), "type"));
	switch(type)
	{
		case TXN_TYPE_INCOME:
			// inc first
			retval = (pcat2->flags & GF_INCOME) - (pcat1->flags & GF_INCOME);
			break;
		default:
			retval = (pcat1->flags & GF_INCOME) - (pcat2->flags & GF_INCOME);
	}

	if( !retval )
		retval = hb_string_utf8_compare(cat1->fullname, cat2->fullname);

  	return retval;
}



static gboolean
ui_cat_entry_popover_completion_func (GtkEntryCompletion *completion,
                                              const gchar        *key,
                                              GtkTreeIter        *iter,
                                              gpointer            user_data)
{
  Category *item = NULL;
  gchar *normalized_string;
  gchar *case_normalized_string;

  gboolean ret = FALSE;

  GtkTreeModel *model;

  model = gtk_entry_completion_get_model (completion);

  gtk_tree_model_get (model, iter,
                      STO_CAT_DATA, &item,
                      -1);

  if (item != NULL)
    {
      normalized_string = g_utf8_normalize (item->fullname, -1, G_NORMALIZE_ALL);

      if (normalized_string != NULL)
        {
          case_normalized_string = g_utf8_casefold (normalized_string, -1);

			//g_print("match '%s' for '%s' ?\n", key, case_normalized_string);
		//if (!strncmp (key, case_normalized_string, strlen (key)))
          if (g_strstr_len (case_normalized_string, strlen (case_normalized_string), key ))
			{
        		ret = TRUE;
				//	g_print(" ==> yes !\n");
				
			}
				
          g_free (case_normalized_string);
        }
      g_free (normalized_string);
    }

  return ret;
}



static void 
ui_cat_entry_popover_destroy( GtkWidget *widget, gpointer user_data )
{

    DB( g_print ("[cat entry popover] destroy\n") );

}


GtkWidget *
ui_cat_entry_popover_new(GtkWidget *label)
{
GtkWidget *mainbox, *box, *entry, *menubutton, *image, *popover, *scrollwin, *treeview;
GtkListStore *store;
GtkEntryCompletion *completion;

    DB( g_print ("[pay entry popover] new\n") );

	mainbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET(mainbox)), GTK_STYLE_CLASS_LINKED);
	
	entry = gtk_entry_new();
	hbtk_box_prepend (GTK_BOX(mainbox), entry);

	menubutton = gtk_menu_button_new ();
	//data->MB_template = menubutton;
	image = hbtk_image_new_from_icon_name_16 ("pan-down-symbolic");
	gtk_button_set_image(GTK_BUTTON(menubutton), image);
	gtk_menu_button_set_direction (GTK_MENU_BUTTON(menubutton), GTK_ARROW_LEFT );
	//gtk_widget_set_halign (menubutton, GTK_ALIGN_END);
	gtk_box_prepend(GTK_BOX(mainbox), menubutton);
	
    completion = gtk_entry_completion_new ();

	gtk_entry_set_completion (GTK_ENTRY (entry), completion);
	g_object_unref(completion);
	
	store = gtk_list_store_new (NUM_STO_CAT,
		G_TYPE_POINTER,
		G_TYPE_STRING
		);

	gtk_tree_sortable_set_default_sort_func(GTK_TREE_SORTABLE(store), ui_cat_entry_popover_compare_func, NULL, NULL);

	ui_cat_entry_popover_populate(store);

    gtk_entry_completion_set_model (completion, GTK_TREE_MODEL(store));
	gtk_entry_completion_set_match_func(completion, ui_cat_entry_popover_completion_func, NULL, NULL);
	g_object_unref(store);

	gtk_entry_completion_set_text_column (completion, STO_CAT_FULLNAME);

	gtk_widget_show_all(mainbox);


	box = gtk_box_new(GTK_ORIENTATION_VERTICAL, SPACING_MEDIUM);
	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	hbtk_box_prepend (GTK_BOX(box), scrollwin);
	//gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin), HB_MINHEIGHT_LIST);
	treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL(store));
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_widget_show_all(box);


	//gtk_widget_set_can_focus(GTK_WIDGET(treeview), FALSE);

GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
	
  renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
		//taken from nemo, not exactly a resize to content, but good compromise
	    "width-chars", 40,
	    NULL);

	column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     STO_CAT_FULLNAME,
                                                     NULL);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_cat_entry_popover_text_cell_data_function, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);


	gtk_tree_view_set_hover_selection(GTK_TREE_VIEW(treeview), TRUE);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(treeview), FALSE);
	gtk_tree_view_set_activate_on_single_click(GTK_TREE_VIEW(treeview), TRUE);

	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(treeview), FALSE);

	//gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_BROWSE);
	
	
	//popover = create_popover (menubutton, box, GTK_POS_BOTTOM);
	popover = create_popover (menubutton, box, GTK_POS_LEFT);
	//gtk_widget_set_size_request (popover, HB_MINWIDTH_LIST, HB_MINHEIGHT_LIST);
	gtk_widget_set_vexpand(popover, TRUE);
	
	gtk_menu_button_set_popover(GTK_MENU_BUTTON(menubutton), popover);
	
	
	// connect our dispose function
	g_signal_connect (entry, "destroy", G_CALLBACK (ui_cat_entry_popover_destroy), NULL);

	g_signal_connect_after (entry  , "changed", G_CALLBACK (ui_cat_entry_popover_function), NULL);

	g_signal_connect (menubutton, "toggled", G_CALLBACK (ui_cat_entry_popover_cb_toggled), entry);
	
	g_signal_connect (treeview, "row-activated", G_CALLBACK (ui_cat_entry_popover_cb_row_activated), entry);

	#if( (GTK_MAJOR_VERSION == 3) && (GTK_MINOR_VERSION >= 22) )
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_popover_popdown), popover);
	#else
		g_signal_connect_swapped(treeview, "row-activated", G_CALLBACK(gtk_widget_hide), popover);
	#endif

	//g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK (ui_cat_entry_popover_cb_selection), entry);
	//g_signal_connect_swapped(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK(gtk_popover_popdown), popover);
	
	if(label)
		gtk_label_set_mnemonic_widget (GTK_LABEL(label), entry);

	//gtk_widget_set_size_request(comboboxentry, HB_MINWIDTH_LIST, -1);

	return mainbox;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


guint
ui_cat_listview_toggle_to_filter(GtkTreeView *treeview, Filter *filter)
{
GtkTreeModel *model;
//GtkTreeSelection *selection;
GtkTreeIter	iter, child;
guint n_child, change = 0;
gboolean valid;
gboolean toggled;

	DB( g_print("(ui_acc_listview) toggle_to_filter\n") );
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	//selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Category *catitem;

		gtk_tree_model_get (model, &iter,
			LST_DEFCAT_TOGGLE, &toggled,
			LST_DEFCAT_DATAS , &catitem,
			-1);

		DB( g_print("    cat k:%3d = %d (%s)\n", catitem->key, toggled, catitem->name) );
		change += da_flt_status_cat_set(filter, catitem->key, toggled);

		//catitem->flt_select = toggled;

		n_child = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(model), &iter);
		gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &iter);
		while(n_child > 0)
		{
			gtk_tree_model_get (model, &child,
				LST_DEFCAT_TOGGLE, &toggled,
				LST_DEFCAT_DATAS, &catitem,
				-1);

			DB( g_print(" subcat k:%3d = %d (%s)\n", catitem->key, toggled, catitem->name) );
			change += da_flt_status_cat_set(filter, catitem->key, toggled);

			//catitem->flt_select = toggled;

			n_child--;
			gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
		}

		/* Make iter point to the next row in the list store */
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
	return change;
}


static void
ui_cat_listview_fixed_toggled (GtkCellRendererToggle *cell,
	       gchar                 *path_str,
	       gpointer               data)
{
GtkTreeModel *model = (GtkTreeModel *)data;
GtkTreeIter  iter, child;
GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
gboolean fixed;
gint n_child;

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, &iter, LST_DEFCAT_TOGGLE, &fixed, -1);

	/* do something with the value */
	fixed ^= 1;

	/* set new value */
	gtk_tree_store_set (GTK_TREE_STORE (model), &iter, LST_DEFCAT_TOGGLE, fixed, -1);

	/* propagate to child */
	n_child = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(model), &iter);
	gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &iter);
	while(n_child > 0)
	{
		gtk_tree_store_set (GTK_TREE_STORE (model), &child, LST_DEFCAT_TOGGLE, fixed, -1);

		n_child--;
		gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
	}
	
	/* clean up */
	gtk_tree_path_free (path);
}


void
ui_cat_listview_quick_select(GtkTreeView *treeview, const gchar *uri)
{
GtkTreeModel *model;
GtkTreeIter	iter, child;
gboolean valid;
gboolean toggle;
gint n_child, qselect = hb_clicklabel_to_int(uri);

	DB( g_print("(ui_acc_listview) quick select\n") );


	DB( g_print(" comboboxlink '%s' %d\n", uri, qselect) );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
		switch(qselect)
		{
			case HB_LIST_QUICK_SELECT_ALL:
				gtk_tree_store_set (GTK_TREE_STORE (model), &iter, LST_DEFCAT_TOGGLE, TRUE, -1);
				break;
			case HB_LIST_QUICK_SELECT_NONE:
				gtk_tree_store_set (GTK_TREE_STORE (model), &iter, LST_DEFCAT_TOGGLE, FALSE, -1);
				break;
			case HB_LIST_QUICK_SELECT_INVERT:
					gtk_tree_model_get (model, &iter, LST_DEFCAT_TOGGLE, &toggle, -1);
					toggle ^= 1;
					gtk_tree_store_set (GTK_TREE_STORE (model), &iter, LST_DEFCAT_TOGGLE, toggle, -1);
				break;
		}

		n_child = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(model), &iter);
		gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &iter);
		while(n_child > 0)
		{

			switch(qselect)
			{
				case HB_LIST_QUICK_SELECT_ALL:
					gtk_tree_store_set (GTK_TREE_STORE (model), &child, LST_DEFCAT_TOGGLE, TRUE, -1);
					break;
				case HB_LIST_QUICK_SELECT_NONE:
					gtk_tree_store_set (GTK_TREE_STORE (model), &child, LST_DEFCAT_TOGGLE, FALSE, -1);
					break;
				case HB_LIST_QUICK_SELECT_INVERT:
						gtk_tree_model_get (model, &child, LST_DEFCAT_TOGGLE, &toggle, -1);
						toggle ^= 1;
						gtk_tree_store_set (GTK_TREE_STORE (model), &child, LST_DEFCAT_TOGGLE, toggle, -1);
					break;
			}

			n_child--;
			gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
		}

		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

}



static gint
ui_cat_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
gint sortcol = GPOINTER_TO_INT(userdata);
Category *entry1, *entry2;
gint retval = 0;
	
	gtk_tree_model_get(model, a, LST_DEFCAT_DATAS, &entry1, -1);
	gtk_tree_model_get(model, b, LST_DEFCAT_DATAS, &entry2, -1);

    switch (sortcol)
    {
		case LST_DEFCAT_SORT_NAME:
			retval = (entry1->flags & GF_INCOME) - (entry2->flags & GF_INCOME);
			if(!retval)
			{
				retval = hb_string_utf8_compare(entry1->name, entry2->name);
			}
			break;
		case LST_DEFCAT_SORT_USETXN:
			retval = entry1->nb_use_txn - entry2->nb_use_txn;
			break;
		case LST_DEFCAT_SORT_USECFG:
			retval = (entry1->nb_use_all - entry1->nb_use_txn) - (entry2->nb_use_all - entry2->nb_use_txn);
			break;
		default:
			g_return_val_if_reached(0);
	}
    return retval;
}


/*
** draw some text from the stored data structure
*/
static void
ui_cat_listview_text_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Category *item;
gchar *markup;

	gtk_tree_model_get(model, iter, 
		LST_DEFCAT_DATAS, &item,
		-1);

	markup = (item->key == 0) ? _("(no category)") : item->typename;

	g_object_set(renderer, "markup", markup, NULL);
}


static void
ui_cat_listview_count_txn_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Category *entry;
gchar buffer[32];
guint16 use;
guint16 usecat;

	buffer[0] = '\0';
	gtk_tree_model_get(model, iter, LST_DEFCAT_DATAS, &entry, -1);
	use = entry->nb_use_txn;
	usecat = entry->nb_use_txncat;
	if(use > 0)
	{
		if( !(entry->flags & GF_SUB) && (usecat != use) ) 
			g_snprintf(buffer, 32-1, "%d (%d)", use, usecat);
		else
			g_snprintf(buffer, 32-1, "%d", use);
	}
	g_object_set(renderer, "text", buffer, NULL);
}


static void
ui_cat_listview_count_cfg_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Category *entry;
gchar buffer[32];
gint use;
gint usecat;

	buffer[0] = '\0';
	gtk_tree_model_get(model, iter, LST_DEFCAT_DATAS, &entry, -1);
	use = entry->nb_use_all - entry->nb_use_txn;
	usecat = entry->nb_use_allcat - entry->nb_use_txncat;
	if(use > 0 || usecat > 0)
	{
		if( !(entry->flags & GF_SUB) && (usecat != use) ) 
			g_snprintf(buffer, 32-1, "%d (%d)", use, usecat);
		else
			g_snprintf(buffer, 32-1, "%d", use);
	}

	g_object_set(renderer, "text", buffer, NULL);
}


static void 
ui_cat_listview_icon_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Category *entry;
gchar *iconname = NULL;

	gtk_tree_model_get(model, iter, LST_DEFCAT_DATAS, &entry, -1);
	if( entry->flags & GF_HIDDEN )
		iconname = ICONNAME_HB_BUTTON_HIDE;

	g_object_set(renderer, "icon-name", iconname, NULL);
}


#if MYDEBUG == 1
static void
ui_cat_listview_cell_data_function_debugkey (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Category *item;
gchar *string;

	gtk_tree_model_get(model, iter, LST_DEFCAT_DATAS, &item, -1);
	string = g_strdup_printf ("[%d]", item->key );
	g_object_set(renderer, "text", string, NULL);
	g_free(string);
}
#endif	


/* = = = = = = = = = = = = = = = = */


void
ui_cat_listview_add(GtkTreeView *treeview, Category *item, GtkTreeIter	*parent)
{
GtkTreeModel *model;
GtkTreeIter	iter;
GtkTreePath *path;

	DB( g_print ("ui_cat_listview_add()\n") );

	if( item->name != NULL )
	{
		model = gtk_tree_view_get_model(treeview);

		gtk_tree_store_append (GTK_TREE_STORE(model), &iter, parent);
		gtk_tree_store_set (GTK_TREE_STORE(model), &iter,
			LST_DEFCAT_TOGGLE, FALSE,
			LST_DEFCAT_DATAS, item,
			//LST_DEFCAT_NAME, item->name,
			-1);

		//select the added line

		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_view_expand_to_path (treeview, path);
		gtk_tree_path_free(path);
		gtk_tree_selection_select_iter (gtk_tree_view_get_selection(treeview), &iter);
	}

}

Category *
ui_cat_listview_get_selected(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print ("ui_cat_listview_get_selected()\n") );

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Category *item;

		gtk_tree_model_get(model, &iter, LST_DEFCAT_DATAS, &item, -1);
		if( item->key != 0 )
			return item;
	}
	return NULL;
}

Category *
ui_cat_listview_get_selected_parent(GtkTreeView *treeview, GtkTreeIter *return_iter)
{
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
GtkTreePath *path;
Category *item;

	DB( g_print ("ui_cat_listview_get_selected_parent()\n") );


	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		path = gtk_tree_model_get_path(model, &iter);

		DB( g_print ("path depth = %d\n", gtk_tree_path_get_depth(path)) );


		if(gtk_tree_path_get_depth(path) > 1)
		{
			if( gtk_tree_path_up(path) )
			{

				DB( g_print ("up ok\n") );

				if(gtk_tree_model_get_iter(model, &iter, path))
				{

					DB( g_print ("iter ok\n") );


					gtk_tree_model_get(model, &iter, LST_DEFCAT_DATAS, &item, -1);
					if( item->key != 0 )
					{
						*return_iter = iter;
						return item;
					}
				}
			}
		}
		else
		{

			DB( g_print ("path <=1\n") );

					gtk_tree_model_get(model, &iter, LST_DEFCAT_DATAS, &item, -1);

					if( item->key != 0 )
					{
						*return_iter = iter;
						return item;
					}


		}
	}
	return NULL;
}


void
ui_cat_listview_remove_selected(GtkTreeView *treeview)
{
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter, child;
gint n_child;

	DB( g_print("ui_cat_listview_remove_selected() \n") );

	selection = gtk_tree_view_get_selection(treeview);
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		//remove any children
		n_child = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(model), &iter);
		gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &iter);
		while(n_child > 0)
		{
			n_child--;
			
			gtk_tree_store_remove(GTK_TREE_STORE(model), &child);
			
			//After being removed, iter is set to the next valid row at that level, 
			//or invalidated if it previously pointed to the last one.
			//gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
		}

		gtk_tree_store_remove(GTK_TREE_STORE(model), &iter);
	}
}


static void
ui_cat_listview_sort_force(GtkTreeSortable *sortable, gpointer user_data)
{
gint sort_column_id;
GtkSortType order;

	DB( g_print("ui_cat_listview_sort_force()\n") );

	gtk_tree_sortable_get_sort_column_id(sortable, &sort_column_id, &order);
	DB( g_print(" - id %d order %d\n", sort_column_id, order) );

	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, order);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sortable), sort_column_id, order);
}


struct CatListContext
{
	GtkTreeModel *model;
	guint	 except_key;
	gint	 type;
	gchar    *needle;
	gushort	 *catmatch;
	gboolean showhidden;
};


static gboolean
ui_cat_listview_get_top_level (GtkTreeModel *liststore, guint32 key, GtkTreeIter *return_iter)
{
GtkTreeIter  iter;
gboolean     valid;
Category *item;

	//DB( g_print("ui_cat_listview_get_top_level() \n") );

    if( liststore != NULL )
    {
		valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(liststore), &iter);
		while (valid)
		{
			gtk_tree_model_get (liststore, &iter, LST_DEFCAT_DATAS, &item, -1);

			if(item->key == key)
			{
				*return_iter = iter;
				return TRUE;
			}

		 valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(liststore), &iter);
		}
	}

	return FALSE;
}


static void
ui_cat_listview_populate_cat_ghfunc(gpointer key, gpointer value, struct CatListContext *ctx)
{
GtkTreeIter  toplevel;
Category *item = value;
gint item_type;
gboolean matchtype = FALSE;
gboolean matchneedle = TRUE;
gboolean matchhidden = TRUE;

	//only cat (lvl1)
	if( item->parent != 0 )
		return;

	item_type = (item->flags & GF_INCOME) ? CAT_TYPE_INCOME : CAT_TYPE_EXPENSE; 
	if( (ctx->type == CAT_TYPE_ALL) 
	 || (ctx->type == item_type)
     //#1740368 add mixed cat as well
	 || (item->flags & GF_MIXED)
	 || item->key == 0
	)
		matchtype = TRUE;

	//disable search non matched
	if( (ctx->catmatch != NULL) && ctx->catmatch[item->key] == 0 )
		matchneedle = FALSE;

	if( (ctx->showhidden == FALSE) && (item->flags & GF_HIDDEN) )
		matchhidden = FALSE;

	if( matchtype && matchneedle && matchhidden )
	{
		gtk_tree_store_insert_with_values (GTK_TREE_STORE(ctx->model), &toplevel, NULL, -1,
			LST_DEFCAT_TOGGLE, FALSE,
			LST_DEFCAT_DATAS, item,
			//LST_DEFCAT_NAME, item->name,
			-1);
	}
	
}


static void
ui_cat_listview_populate_subcat_ghfunc(gpointer key, gpointer value, struct CatListContext *ctx)
{
GtkTreeIter  toplevel, child;
Category *item = value;
gboolean hasparent;
gboolean matchneedle = TRUE;
gboolean matchhidden = TRUE;

	//only subcat (lvl2)
	if( item->parent == 0 )
		return;

	hasparent = ui_cat_listview_get_top_level(ctx->model, item->parent, &toplevel);
	if( hasparent == FALSE )
		return;

	//#1740368 no need to filter on type, always insert all childs

	//disable search non matched
	if( (ctx->catmatch != NULL) && ctx->catmatch[item->key] == 0 )
		matchneedle = FALSE;

	if( (ctx->showhidden == FALSE) && (item->flags & GF_HIDDEN) )
		matchhidden = FALSE;

	if( matchneedle && matchhidden )
	{
		gtk_tree_store_insert_with_values (GTK_TREE_STORE(ctx->model), &child, &toplevel, -1,
			LST_DEFCAT_TOGGLE, FALSE,
			LST_DEFCAT_DATAS, item,
			//LST_DEFCAT_NAME, item->name,
			-1);
	}

}


void
ui_cat_listview_populate(GtkWidget *view, gint type, gchar *needle, gboolean showhidden)
{
GtkTreeModel *model;
struct CatListContext ctx = { 0 };
gboolean hastext = FALSE;
guint32 maxcat;
gushort *catmatch = NULL;
GList *lcat, *list;

	DB( g_print("ui_cat_listview_populate() \n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

	gtk_tree_store_clear (GTK_TREE_STORE(model));

	g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
	gtk_tree_view_set_model(GTK_TREE_VIEW(view), NULL); /* Detach model from view */

	if( needle != NULL)
	{
		hastext = (strlen(needle) >= 2) ? TRUE : FALSE;
		DB( g_print(" search: '%s' %s\n", needle, hastext ? "on":"off") );
	}

	if( hastext )
	{
		maxcat = da_cat_get_max_key();
		catmatch = g_malloc0((maxcat+2) * sizeof(gushort));
		if( catmatch != NULL )
		{
			// mark cat/subcat that match
			lcat = list = g_hash_table_get_values(GLOBALS->h_cat);
			while (list != NULL)
			{
			Category *item = list->data;
			gboolean match = hb_string_utf8_strstr(item->name, needle, FALSE);

				if(match)
				{
					catmatch[item->key]++;
					if(item->parent != 0)
						catmatch[item->parent]++;

					DB( g_print(" match %4d:%4d '%s'\n", item->parent, item->key, item->name) );
				}

				list = g_list_next(list);
			}
			g_list_free(lcat);
		}
	}

	/* clear and populate */
	ctx.model  = model;
	ctx.type   = type;
	ctx.needle = needle;
	ctx.catmatch = catmatch;
	ctx.showhidden = showhidden;
	
	/* we have to do this in 2 times to ensure toplevel (cat) will be added before childs */
	/* populate cat 1st */
	g_hash_table_foreach(GLOBALS->h_cat, (GHFunc)ui_cat_listview_populate_cat_ghfunc, &ctx);
	g_hash_table_foreach(GLOBALS->h_cat, (GHFunc)ui_cat_listview_populate_subcat_ghfunc, &ctx);


	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model); /* Re-attach model to view */
	g_object_unref(model);

	gtk_tree_view_expand_all (GTK_TREE_VIEW(view));

	if( hastext )
	{
		g_free(catmatch);
	}
}


static gboolean
ui_cat_listview_search_equal_func (GtkTreeModel *model,
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
Category *item;
	
  //gtk_tree_model_get_value (model, iter, column, &value);
  gtk_tree_model_get(model, iter, LST_DEFCAT_DATAS, &item, -1);

  if(item !=  NULL)
  {
	  normalized_string = g_utf8_normalize (item->name, -1, G_NORMALIZE_ALL);
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


GtkWidget *
ui_cat_listview_new(gboolean withtoggle, gboolean withcount)
{
GtkTreeStore *store;
GtkWidget *treeview;
GtkCellRenderer		*renderer;
GtkTreeViewColumn	*column;

	DB( g_print("ui_cat_listview_new() \n") );

	store = gtk_tree_store_new(
		NUM_LST_DEFCAT,
		G_TYPE_BOOLEAN,
		G_TYPE_POINTER
		//G_TYPE_STRING
		);

	treeview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	gtk_tree_view_set_grid_lines (GTK_TREE_VIEW (treeview), PREFS->grid_lines);
	gtk_tree_view_set_enable_tree_lines(GTK_TREE_VIEW (treeview), TRUE);


	#if MYDEBUG == 1
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_cat_listview_cell_data_function_debugkey, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	#endif


	// column: hide icon
	//#1826360 wish: archive payee/category to lighten the lists
	if( withtoggle == FALSE )
	{
		column = gtk_tree_view_column_new();
		renderer = gtk_cell_renderer_pixbuf_new ();
		//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_cat_listview_icon_cell_data_function, NULL, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
	}

	// column: toggle
	if( withtoggle == TRUE )
	{
		renderer = gtk_cell_renderer_toggle_new ();
		column = gtk_tree_view_column_new_with_attributes (_("Visible"),
						renderer, "active", LST_DEFCAT_TOGGLE, NULL);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

		gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), GTK_SELECTION_NONE);

		g_signal_connect (G_OBJECT(renderer), "toggled",
			    G_CALLBACK (ui_cat_listview_fixed_toggled), store);

		g_object_set_data(G_OBJECT(treeview), "togrdr_data", renderer);
	}

	// column: usage
	if( withcount == TRUE )
	{
		column = gtk_tree_view_column_new();
		//TRANSLATORS: 'txn' is abbrevation for transaction
		gtk_tree_view_column_set_title(column, _("# txn"));
		renderer = gtk_cell_renderer_text_new ();
		g_object_set(renderer, "xalign", 0.5, NULL);
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_cat_listview_count_txn_cell_data_function, GINT_TO_POINTER(LST_DEFCAT_DATAS), NULL);
		gtk_tree_view_column_set_alignment (column, 0.5);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_sort_column_id (column, LST_DEFCAT_SORT_USETXN);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
		//by degfault hide this column
		gtk_tree_view_column_set_visible(column, FALSE);

		column = gtk_tree_view_column_new();
		//TRANSLATORS: 'cfg' is abbrevation for configuration
		gtk_tree_view_column_set_title(column, _("# cfg"));
		renderer = gtk_cell_renderer_text_new ();
		g_object_set(renderer, "xalign", 0.5, NULL);
		gtk_tree_view_column_pack_start(column, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(column, renderer, ui_cat_listview_count_cfg_cell_data_function, GINT_TO_POINTER(LST_DEFCAT_DATAS), NULL);
		gtk_tree_view_column_set_alignment (column, 0.5);
		gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_sort_column_id (column, LST_DEFCAT_SORT_USECFG);
		gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);
		//by degfault hide this column
		gtk_tree_view_column_set_visible(column, FALSE);
	}

	// column: name
	renderer = gtk_cell_renderer_text_new ();

	g_object_set(renderer, 
		"ellipsize", PANGO_ELLIPSIZE_END,
	    "ellipsize-set", TRUE,
	    NULL);

	if( withtoggle == FALSE )
	{
		g_object_set(renderer, 
			//taken from nemo, not exactly a resize to content, but good compromise
			"width-chars", 40,
			NULL);
	}

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Category"));
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_cat_listview_text_cell_data_function, GINT_TO_POINTER(LST_DEFCAT_DATAS), NULL);
	//#2004631 date and column title alignement
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_column_set_resizable(column, TRUE);
	//gtk_tree_view_column_set_min_width(column, HB_MINWIDTH_LIST);
	gtk_tree_view_column_set_sort_column_id (column, LST_DEFCAT_SORT_NAME);
	gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

	gtk_tree_view_set_expander_column(GTK_TREE_VIEW(treeview), column);

	if( withtoggle == TRUE )
		gtk_tree_view_set_search_equal_func(GTK_TREE_VIEW(treeview), ui_cat_listview_search_equal_func, NULL, NULL);

	// treeview attribute
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(treeview), withcount);

	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFCAT_SORT_NAME, ui_cat_listview_compare_func, GINT_TO_POINTER(LST_DEFCAT_SORT_NAME), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFCAT_SORT_USETXN, ui_cat_listview_compare_func, GINT_TO_POINTER(LST_DEFCAT_SORT_USETXN), NULL);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFCAT_SORT_USECFG, ui_cat_listview_compare_func, GINT_TO_POINTER(LST_DEFCAT_SORT_USECFG), NULL);
	
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(store), LST_DEFCAT_SORT_NAME, GTK_SORT_ASCENDING);

	return treeview;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

/**
 * ui_cat_manage_filter_text_handler
 *
 *	filter to entry to avoid seizure of ':' char
 *
 */
static void
ui_cat_manage_filter_text_handler (GtkEntry    *entry,
                          const gchar *text,
                          gint         length,
                          gint        *position,
                          gpointer     data)
{
GtkEditable *editable = GTK_EDITABLE(entry);
gint i, count=0;
gchar *result = g_new0 (gchar, length+1);

  for (i=0; i < length; i++)
  {
    if (text[i]==':')
      continue;
    result[count++] = text[i];
  }


  if (count > 0) {
    g_signal_handlers_block_by_func (G_OBJECT (editable),
                                     G_CALLBACK (ui_cat_manage_filter_text_handler),
                                     data);
    gtk_editable_insert_text (editable, result, count, position);
    g_signal_handlers_unblock_by_func (G_OBJECT (editable),
                                       G_CALLBACK (ui_cat_manage_filter_text_handler),
                                       data);
  }
  g_signal_stop_emission_by_name (G_OBJECT (editable), "insert_text");

  g_free (result);
}


static void
ui_cat_manage_dialog_refilter(struct ui_cat_manage_dialog_data *data)
{
gint type;
gboolean showhidden;
gchar *needle;

	DB( g_print("\n[ui-cat-manage] refilter\n") );

	type = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type)) == 1 ? CAT_TYPE_INCOME : CAT_TYPE_EXPENSE;
	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));
	showhidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_showhidden));
	ui_cat_listview_populate(data->LV_cat, type, needle, showhidden);
	gtk_tree_view_expand_all (GTK_TREE_VIEW(data->LV_cat));
}


static void
ui_cat_manage_dialog_delete_unused(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data = user_data;
gboolean result;

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[ui-cat-manage] delete unused\n") );

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->dialog),
			_("Delete unused categories"),
			_("Are you sure you want to permanently\ndelete unused categories?"),
			_("_Delete"),
			TRUE
		);

	if( result == GTK_RESPONSE_OK )
	{
	GtkTreeModel *model;	

		//#1996275 fill usage before delete !
		if( data->usagefilled == FALSE )
		{
			category_fill_usage();
			data->usagefilled = TRUE;
		}
	
		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_cat));
		gtk_tree_store_clear (GTK_TREE_STORE(model));

		//#1917075
		data->change += category_delete_unused();
	
		ui_cat_manage_dialog_refilter (data);
	}
}


static void
ui_cat_manage_dialog_load_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data = user_data;
gchar *filename = NULL;
gchar *error;

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(widget), GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[ui-cat-manage] load csv\n") );

	if( ui_file_chooser_csv(GTK_WINDOW(data->dialog), GTK_FILE_CHOOSER_ACTION_OPEN, &filename, NULL) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		if(!category_load_csv(filename, &error))
		{
			ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
					_("File format error"),
					_("The CSV file must contains the exact numbers of column,\nseparated by a semi-colon, please see the help for more details.")
					);
		}

		g_free( filename );
		ui_cat_manage_dialog_refilter(data);
	}

}


static void
ui_cat_manage_dialog_save_csv(GSimpleAction *action, GVariant *parameter, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data = user_data;
gchar *filename = NULL;
gchar *error;

	DB( g_print("\n[ui-cat-manage] save csv\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( ui_file_chooser_csv(GTK_WINDOW(data->dialog), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, NULL) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );

		category_save_csv(filename, &error);
		g_free( filename );
	}
}


static void
ui_cat_manage_dialog_cb_show_usage (GtkToggleButton *button, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;
gboolean showusage;
GtkTreeViewColumn *column;

	DB( g_print("\n[ui-cat-manage] show usage\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW)), "inst_data");

	if( data->usagefilled == FALSE )
	{
		category_fill_usage();
		data->usagefilled = TRUE;
	}

	showusage = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_showusage));

	column = hbtk_treeview_get_column_by_id(GTK_TREE_VIEW(data->LV_cat), LST_DEFCAT_SORT_USETXN);
	if(column != NULL)
	{
		gtk_tree_view_column_set_visible(column, showusage);
	}
	column = hbtk_treeview_get_column_by_id(GTK_TREE_VIEW(data->LV_cat), LST_DEFCAT_SORT_USECFG);
	if(column != NULL)
	{
		gtk_tree_view_column_set_visible(column, showusage);
	}

}


static void
ui_cat_manage_dialog_cb_show_hidden (GtkToggleButton *button, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;

	DB( g_print("\n[ui-cat-manage] show hidden\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(button), GTK_TYPE_WINDOW)), "inst_data");
	ui_cat_manage_dialog_refilter(data);
}


/**
 * ui_cat_manage_dialog_add:
 *
 * add an empty new category/subcategory
 *
 */
static void
ui_cat_manage_dialog_add(GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;
gboolean isadded, subcat = GPOINTER_TO_INT(user_data);
const gchar *name;
//GtkTreeModel *model;
GtkWidget *tmpwidget;
Category *item, *paritem;
gint type;

	DB( g_print("\n[ui-cat-manage] add\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("(data=%p) is subcat=%d\n", data, subcat) );

	tmpwidget = (subcat == FALSE ? data->ST_name1 : data->ST_name2);
	name = gtk_entry_get_text(GTK_ENTRY(tmpwidget));
	//model  = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_cat));

	item = da_cat_malloc();
	item->name = g_strdup(name);
	g_strstrip(item->name);

	isadded = FALSE;
	if( strlen(item->name) > 0 )
	{
		/* if cat use new id */
		if(subcat == FALSE)
		{
			type = hbtk_switcher_get_active (HBTK_SWITCHER(data->RA_type));
			if(type == 1)
				item->flags |= GF_INCOME;

			isadded = da_cat_append(item);
			if( isadded == TRUE )
			{
				DB( g_print(" => add cat: %p %d, %s type=%d\n", item, subcat, item->name, type) );
				ui_cat_listview_add(GTK_TREE_VIEW(data->LV_cat), item, NULL);
				data->change++;
			}
		}
		/* if subcat use parent id & gf_income */
		else
		{
		GtkTreeIter parent_iter;

			paritem = ui_cat_listview_get_selected_parent(GTK_TREE_VIEW(data->LV_cat), &parent_iter);
			if(paritem)
			{
				DB( g_print(" => selitem parent: %d, %s\n", paritem->key, paritem->name) );

				item->parent = paritem->key;
				item->flags |= (paritem->flags & GF_INCOME);
				item->flags |= GF_SUB;

				isadded = da_cat_append(item);
				if( isadded == TRUE )
				{
					DB( g_print(" => add subcat: %p %d, %s\n", item, subcat, item->name) );
					ui_cat_listview_add(GTK_TREE_VIEW(data->LV_cat), item, &parent_iter);
					data->change++;
				}
			}
		}
	}

	//#2051349 warn user and free lack
	if( isadded == FALSE )
	{
		DB( g_print(" existing item\n") );
		da_cat_free(item);
		ui_dialog_msg_infoerror(GTK_WINDOW(data->dialog), GTK_MESSAGE_ERROR,
			_("Error"),
			_("Duplicate category name. Try another name.") );
	}

	gtk_entry_set_text(GTK_ENTRY(tmpwidget),"");
}


static void
ui_cat_manage_dialog_edit_entry_cb(GtkEditable *editable, gpointer user_data)
{
GtkDialog *window = user_data;
const gchar *buffer;

	DB( g_print("\n[ui-cat-manage] edit cb\n") );

	buffer = gtk_entry_get_text(GTK_ENTRY(editable));
	gtk_dialog_set_response_sensitive(GTK_DIALOG(window), GTK_RESPONSE_ACCEPT, strlen(buffer) > 0 ? TRUE : FALSE);
}


static void
ui_cat_manage_dialog_edit(GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;
GtkWidget *dialog, *content_area, *grid;
GtkWidget *label, *w_name, *w_type = NULL, *w_child;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;
gint row, n_child;

	DB( g_print("\n[ui-cat-manage] edit\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Category *item;

		gtk_tree_model_get(model, &iter, LST_DEFCAT_DATAS, &item, -1);

		n_child = gtk_tree_model_iter_n_children(model, &iter);

		dialog = gtk_dialog_new_with_buttons (_("Edit Category"),
						    GTK_WINDOW (data->dialog),
						    0,
						    _("_Cancel"),
						    GTK_RESPONSE_REJECT,
						    _("_OK"),
						    GTK_RESPONSE_ACCEPT,
						    NULL);

		content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

		grid = gtk_grid_new();
		gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_SMALL);
		gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
		hb_widget_set_margin(GTK_WIDGET(grid), SPACING_LARGE);
		hbtk_box_prepend (GTK_BOX (content_area), grid);

		// group :: General
		row = 0;
		label = make_label_widget(_("_Name:"));
		gtk_grid_attach (GTK_GRID (grid), label, 1, row, 1, 1);
		w_name = gtk_entry_new();
		//gtk_widget_set_size_request (w_name, HB_MINWIDTH_LIST, -1);
		gtk_widget_set_hexpand(w_name, TRUE);
		gtk_entry_set_text(GTK_ENTRY(w_name), item->name);
		gtk_entry_set_activates_default (GTK_ENTRY(w_name), TRUE);
		gtk_widget_grab_focus (w_name);
		gtk_grid_attach (GTK_GRID (grid), w_name, 2, row, 1, 1);

		// group :: Type
		row++;
		label = make_label_group(_("Change Type"));
		gtk_widget_set_margin_top(label, SPACING_LARGE);
		gtk_grid_attach (GTK_GRID (grid), label, 0, row, 3, 1);

		row++;
		w_type = gtk_check_button_new_with_mnemonic(_("_Income"));
		gtk_grid_attach (GTK_GRID (grid), w_type, 1, row, 2, 1);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w_type), item->flags & GF_INCOME ? TRUE : FALSE);

		row++;
		w_child = gtk_check_button_new_with_mnemonic(_("Propagate to _children"));
		gtk_grid_attach (GTK_GRID (grid), w_child, 1, row, 2, 1);

		g_signal_connect (G_OBJECT (w_name), "changed", G_CALLBACK (ui_cat_manage_dialog_edit_entry_cb), dialog);

		gtk_widget_show_all(grid);


		if( (item->flags & GF_SUB) || n_child == 0 )
		{
			gtk_widget_hide(w_child);
		}

		
		gtk_dialog_set_default_response(GTK_DIALOG( dialog ), GTK_RESPONSE_ACCEPT);

		//wait for the user
		gint result = gtk_dialog_run (GTK_DIALOG (dialog));

		if(result == GTK_RESPONSE_ACCEPT)
		{
		const gchar *name;

			// 1: manage renaming
			name = gtk_entry_get_text(GTK_ENTRY(w_name));
			// ignore if item is empty
			if (name && *name)
			{
				if( category_rename(item, name) )
				{
					//to redraw the active entry
					gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_cat));
					data->change++;
				}
				else
				{
				Category *parent;
				gchar *fromname, *toname = NULL;

					fromname = item->fullname;

					if( item->parent == 0)
						toname = g_strdup(name);
					else
					{
						parent = da_cat_get(item->parent);
						if( parent )
						{
							toname = g_strdup_printf("%s:%s", parent->name, name);
						}
					}

					ui_dialog_msg_infoerror(GTK_WINDOW(dialog), GTK_MESSAGE_ERROR,
						_("Error"),
						_("Cannot rename this Category,\n"
						"from '%s' to '%s',\n"
						"this name already exists."),
						fromname,
						toname
						);

					g_free(toname);
				}
			}
	
			// 2: manage flag change
			gboolean isIncome = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_type));
			gboolean doChild  = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w_child));
			data->change += category_change_type(item, isIncome, doChild);
			
			ui_cat_listview_sort_force(GTK_TREE_SORTABLE(model), NULL);
	    }

		// cleanup and destroy
		gtk_window_destroy (GTK_WINDOW(dialog));
	}

}


static void
ui_cat_manage_dialog_merge_entry_cb(GtkEditable *editable, gpointer user_data)
{
GtkDialog *window = user_data;
const gchar *buffer;

	DB( g_print("\n[ui-cat-manage] merge cb\n") );

	buffer = gtk_entry_get_text(GTK_ENTRY(editable));
	gtk_dialog_set_response_sensitive(GTK_DIALOG(window), GTK_RESPONSE_OK, strlen(buffer) > 0 ? TRUE : FALSE);
}


static void
ui_cat_manage_dialog_merge(GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;
GtkWidget *dialog, *content, *mainvbox;
GtkWidget *getwidget, *cb_subcat, *togglebutton;
GtkTreeSelection *selection;
GtkTreeModel *model;
GtkTreeIter iter;

	DB( g_print("\n[ui-cat-manage] merge\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	Category *srccat;
	gchar *title;
	gchar *secondtext;

		gtk_tree_model_get(model, &iter, LST_DEFCAT_DATAS, &srccat, -1);

		title = g_strdup_printf (
			_("Merge category '%s'"), srccat->name);

		dialog = gtk_message_dialog_new (GTK_WINDOW (data->dialog),
			                              GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			                              GTK_MESSAGE_WARNING,
			                              GTK_BUTTONS_NONE,
			                              title,
		                                  NULL
			                              );

		gtk_dialog_add_buttons (GTK_DIALOG(dialog),
				_("_Cancel"), GTK_RESPONSE_CANCEL,
				_("Merge"), GTK_RESPONSE_OK,
				NULL);

		gtk_dialog_set_default_response(GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

		content = gtk_message_dialog_get_message_area(GTK_MESSAGE_DIALOG (dialog));
		mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
		hbtk_box_prepend (GTK_BOX (content), mainvbox);

		secondtext = _("Transactions assigned to this category,\n"
			  "will be moved to the category selected below.");

		g_object_set(GTK_MESSAGE_DIALOG (dialog), "secondary-text", secondtext, NULL);
		g_free(title);

		//getwidget = ui_cat_comboboxentry_new(NULL);
		getwidget = ui_cat_entry_popover_new(NULL);
		gtk_box_prepend (GTK_BOX (mainvbox), getwidget);

		cb_subcat = gtk_check_button_new_with_mnemonic(_("Include _subcategories"));
		gtk_box_prepend (GTK_BOX (mainvbox), cb_subcat);

		//#2079801 warn budget
		secondtext = g_strdup_printf (
			_("_Delete the category '%s' (and any budget)"), srccat->name);
		togglebutton = gtk_check_button_new_with_mnemonic(secondtext);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(togglebutton), TRUE);
		g_free(secondtext);
		gtk_box_prepend (GTK_BOX (mainvbox), togglebutton);

		//setup 
		//gtk_combo_box_set_active(GTK_COMBO_BOX(getwidget), oldpos);
		g_signal_connect (G_OBJECT (ui_cat_entry_popover_get_entry(GTK_BOX(getwidget))), "changed", G_CALLBACK (ui_cat_manage_dialog_merge_entry_cb), dialog);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);

		//5.5 done in popover, let's keep the src as well
		//ui_cat_comboboxentry_populate_except(GTK_COMBO_BOX(getwidget), GLOBALS->h_cat, srccat->key);
		gtk_widget_grab_focus (getwidget);

		gtk_widget_show_all(mainvbox);

		hb_widget_visible(cb_subcat, (srccat->flags & GF_SUB) ? FALSE : TRUE);

		//wait for the user
		gint result = gtk_dialog_run (GTK_DIALOG (dialog));

		if(result == GTK_RESPONSE_OK)
		{
		GtkTreeModel *model;
		Category *newcat, *parent;
		gboolean dosubcat;
		guint dstcatkey;

			//dstcatkey = ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(getwidget));
			dstcatkey = ui_cat_entry_popover_get_key_add_new(GTK_BOX(getwidget));
			dosubcat = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(cb_subcat));

			//do nothing if src = dst...
			if( srccat->key != dstcatkey )
			{

				DB( g_print(" -> move cat to %d (subcat=%d)\n", dstcatkey, dosubcat) );

				model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_cat));
				gtk_tree_store_clear (GTK_TREE_STORE(model));

				category_move(srccat->key, dstcatkey, dosubcat);

				newcat = da_cat_get (dstcatkey);

				//#1771720: update count
				//TODO: this is imperfect here, as if subcat, we don't count ?
				newcat->nb_use_all += srccat->nb_use_all;
				newcat->nb_use_txn += srccat->nb_use_txn;
				srccat->nb_use_all = 0;
				srccat->nb_use_txn = 0;
				
				//keep the income type with us
				parent = da_cat_get(srccat->parent);
				if(parent != NULL && (parent->flags & GF_INCOME))
					newcat->flags |= GF_INCOME;

				//add the new category into listview
				if(newcat)
					ui_cat_listview_add(GTK_TREE_VIEW(data->LV_cat), newcat, NULL);

				// delete the old category
				if( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)) )
				{
					DB( g_print(" -> delete %d '%s'\n", srccat->key, srccat->name ) );

					ui_cat_listview_remove_selected(GTK_TREE_VIEW(data->LV_cat));
					da_cat_delete(srccat->key);
					//#2079801 later delete budget
				}


				data->change++;

				ui_cat_manage_dialog_refilter(data);
			}
		}

		// cleanup and destroy
		gtk_window_destroy (GTK_WINDOW(dialog));
	}

}


/*
** delete the selected category to our treeview and temp GList
*/
static void
ui_cat_manage_dialog_delete(GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;
Category *item;
gint result;

	DB( g_print("\n[ui-cat-manage] delete\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	item = ui_cat_listview_get_selected(GTK_TREE_VIEW(data->LV_cat));
	if( item != NULL && item->key != 0 )
	{
	gchar *title = NULL;
	gchar *secondtext = NULL;

		title = g_strdup_printf (
			_("Are you sure you want to permanently delete '%s'?"), item->name);

		if( item->nb_use_all > 0 )
		{
			secondtext = _("This category is used.\n"
			    "Any transaction using that category will be set to (no category)");
		}

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
			ui_cat_listview_remove_selected(GTK_TREE_VIEW(data->LV_cat));
			category_move(item->key, 0, TRUE);
			da_cat_delete(item->key);
			data->change++;
		}

	}
}


static void
ui_cat_manage_dialog_expand_all(GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;

	DB( g_print("\n[ui-cat-manage] expand all\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_cat));

}


static void
ui_cat_manage_dialog_collapse_all(GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;

	DB( g_print("\n[ui-cat-manage] collapse all\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	gtk_tree_view_collapse_all(GTK_TREE_VIEW(data->LV_cat));

}


//#1826360 wish: archive payee/category to lighten the lists
static void
ui_cat_manage_dialog_hide(GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;
GtkTreeSelection *selection;
GtkTreeModel     *model;
GtkTreeIter      iter, child;
Category *item;
gboolean showhidden;
gint n_child;

	DB( g_print("\n[ui-cat-manage] hide\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	showhidden = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->BT_showhidden));

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat));
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		//manage children
		n_child = gtk_tree_model_iter_n_children (GTK_TREE_MODEL(model), &iter);
		gtk_tree_model_iter_children (GTK_TREE_MODEL(model), &child, &iter);
	    while(n_child > 0)
    	{
			gtk_tree_model_get(model, &child, LST_DEFCAT_DATAS, &item, -1);
			if( item != NULL )
			{
				item->flags ^= GF_HIDDEN;
				data->change++;
			}

			n_child--;
			gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &child);
		}

		gtk_tree_model_get(model, &iter, LST_DEFCAT_DATAS, &item, -1);
		if( item != NULL )
		{
			item->flags ^= GF_HIDDEN;
			data->change++;
		}
	}

	if( showhidden )
		//refresh
		gtk_widget_queue_draw(data->LV_cat);
	else
		ui_cat_listview_remove_selected(GTK_TREE_VIEW(data->LV_cat));
	
}


static void
ui_cat_manage_dialog_update(GtkWidget *treeview, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
GtkTreePath *path;
gchar *category = NULL;
gboolean selected, sensitive;
gboolean haschild = FALSE;

	DB( g_print("\n[ui-cat-manage] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");
	//window = gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW);
	//DB( g_print("(defcategory) widget=%08lx, window=%08lx, inst_data=%08lx\n", treeview, window, data) );

	//if true there is a selected node
	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat)), &model, &iter);
	if (selected)
	{
	gchar *tree_path_str;
	Category *item;

		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
			LST_DEFCAT_DATAS, &item,
			-1);

		if( item->key == 0 )
			selected = FALSE;

		haschild = gtk_tree_model_iter_has_child(GTK_TREE_MODEL(model), &iter);
		DB( g_print(" => has child=%d\n", haschild) );


		path = gtk_tree_model_get_path(gtk_tree_view_get_model(GTK_TREE_VIEW(treeview)), &iter);
		tree_path_str = gtk_tree_path_to_string(path);
		DB( g_print(" => select is=%s, depth=%d (id=%d, %s) flags=%d\n",
			tree_path_str,
			gtk_tree_path_get_depth(path),
			item->key,
			item->name,
			item->flags
			) );
		g_free(tree_path_str);

		//get parent if subcategory selected
		DB( g_print(" => get parent for title\n") );
		if(gtk_tree_path_get_depth(path) != 1)
			gtk_tree_path_up(path);

		if(gtk_tree_model_get_iter(model, &iter, path))
		{
		Category *tmpitem;

			gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
				LST_DEFCAT_DATAS, &tmpitem,
				-1);

			if(tmpitem->key > 0)
				category = tmpitem->name;

			DB( g_print(" => parent is %s\n", category) );

		}

		gtk_tree_path_free(path);
	}

	DB( g_print(" selected = %d\n", selected) );

	gtk_label_set_text(GTK_LABEL(data->LA_category), category);

	sensitive = (selected == TRUE) ? TRUE : FALSE;
	gtk_widget_set_sensitive(data->ST_name2, sensitive);
	gtk_widget_set_sensitive(data->BT_edit, sensitive);
	gtk_widget_set_sensitive(data->BT_merge, sensitive);
	gtk_widget_set_sensitive(data->BT_hide, sensitive);

	//avoid deleting top categories
	sensitive = (haschild == TRUE) ? FALSE : sensitive;
	gtk_widget_set_sensitive(data->BT_delete, sensitive);
}


static gboolean
ui_cat_manage_dialog_cb_on_key_press(GtkWidget *source, GdkEvent *event, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data = user_data;
GdkModifierType state;
guint keyval;

	gdk_event_get_state (event, &state);
	gdk_event_get_keyval(event, &keyval);

	// On Control-f enable search entry
	if (state & GDK_CONTROL_MASK && keyval == GDK_KEY_f)
	{
		gtk_widget_grab_focus(data->ST_search);
	}
	else
	if (keyval == GDK_KEY_Escape && gtk_widget_has_focus(data->ST_search))
	{
		hbtk_entry_set_text(GTK_ENTRY(data->ST_search), NULL);
		gtk_widget_grab_focus(data->LV_cat);
		return TRUE;
	}

	return GDK_EVENT_PROPAGATE;
}


static void
ui_cat_manage_dialog_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	DB( g_print("\n[ui-cat-manage] selection\n") );

	ui_cat_manage_dialog_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}

static void
ui_cat_manage_dialog_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            user_data)
{
GtkTreeModel		 *model;
GtkTreeIter			 iter;

	DB( g_print("\n[ui-cat-manage] onRowActivated\n") );

	//TODO: check if not none, should be done into edit
	model = gtk_tree_view_get_model(treeview);
	gtk_tree_model_get_iter_first(model, &iter);
	if(gtk_tree_selection_iter_is_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), &iter) == FALSE)
	{
		ui_cat_manage_dialog_edit(GTK_WIDGET(treeview), NULL);
	}
}


static gboolean
ui_cat_manage_dialog_cleanup(struct ui_cat_manage_dialog_data *data, gint result)
{
gboolean doupdate = FALSE;

	DB( g_print("\n[ui-cat-manage] cleanup\n") );

	if(result == GTK_RESPONSE_ACCEPT)
	{

		//do_application_specific_something ();
		DB( g_print(" accept\n") );


		GLOBALS->changes_count += data->change;
	}

	DB( g_print(" free tmp_list\n") );

	//da_category_destroy(data->tmp_list);

	return doupdate;
}


static void
ui_cat_manage_type_changed_cb (GtkToggleButton *button, gpointer user_data)
{
	ui_cat_manage_dialog_refilter(user_data);
	//g_print(" toggle type=%d\n", gtk_toggle_button_get_active(button));
}


static void
ui_cat_manage_search_changed_cb (GtkWidget *widget, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data = user_data;

	DB( g_printf("\n[ui-cat-manage] search_changed_cb\n") );

	ui_cat_manage_dialog_refilter(data);
}


static void
ui_cat_manage_dialog_setup(struct ui_cat_manage_dialog_data *data)
{

	DB( g_print("\n[ui-cat-manage] setup\n") );

	DB( g_print(" init data\n") );

	//init GList
	data->tmp_list = NULL; //data->tmp_list = hb-glist_clone_list(GLOBALS->cat_list, sizeof(struct _Group));
	data->change = 0;
	data->usagefilled = FALSE;

	//#2051419 show hidden by default
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->BT_showhidden), TRUE);

	DB( g_print(" populate\n") );
	
	//debug
	//da_cat_debug_list();
	ui_cat_manage_dialog_refilter(data);

	//DB( g_print(" set widgets default\n") );

	DB( g_print(" connect widgets signals\n") );	

    g_signal_connect (G_OBJECT (data->BT_showhidden), "toggled", G_CALLBACK (ui_cat_manage_dialog_cb_show_hidden), NULL);
    g_signal_connect (G_OBJECT (data->BT_showusage) , "toggled", G_CALLBACK (ui_cat_manage_dialog_cb_show_usage), NULL);

	g_signal_connect (G_OBJECT (data->RA_type), "changed", G_CALLBACK (ui_cat_manage_type_changed_cb), data);

	g_object_bind_property (data->BT_add, "active", data->RE_addreveal, "reveal-child", G_BINDING_BIDIRECTIONAL);

	gtk_tree_view_set_search_entry(GTK_TREE_VIEW(data->LV_cat), GTK_ENTRY(data->ST_search));

	g_signal_connect (G_OBJECT (data->ST_search), "search-changed", G_CALLBACK (ui_cat_manage_search_changed_cb), data);

	g_signal_connect (G_OBJECT (data->ST_name1), "activate", G_CALLBACK (ui_cat_manage_dialog_add), GINT_TO_POINTER(FALSE));
	g_signal_connect (G_OBJECT (data->ST_name2), "activate", G_CALLBACK (ui_cat_manage_dialog_add), GINT_TO_POINTER(TRUE));

	g_signal_connect(G_OBJECT(data->ST_name1), "insert-text", G_CALLBACK(ui_cat_manage_filter_text_handler), NULL);
	g_signal_connect(G_OBJECT(data->ST_name2), "insert-text", G_CALLBACK(ui_cat_manage_filter_text_handler), NULL);


	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_cat)), "changed", G_CALLBACK (ui_cat_manage_dialog_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(data->LV_cat), "row-activated", G_CALLBACK (ui_cat_manage_dialog_onRowActivated), NULL);

	g_signal_connect (G_OBJECT (data->BT_edit), "clicked", G_CALLBACK (ui_cat_manage_dialog_edit), NULL);
	g_signal_connect (G_OBJECT (data->BT_merge), "clicked", G_CALLBACK (ui_cat_manage_dialog_merge), NULL);
	g_signal_connect (G_OBJECT (data->BT_delete), "clicked", G_CALLBACK (ui_cat_manage_dialog_delete), NULL);

	g_signal_connect (G_OBJECT (data->BT_hide), "clicked", G_CALLBACK (ui_cat_manage_dialog_hide), NULL);

	g_signal_connect (G_OBJECT (data->BT_expand), "clicked", G_CALLBACK (ui_cat_manage_dialog_expand_all), NULL);
	g_signal_connect (G_OBJECT (data->BT_collapse), "clicked", G_CALLBACK (ui_cat_manage_dialog_collapse_all), NULL);

}


static gboolean
ui_cat_manage_mapped (GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct ui_cat_manage_dialog_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( data->mapped_done == TRUE )
		return FALSE;

	DB( g_print("\n[ui-cat-manage] mapped\n") );

	ui_cat_manage_dialog_setup(data);
	ui_cat_manage_dialog_update(data->LV_cat, NULL);

	data->mapped_done = TRUE;

	return FALSE;
}


static const GActionEntry win_actions[] = {
	{ "imp"		, ui_cat_manage_dialog_load_csv, NULL, NULL, NULL, {0,0,0} },
	{ "exp"		, ui_cat_manage_dialog_save_csv, NULL, NULL, NULL, {0,0,0} },
	{ "del"		, ui_cat_manage_dialog_delete_unused, NULL, NULL, NULL, {0,0,0} },
//	{ "actioname"	, not_implemented, NULL, NULL, NULL, {0,0,0} },
};


GtkWidget *ui_cat_manage_dialog (void)
{
struct ui_cat_manage_dialog_data *data;
GtkWidget *dialog, *content, *mainvbox, *bbox, *table, *hbox, *vbox, *label, *scrollwin, *treeview;
GtkWidget *widget, *image, *tbar, *revealer;
gint w, h, dw, dh, row;

	DB( g_print("\n[ui-cat-manage] new\n") );

	data = g_malloc0(sizeof(struct ui_cat_manage_dialog_data));
	if(!data) return NULL;

	dialog = gtk_dialog_new_with_buttons (_("Manage Categories"),
					    GTK_WINDOW(GLOBALS->mainwindow),
					    0,
					    _("_Close"),
					    GTK_RESPONSE_ACCEPT,
					    NULL);

	data->dialog = dialog;
	data->change = 0;

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	dh = (h*1.33/PHI);
	//ratio 2:3
	dw = (dh * 2) / 3;
	DB( g_print(" main w=%d h=%d => diag w=%d h=%d\n", w, h, dw, dh) );
	gtk_window_set_default_size (GTK_WINDOW(dialog), dw, dh);
	
	//store our window private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" dialog=%p, inst_data=%p\n", dialog, data) );


	//dialog contents
	content = gtk_dialog_get_content_area(GTK_DIALOG (dialog));
	mainvbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	hbtk_box_prepend (GTK_BOX (content), mainvbox);
	hb_widget_set_margin(GTK_WIDGET(mainvbox), SPACING_LARGE);

    //our table
	table = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	hbtk_box_prepend (GTK_BOX (mainvbox), table);

	//filter part
	row = 0;
	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_grid_attach (GTK_GRID (table), bbox, 0, row, 2, 1);
	
	widget = make_image_toggle_button(ICONNAME_HB_BUTTON_HIDE, _("Show Hidden") );
	data->BT_showhidden = widget;
	gtk_box_prepend (GTK_BOX (bbox), widget);

	widget = make_image_toggle_button(ICONNAME_HB_BUTTON_USAGE, _("Show Usage") );
	data->BT_showusage = widget;
	gtk_box_prepend (GTK_BOX (bbox), widget);
	
	widget = hbtk_switcher_new (GTK_ORIENTATION_HORIZONTAL);
	hbtk_switcher_setup(HBTK_SWITCHER(widget), CYA_CAT_TYPE, TRUE);
	data->RA_type = widget;
	//gtk_widget_set_halign (bbox, GTK_ALIGN_CENTER);
	gtk_box_prepend (GTK_BOX (bbox), widget);

	//menubutton
	widget = gtk_menu_button_new();
	image = hbtk_image_new_from_icon_name_16 (ICONNAME_HB_BUTTON_MENU);
	g_object_set (widget, "image", image,  NULL);
	gtk_widget_set_halign (widget, GTK_ALIGN_END);
	gtk_box_append (GTK_BOX (bbox), widget);

	GMenu *menu = g_menu_new ();
	GMenu *section = g_menu_new ();
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("_Import CSV..."), "win.imp");
	g_menu_append (section, _("E_xport CSV..."), "win.exp");
	g_object_unref (section);

	section = g_menu_new ();
	g_menu_append_section(menu, NULL, G_MENU_MODEL(section));
	g_menu_append (section, _("_Delete unused..."), "win.del");
	g_object_unref (section);

	GActionGroup *group = (GActionGroup*)g_simple_action_group_new ();
	data->actions = group;
	g_action_map_add_action_entries (G_ACTION_MAP (group), win_actions, G_N_ELEMENTS (win_actions), data);

	gtk_widget_insert_action_group (widget, "win", G_ACTION_GROUP(group));
	gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (widget), G_MENU_MODEL (menu));

	
	widget = make_search();
	data->ST_search = widget;
	gtk_box_append(GTK_BOX (bbox), widget);


	// list + toolbar
	row++;
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_grid_attach (GTK_GRID (table), vbox, 0, row, 2, 1);
	
	scrollwin = make_scrolled_window(GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrollwin), HB_MINHEIGHT_LIST);
 	treeview = ui_cat_listview_new(FALSE, TRUE);
	data->LV_cat = treeview;
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW(scrollwin), treeview);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	hbtk_box_prepend (GTK_BOX(vbox), scrollwin);

	//list toolbar
	tbar = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
	gtk_style_context_add_class (gtk_widget_get_style_context (tbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_box_prepend (GTK_BOX (vbox), tbar);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (tbar), bbox);

		widget = make_image_toggle_button(ICONNAME_LIST_ADD, _("Add"));
		data->BT_add = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

		widget = make_image_button(ICONNAME_LIST_DELETE, _("Delete"));
		data->BT_delete = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (tbar), bbox);

		widget = make_image_button(ICONNAME_LIST_EDIT, _("Edit"));
		data->BT_edit = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

		widget = make_image_button(ICONNAME_HB_LIST_MERGE, _("Move/Merge"));
		data->BT_merge = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);

	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_prepend (GTK_BOX (tbar), bbox);

		widget = make_image_button(ICONNAME_HB_BUTTON_HIDE, _("Show/Hide"));
		data->BT_hide = widget;
		gtk_box_prepend(GTK_BOX(bbox), widget);
	
	bbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (GTK_BOX (tbar), bbox);
	
		widget = make_image_button(ICONNAME_HB_BUTTON_EXPAND, _("Expand all"));
		data->BT_expand = widget;
		gtk_box_prepend (GTK_BOX (bbox), widget);

		widget = make_image_button(ICONNAME_HB_BUTTON_COLLAPSE, _("Collapse all"));
		data->BT_collapse = widget;
		gtk_box_prepend (GTK_BOX (bbox), widget);

	// subcategory + add button
	row++;
	revealer = gtk_revealer_new ();
	data->RE_addreveal = revealer;
	gtk_grid_attach (GTK_GRID (table), revealer, 0, row, 2, 1);
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, SPACING_SMALL);
	gtk_revealer_set_child (GTK_REVEALER(revealer), vbox);

	widget = gtk_entry_new ();
	data->ST_name1 = widget;
	gtk_entry_set_placeholder_text(GTK_ENTRY(data->ST_name1), _("new category") );
	hbtk_box_prepend (GTK_BOX (vbox), widget);
	
	row++;
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_box_prepend (GTK_BOX (vbox), hbox);
	data->LA_category = gtk_label_new(NULL);
	gtk_box_prepend (GTK_BOX (hbox), data->LA_category);
	label = gtk_label_new(":");
	gtk_box_prepend (GTK_BOX (hbox), label);
	data->ST_name2 = gtk_entry_new ();
	gtk_entry_set_placeholder_text(GTK_ENTRY(data->ST_name2), _("new subcategory") );
	hbtk_box_prepend (GTK_BOX (hbox), data->ST_name2);


	// connect dialog signals
    g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);
	g_signal_connect (dialog, "map-event", G_CALLBACK (ui_cat_manage_mapped), &dialog);
	g_signal_connect (dialog, "key-press-event", G_CALLBACK (ui_cat_manage_dialog_cb_on_key_press), (gpointer)data);


	// show & run dialog
	DB( g_print(" run dialog\n") );
	gtk_widget_show_all (dialog);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup and destroy
	ui_cat_manage_dialog_cleanup(data, result);
	gtk_window_destroy (GTK_WINDOW(dialog));

	g_free(data);

	return NULL;
}


